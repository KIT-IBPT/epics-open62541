/*
 * Copyright 2017-2024 aquenos GmbH.
 * Copyright 2017-2024 Karlsruhe Institute of Technology.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * This software has been developed by aquenos GmbH on behalf of the
 * Karlsruhe Institute of Technology's Institute for Beam Physics and
 * Technology.
 *
 * This software contains code originally developed by aquenos GmbH for
 * the s7nodave EPICS device support. aquenos GmbH has relicensed the
 * affected portions of code from the s7nodave EPICS device support
 * (originally licensed under the terms of the GNU GPL) under the terms
 * of the GNU LGPL version 3 or newer.
 */

#include <chrono>
#include <fstream>
#include <iterator>

#include "open62541Error.h"
#include "UaException.h"

#include "ServerConnection.h"

namespace open62541 {
namespace epics {

ServerConnection::ServerConnection(const std::string &endpointUrl) :
    ServerConnection(endpointUrl, std::string(), std::string(), false,
      SecurityMode::invalid, std::string(), std::string(), std::string(),
      std::string(), false) {
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    SecurityMode securityMode, const std::string &clientCertPath,
    const std::string &clientKeyPath, const std::string &serverCertPath,
    const std::string &applicationUri) :
    ServerConnection(endpointUrl, std::string(), std::string(), false,
      securityMode, clientCertPath, clientKeyPath, serverCertPath,
      applicationUri, true) {
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const std::string &username, const std::string &password) :
    ServerConnection(endpointUrl, username, password, true,
      SecurityMode::invalid, std::string(), std::string(), std::string(),
      std::string(), false) {
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const std::string &username, const std::string &password,
    SecurityMode securityMode, const std::string &clientCertPath,
    const std::string &clientKeyPath, const std::string &serverCertPath,
    const std::string &applicationUri) :
    ServerConnection(endpointUrl, username, password, true, securityMode,
    clientCertPath, clientKeyPath, serverCertPath, applicationUri, true) {
}

ServerConnection::~ServerConnection() {
  // Tell the connection thread to shutdown.
  shutdownRequested.store(true, std::memory_order_release);
  requestQueueCv.notify_all();
  // Wait for the shutdown to finish.
  if (connectionThread.joinable()) {
    connectionThread.join();
  }
  // Most likely the lock guard is not needed here, but taking the mutex should
  // not cost us much and we do not have to worry about any side effects.
  {
    std::lock_guard<std::mutex> lock(mutex);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    client = nullptr;
  }
}

void ServerConnection::addMonitoredItem(const std::string &subscriptionName,
    const UaNodeId &nodeId,
    std::shared_ptr<ServerConnection::MonitoredItemCallback> const &callback,
    double samplingInterval, std::uint32_t queueSize, bool discardOldest) {
  std::unique_ptr<Request> request(new AddMonitoredItemRequest(
    callback, discardOldest, nodeId, queueSize, samplingInterval, subscriptionName));
  {
    std::lock_guard<std::mutex> lock(requestQueueMutex);
    requestQueue.push_back(std::move(request));
  }
  requestQueueCv.notify_all();
}

std::uint32_t ServerConnection::getSubscriptionLifetimeCount(
    const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex);
  return subscriptions[name].lifetimeCount;
}

std::uint32_t ServerConnection::getSubscriptionMaxKeepAliveCount(
    const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex);
  return subscriptions[name].maxKeepAliveCount;
}

double ServerConnection::getSubscriptionPublishingInterval(
    const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex);
  return subscriptions[name].publishingInterval;
}

UaVariant ServerConnection::read(const UaNodeId &nodeId) {
  std::lock_guard<std::mutex> lock(mutex);
  return readInternal(nodeId);
}

void ServerConnection::readAsync(const UaNodeId &nodeId,
    std::shared_ptr<ReadCallback> callback) {
  std::unique_ptr<Request> request(new ReadRequest(callback, nodeId));
  {
    std::lock_guard<std::mutex> lock(requestQueueMutex);
    requestQueue.push_back(std::move(request));
  }
  requestQueueCv.notify_all();
}

void ServerConnection::removeMonitoredItem(const std::string &subscriptionName,
    const UaNodeId &nodeId,
    std::shared_ptr<ServerConnection::MonitoredItemCallback> const &callback) {
  std::unique_ptr<Request> request(new RemoveMonitoredItemRequest(
    callback, nodeId, subscriptionName));
  {
    std::lock_guard<std::mutex> lock(requestQueueMutex);
    requestQueue.push_back(std::move(request));
  }
  requestQueueCv.notify_all();
}

void ServerConnection::setSubscriptionLifetimeCount(
    const std::string &name, std::uint32_t lifetimeCount) {
  std::lock_guard<std::mutex> lock(mutex);
  subscriptions[name].lifetimeCount = lifetimeCount;
}

void ServerConnection::setSubscriptionMaxKeepAliveCount(
    const std::string &name, std::uint32_t maxKeepAliveCount) {
  std::lock_guard<std::mutex> lock(mutex);
  subscriptions[name].maxKeepAliveCount = maxKeepAliveCount;
}

void ServerConnection::setSubscriptionPublishingInterval(
    const std::string &name, double publishingInterval) {
  std::lock_guard<std::mutex> lock(mutex);
  subscriptions[name].publishingInterval = publishingInterval;
}

void ServerConnection::write(const UaNodeId &nodeId, const UaVariant &value) {
  std::lock_guard<std::mutex> lock(mutex);
  writeInternal(nodeId, value);
}

void ServerConnection::writeAsync(const UaNodeId &nodeId,
    const UaVariant &value, std::shared_ptr<WriteCallback> callback) {
  std::unique_ptr<Request> request(new WriteRequest{callback, nodeId, value});
  {
    std::lock_guard<std::mutex> lock(requestQueueMutex);
    requestQueue.push_back(std::move(request));
  }
  requestQueueCv.notify_all();
}

namespace {

std::vector<char> loadBinaryFile(std::string const &path) {
  std::vector<char> data;
  try {
    std::ifstream stream(path, std::ios::binary);
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    data = std::vector<char>(
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>());
  } catch (...) {
    throw std::runtime_error(
      std::string("Error while trying to read \"") + path + "\".");
  }
  if (!data.size()) {
    throw std::runtime_error(
      std::string("Error while trying to read \"") + path
      + "\": File is empty.");
  }
  return data;
}

} // anonymous namespace

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const std::string &username, const std::string &password,
    bool useAuthentication, SecurityMode securityMode,
    const std::string &clientCertPath, const std::string &clientKeyPath,
    const std::string &serverCertPath, const std::string &applicationUri,
    bool useEncryption) :
    applicationUri(applicationUri), endpointUrl(endpointUrl),
    password(password), securityMode(securityMode), shutdownRequested(false),
    useAuthentication(useAuthentication), useEncryption(useEncryption),
    username(username) {
  // If encryption is enabled, we first have to read the client certificate and
  // key from their respective files. If a server certificate has been
  // specified, we load it as well.
  if (useEncryption) {
#ifdef UA_ENABLE_ENCRYPTION
    this->clientCert = loadBinaryFile(clientCertPath);
    this->clientKey = loadBinaryFile(clientKeyPath);
    if (serverCertPath.length()) {
      this->serverCert = loadBinaryFile(serverCertPath);
    }
#else
    throw std::logic_error(
      "The encryption features are not available because the EPICS device support has been compiled without them. Please set USE_MBEDTLS to YES in configure/CONFIG_SITE.local and recompile the device support to enable them.");
#endif
  }
  // Now we can construct the client.
  this->client = UA_Client_new();
  if (!this->client) {
    throw UaException(UA_STATUSCODE_BADOUTOFMEMORY);
  }
  try {
    configureClient();
    this->connectionThread = std::thread([this]() {runConnectionThread();});
  } catch (...) {
    // When creating the thread fails, we have to delete the client because the
    // destructor is not going to be called when the constructor throws an
    // exception.
    UA_Client_delete(this->client);
    this->client = nullptr;
    throw;
  }
  // Try to establish the connection so that read or write operations can
  // proceed without an unnecessary delay.
  {
    std::lock_guard<std::mutex> lock(mutex);
    connect();
  }
}

void ServerConnection::activateMonitoredItem(Subscription &subscription,
    MonitoredItem &monitoredItem) {
  // This method should only be called for a monitored item that has not been
  // activated yet.
  assert (!monitoredItem.active);
  // UA_MonitoredItemCreateRequest_default does not copy the node ID before
  // putting it inside the UA_MonitoredItemCreateRequest structure. This means
  // that the node ID is going to be deallocated when the request structure is
  // cleared by calling UA_MonitoredItemCreateRequest_clear later in this
  // method. We cannot have this happen, so we have to copy the node ID before
  // passing it to UA_MonitoredItemCreateRequest_default.
  UA_NodeId copiedNodeId;
  UA_NodeId_init(&copiedNodeId);
  UA_NodeId_copy(&monitoredItem.nodeId.get(), &copiedNodeId);
  auto monitoredItemCreateRequest =
    UA_MonitoredItemCreateRequest_default(copiedNodeId);
  // The monitoringMode is set to UA_MONITORINGMODE_REPORTING by default, which
  // is what we need. We configure the other parameters according to what was
  // requested by the user.
  monitoredItemCreateRequest.requestedParameters.discardOldest =
    monitoredItem.discardOldest;
  monitoredItemCreateRequest.requestedParameters.queueSize =
    monitoredItem.queueSize;
  monitoredItemCreateRequest.requestedParameters.samplingInterval =
    monitoredItem.samplingInterval;
  void *context = &monitoredItem;
  UA_Client_DeleteMonitoredItemCallback deleteCallback = nullptr;
  auto monitoredItemCreateResult = UA_Client_MonitoredItems_createDataChange(
    client, subscription.subscriptionId, UA_TIMESTAMPSTORETURN_NEITHER,
    monitoredItemCreateRequest, context,
    monitoredItemDataChangeNotificationCallback, deleteCallback);
  auto status = monitoredItemCreateResult.statusCode;
  auto monitoredItemId = monitoredItemCreateResult.monitoredItemId;
  UA_MonitoredItemCreateRequest_clear(&monitoredItemCreateRequest);
  UA_MonitoredItemCreateResult_clear(&monitoredItemCreateResult);
  if (status == UA_STATUSCODE_GOOD) {
    monitoredItem.monitoredItemId = monitoredItemId;
    monitoredItem.active = true;
  } else {
    if (!maybeResetConnection(status) || !monitoredItem.active) {
      throw UaException(status);
    }
  }
}

void ServerConnection::activateSubscription(Subscription &subscription) {
  assert (!subscription.active);
  auto createSubscriptionRequest = UA_CreateSubscriptionRequest_default();
  createSubscriptionRequest.requestedLifetimeCount = subscription.lifetimeCount;
  createSubscriptionRequest.requestedMaxKeepAliveCount =
    subscription.maxKeepAliveCount;
  createSubscriptionRequest.requestedPublishingInterval =
    subscription.publishingInterval;
  void *context = nullptr;
  auto createSubscriptionResponse = UA_Client_Subscriptions_create(
    client, createSubscriptionRequest, context, nullptr, nullptr);
  auto subscriptionId = createSubscriptionResponse.subscriptionId;
  auto status = createSubscriptionResponse.responseHeader.serviceResult;
  UA_CreateSubscriptionRequest_clear(&createSubscriptionRequest);
  UA_CreateSubscriptionResponse_clear(&createSubscriptionResponse);
  if (status == UA_STATUSCODE_GOOD) {
    subscription.subscriptionId = subscriptionId;
    subscription.active = true;
  } else {
    // For certain errors, we try to reconnect. If the reconnect attempt fails
    // or the subscription cannot not be activated as part of that attempt, we
    // still throw an exception.
    if (!maybeResetConnection(status) || !subscription.active) {
      throw UaException(status);
    }
  }
}

void ServerConnection::addMonitoredItemInternal(
    const std::string &subscriptionName, const UaNodeId &nodeId,
    std::shared_ptr<ServerConnection::MonitoredItemCallback> const &callback,
    double samplingInterval, std::uint32_t queueSize, bool discardOldest) {
  auto &subscription = subscriptions[subscriptionName];
  auto &monitoredItems = subscription.monitoredItems[nodeId];
  // If the specified callback is already registered for the specified
  // subscription and node ID, we discard this request.
  for (auto &monitoredItem : monitoredItems) {
    if (monitoredItem.callback == callback) {
      return;
    }
  }
  // The specified callback does not exist yet for the specified node ID, so we
  // add a monitored item to our internal data structures.
  monitoredItems.emplace_back(
    callback, discardOldest, nodeId, queueSize, samplingInterval);
  auto &monitoredItem = monitoredItems.back();
  // The following actions might result in a UaException (e.g. because the
  // server is offline or does not support a certain option). For this reason,
  // we wrap it in a try-catch block and notifiy the callback of the problem if
  // there is any.
  try {
    // If the subscription has not been created on the server yet, we have to do
    // this first, before we can add the monitored item to it.
    if (!subscription.active) {
      activateSubscription(subscription);
    }
    // Now that we have an active subscription, we can register the monitored
    // item with the server.
    activateMonitoredItem(subscription, monitoredItem);
  } catch (UaException const &e) {
    // We notify the callback that there is a problem.
    try {
      callback->failure(nodeId, e.getStatusCode());
    } catch (...) {
      // We catch all exceptions because an exception in a callback should never
      // stop the connection thread.
      errorExtendedPrintf(
          "Exception from callback caught in connection thread.");
    }
  }
}

void ServerConnection::configureClient() {
  auto config = UA_Client_getConfig(this->client);
  // The useEncryption flag can only be set to true if encryption is enabled at
  // compile time.
  if (useEncryption) {
#ifdef UA_ENABLE_ENCRYPTION
    // When using encryption, we specify a client certificate and (optionally)
    // a server certificate. The byte strings that we create here are only used
    // during the call to the functions and will not be cleared, so it is okay
    // to simply make them reference the data that we store inside this class.
    UA_ByteString clientCertString;
    clientCertString.data = reinterpret_cast<std::uint8_t *>(clientCert.data());
    clientCertString.length = clientCert.size();
    UA_ByteString clientKeyString;
    clientKeyString.data = reinterpret_cast<std::uint8_t *>(clientKey.data());
    clientKeyString.length = clientKey.size();
    // If no server certificate has been specified, we use an empty trust list.
    // This will effectively cause the client to accept any server certificate.
    UA_ByteString serverCertString;
    UA_ByteString const *trustList;
    std::size_t trustListSize;
    if (serverCert.size()) {
      serverCertString.data =
        reinterpret_cast<std::uint8_t *>(serverCert.data());
      serverCertString.length = serverCert.size();
      trustList = &serverCertString;
      trustListSize = 1;
    } else {
      trustList = nullptr;
      trustListSize = 0;
    }
    auto statusCode = UA_ClientConfig_setDefaultEncryption(
      config, clientCertString, clientKeyString, trustList, trustListSize,
      nullptr, 0);
    if (statusCode) {
      throw UaException(statusCode);
    }
    // If a non-empty application URI has been specified. we set it in the
    // client description. This is necessary because servers will typically
    // verify that the application URI specified in the client description
    // matches the application URI specified in the client certificate.
    if (applicationUri.length()) {
      UA_String_clear(&config->clientDescription.applicationUri);
      config->clientDescription.applicationUri =
        UA_STRING_ALLOC(applicationUri.c_str());
    }
    // We also have to set the message security mode so that the client actually
    // uses encryption if desired.
    switch (securityMode) {
    case SecurityMode::invalid:
      config->securityMode = UA_MESSAGESECURITYMODE_INVALID;
      break;
    case SecurityMode::none:
      config->securityMode = UA_MESSAGESECURITYMODE_NONE;
      break;
    case SecurityMode::sign:
      config->securityMode = UA_MESSAGESECURITYMODE_SIGN;
      break;
    case SecurityMode::signAndEncrypt:
      config->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
      break;
    }
#endif // UA_ENABLE_ENCRYPTION
  } else {
    auto statusCode = UA_ClientConfig_setDefault(config);
    if (statusCode) {
      throw UaException(statusCode);
    }
  }
}

bool ServerConnection::connect() {
  UA_StatusCode status;
  if (useAuthentication) {
    status = UA_Client_connectUsername(client, endpointUrl.c_str(),
        username.c_str(), password.c_str());
  } else {
    status = UA_Client_connect(client, endpointUrl.c_str());
  }
  if (status == UA_STATUSCODE_GOOD) {
    // When the connection has been (re-)established, we also want to reactivate
    // all monitored items.
    for (auto &subscriptionEntry : subscriptions) {
      auto &subscription = subscriptionEntry.second;
      // We do not activate empty subscriptions. They are going to be activated
      // when the first monitored item is added to them.
      if (subscription.monitoredItems.empty()) {
        continue;
      }
      // Problems when activating a subscription should not keep us from trying
      // to activate other subscriptions.
      try  {
        activateSubscription(subscription);
        for (auto &monitoredItemsEntry : subscription.monitoredItems) {
          auto &monitoredItems = monitoredItemsEntry.second;
          for (auto &monitoredItem : monitoredItems) {
            // Problems when activating a monitored item should not keep us from
            // trying to activate other monitored items.
            try {
              activateMonitoredItem(subscription, monitoredItem);
            } catch (UaException &e) {
              try {
                monitoredItem.callback->failure(
                  monitoredItem.nodeId, e.getStatusCode());
              } catch (...) {
                // We catch all exceptions because an exception in a callback
                // should never stop the connection thread.
                errorExtendedPrintf(
                    "Exception from callback caught in connection thread.");
              }
            }
          }
        }
      } catch (UaException &e) {
        // We notify the callbacks of the monitored items that the subscription
        // could not be registered.
        for (auto &monitoredItemsEntry : subscription.monitoredItems) {
          auto &monitoredItems = monitoredItemsEntry.second;
          for (auto &monitoredItem : monitoredItems) {
            try {
              monitoredItem.callback->failure(
                monitoredItem.nodeId, e.getStatusCode());
            } catch (...) {
              // We catch all exceptions because an exception in a callback
              // should never stop the connection thread.
              errorExtendedPrintf(
                  "Exception from callback caught in connection thread.");
            }
          }
        }
      }
    }
    return true;
  } else {
    errorExtendedPrintf("Could not connect to OPC UA server: %s",
        UA_StatusCode_name(status));
    return false;
  }
}

void ServerConnection::deactivateMonitoredItem(Subscription &subscription,
    MonitoredItem &monitoredItem) {
  // This method should only be called if the monitored item is active.
  assert(monitoredItem.active);
  // We do not check the status code returned by the function call. If there is
  // an error, there is nothing that we can reasonably do anyway.
  UA_Client_MonitoredItems_deleteSingle(
    client, subscription.subscriptionId, monitoredItem.monitoredItemId);
  monitoredItem.active = false;
}

void ServerConnection::deactivateSubscription(Subscription &subscription) {
  // This method should only be called if the subscription is active.
  assert(subscription.active);
  // We do not check the status code returned by the function call. If there is
  // an error, there is nothing that we can reasonably do anyway.
  UA_Client_Subscriptions_deleteSingle(client, subscription.subscriptionId);
  subscription.active = false;
}

bool ServerConnection::maybeResetConnection(UA_StatusCode statusCode) {
  // We only try to reset the connection for specific status codes. For other
  // status codes resetting the connection is most likely not going to help
  // anyway.
  switch (statusCode) {
  case UA_STATUSCODE_BADCOMMUNICATIONERROR:
  case UA_STATUSCODE_BADCONNECTIONCLOSED:
  case UA_STATUSCODE_BADSERVERNOTCONNECTED:
  case UA_STATUSCODE_BADSESSIONIDINVALID:
    break;
  default:
    return false;
  }
  // In case of an error, disconnecting the client might be sufficient, but
  // there might also be cases where creating a fresh client is beneficial. We
  // do not know which case we have, so we always destroy the whole client.
  // When the client is destroyed, it is disconnected automatically.
  // In the past, we simply reset the client, but recent versions of the
  // open62541 library do not expose this API any longer. In order to still be
  // able to assume in other parts of the code that there always is a client
  // object, we first allocate the new client and only destroy the old client
  // if the allocation was successful. In the unlikely event that the
  // allocation fails, we continue using the old client object.
  UA_Client *newClient = UA_Client_new();
  if (newClient) {
    UA_Client_delete(client);
    client = newClient;
    newClient = nullptr;
    configureClient();
  }
  // We also have to reset the status of all subscriptions and monitored items.
  for (auto &subscriptionEntry : subscriptions) {
    auto &subscription = subscriptionEntry.second;
    subscription.active = false;
    for (auto &monitoredItemsEntry : subscription.monitoredItems) {
      auto &monitoredItems = monitoredItemsEntry.second;
      for (auto &monitoredItem : monitoredItems) {
        // If the monitored item was active before, we call the failure callback
        // so that the watching code knows that monitoring notifications are not
        // going to be received from now on.
        if (monitoredItem.active) {
          monitoredItem.active = false;
          try {
            monitoredItem.callback->failure(monitoredItem.nodeId, statusCode);
          } catch (...) {
            // We catch all exceptions because an exception in a callback should
            // never stop the connection thread.
            errorExtendedPrintf(
                "Exception from callback caught in connection thread.");
          }
        }
      }
    }
  }
  return connect();
}

UaVariant ServerConnection::readInternal(const UaNodeId &nodeId) {
  UA_StatusCode status;
  UA_Variant targetValue;
  UA_Variant_init(&targetValue);
  status = UA_Client_readValueAttribute(client, nodeId.get(), &targetValue);
  if (status == UA_STATUSCODE_GOOD) {
    return UaVariant(std::move(targetValue));
  } else {
    if (maybeResetConnection(status)) {
      status = UA_Client_readValueAttribute(client, nodeId.get(), &targetValue);
      if (status == UA_STATUSCODE_GOOD) {
        return UaVariant(std::move(targetValue));
      } else {
        UA_Variant_clear(&targetValue);
        throw UaException(status);
      }
    } else {
      UA_Variant_clear(&targetValue);
      throw UaException(status);
    }
  }
}

void ServerConnection::removeMonitoredItemInternal(
    const std::string &subscriptionName, const UaNodeId &nodeId,
    std::shared_ptr<ServerConnection::MonitoredItemCallback> const &callback) {
  // If the specified subscription does not exist, we are done.
  auto subscriptionIterator = subscriptions.find(subscriptionName);
  if (subscriptionIterator == subscriptions.end()) {
    return;
  }
  auto &subscription = subscriptionIterator->second;
  // If there are no monitored items for the specified node ID, we are done.
  auto monitoredItemsIterator = subscription.monitoredItems.find(nodeId);
  if (monitoredItemsIterator == subscription.monitoredItems.end()) {
    return;
  }
  auto &monitoredItems = monitoredItemsIterator->second;
  // There might be multiple registrations for the same node ID, so we iterate
  // over all items until we find the one that matches the callback.
  for (auto monitoredItemIterator = monitoredItems.begin();
      monitoredItemIterator != monitoredItems.end(); ++monitoredItemsIterator) {
    auto &monitoredItem = *monitoredItemIterator;
    if (monitoredItem.callback == callback) {
      if (monitoredItem.active) {
        deactivateMonitoredItem(subscription, monitoredItem);
      }
      monitoredItems.erase(monitoredItemIterator);
      break;
    }
  }
  // If this was the last monitored item for the specified node ID, we remove
  // the entire entry for the node ID from the subscription.
  if (monitoredItems.empty()) {
    subscription.monitoredItems.erase(monitoredItemsIterator);
  }
  // If this was the last node ID for the specified subscription, we deactivate
  // the subscription.
  if (subscription.monitoredItems.empty()) {
    deactivateSubscription(subscription);
  }
}

void ServerConnection::runConnectionThread() {
  while (!shutdownRequested.load(std::memory_order_acquire)) {
    {
      // On each iteration, we have the client do some background activity, like
      // processing notifications and calling callbacks. We need to hold the
      // mutex while doing this because no other calls should be made to the
      // client while doing this and our callbacks also depend on the mutex
      // being held.
      std::lock_guard<std::mutex> lock(mutex);
      auto status = UA_Client_run_iterate(client, 0);
      // If there is an error, we reset the connection for certain status codes.
      if (status != UA_STATUSCODE_GOOD) {
        try {
          maybeResetConnection(status);
        } catch (UaException const &e) {
          // There are rare cases in which the maybeResetConnection method might
          // throw an exception. In these cases, we log the exception and
          // continue. We log the exception because otherwise there might be no
          // way for the user to detect the underlying problem (the user would
          // only see errors because the client is disconnected).
          errorExtendedPrintf("Could not configure the OPC UA client: %s",
            UA_StatusCode_name(e.getStatusCode()));
        }
      }
    }
    // We need to hold a lock on the mutex protecting access to the request
    // queue while trying to retrieve the next request.
    std::unique_ptr<Request> request;
    {
      std::unique_lock<std::mutex> requestQueueLock(requestQueueMutex);
      if (requestQueue.empty()) {
        // If the request queue is empty, we sleep for one millisecond. After that
        // time, we wake up in order to have the client process background
        // activity (like processing notifications that might have arrived).
        requestQueueCv.wait_for(requestQueueLock, std::chrono::milliseconds(1));
        continue;
      }
      request = std::move(requestQueue.front());
      requestQueue.pop_front();
    }
    // While processing the requests, we have to hold a lock on the general
    // mutex.
    std::lock_guard<std::mutex> lock(mutex);
    switch (request->type) {
    case RequestType::addMonitoredItem: {
      AddMonitoredItemRequest &addMonitoredItemRequest =
        *(dynamic_cast<AddMonitoredItemRequest *>(request.get()));
      addMonitoredItemInternal(
        addMonitoredItemRequest.subscription,
        addMonitoredItemRequest.nodeId,
        addMonitoredItemRequest.callback,
        addMonitoredItemRequest.samplingInterval,
        addMonitoredItemRequest.queueSize,
        addMonitoredItemRequest.discardOldest);
      break;
    }
    case RequestType::read: {
      ReadRequest &readRequest = *(dynamic_cast<ReadRequest *>(
        request.get()));
      UA_StatusCode status;
      UaVariant value;
      try {
        value = readInternal(readRequest.nodeId);
        status = UA_STATUSCODE_GOOD;
      } catch (UaException const &e) {
        status = e.getStatusCode();
      }
      try {
        if (status == UA_STATUSCODE_GOOD) {
          readRequest.callback->success(readRequest.nodeId.get(), value.get());
        } else {
          readRequest.callback->failure(readRequest.nodeId.get(), status);
        }
      } catch (...) {
        // We catch all exceptions because an exception in a callback should
        // never stop the connection thread.
        errorExtendedPrintf(
            "Exception from callback caught in connection thread.");
      }
      break;
    }
    case RequestType::removeMonitoredItem: {
      RemoveMonitoredItemRequest &removeMonitoredItemRequest =
        *(dynamic_cast<RemoveMonitoredItemRequest *>(request.get()));
      removeMonitoredItemInternal(
        removeMonitoredItemRequest.subscription,
        removeMonitoredItemRequest.nodeId,
        removeMonitoredItemRequest.callback);
      break;
    }
    case RequestType::write: {
      WriteRequest &writeRequest = *(dynamic_cast<WriteRequest *>(
        request.get()));
      UA_StatusCode status;
      try {
        writeInternal(writeRequest.nodeId, writeRequest.value);
        status = UA_STATUSCODE_GOOD;
      } catch (UaException const &e) {
        status = e.getStatusCode();
      }
      try {
        if (status == UA_STATUSCODE_GOOD) {
          writeRequest.callback->success(writeRequest.nodeId.get());
        } else {
          writeRequest.callback->failure(writeRequest.nodeId.get(), status);
        }
      } catch (...) {
        // We catch all exceptions because an exception in a callback should
        // never stop the connection thread.
        errorExtendedPrintf(
            "Exception from callback caught in connection thread.");
      }
      break;
    }
    }
  }
}

void ServerConnection::writeInternal(const UaNodeId &nodeId,
    const UaVariant &value) {
  UA_StatusCode status;
  status = UA_Client_writeValueAttribute(client, nodeId.get(), &value.get());
  if (status != UA_STATUSCODE_GOOD) {
    if (maybeResetConnection(status)) {
      status = UA_Client_writeValueAttribute(client, nodeId.get(), &value.get());
      if (status != UA_STATUSCODE_GOOD) {
        throw UaException(status);
      }
    } else {
      throw UaException(status);
    }
  }
}

void ServerConnection::monitoredItemDataChangeNotificationCallback(
    UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext,
    UA_UInt32 monitoredItemId, void *monitoredItemContext,
    UA_DataValue *value) {
  MonitoredItem *monitoredItem =
    static_cast<MonitoredItem *>(monitoredItemContext);
  if (value->hasValue) {
    monitoredItem->callback->success(
      monitoredItem->nodeId, std::move(value->value));
  }
  if (value->hasStatus && value->status != UA_STATUSCODE_GOOD) {
    monitoredItem->callback->failure(monitoredItem->nodeId, value->status);
  }
}

}
}
