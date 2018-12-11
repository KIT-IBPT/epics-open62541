/*
 * Copyright 2017-2018 aquenos GmbH.
 * Copyright 2017-2018 Karlsruhe Institute of Technology.
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

#include "open62541Error.h"
#include "UaException.h"

#include "ServerConnection.h"

namespace open62541 {
namespace epics {

ServerConnection::ServerConnection(const std::string &endpointUrl) :
    ServerConnection(endpointUrl, UA_ClientConfig_default) {
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const UA_ClientConfig &config) :
    ServerConnection(endpointUrl, config, std::string(), std::string(), false) {
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const std::string &username, const std::string &password) :
    ServerConnection(endpointUrl, UA_ClientConfig_default, username, password) {
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const UA_ClientConfig &config, const std::string &username,
    const std::string &password) :
    ServerConnection(endpointUrl, config, username, password, true) {
}

ServerConnection::~ServerConnection() {
  // Tell the connection thread to shutdown.
  shutdownRequested.store(true, std::memory_order_release);
  connectionThreadCv.notify_all();
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

void ServerConnection::read(const UA_NodeId &nodeId, UA_Variant &targetValue) {
  std::lock_guard<std::mutex> lock(mutex);
  UA_StatusCode status = readInternal(nodeId, targetValue);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
}

void ServerConnection::readAsync(const UA_NodeId &nodeId,
    std::shared_ptr<ReadCallback> callback) {
  Request request = Request::readRequest(nodeId, callback);
  {
    std::lock_guard<std::mutex> lock(mutex);
    requestQueue.push_back(request);
  }
  connectionThreadCv.notify_all();
}

void ServerConnection::write(const UA_NodeId &nodeId, const UA_Variant &value) {
  std::lock_guard<std::mutex> lock(mutex);
  UA_StatusCode status = writeInternal(nodeId, value);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
}

void ServerConnection::writeAsync(const UA_NodeId &nodeId,
    const UA_Variant &value, std::shared_ptr<WriteCallback> callback) {
  Request request = Request::writeRequest(nodeId, value, callback);
  {
    std::lock_guard<std::mutex> lock(mutex);
    requestQueue.push_back(request);
  }
  connectionThreadCv.notify_all();
}

ServerConnection::Request ServerConnection::Request::readRequest(
    const UA_NodeId &nodeId, const std::shared_ptr<ReadCallback> &callback) {
  // For a read request, the variant is never used, but we still initialize it
  // so that it is correctly identified as an empty variant in the requests's
  // destructor.
  UA_Variant v;
  UA_Variant_init(&v);
  return Request(RequestType::read, nodeId, v, callback,
      std::shared_ptr<WriteCallback>());
}

ServerConnection::Request ServerConnection::Request::writeRequest(
    const UA_NodeId &nodeId, const UA_Variant &value,
    const std::shared_ptr<WriteCallback> &callback) {
  return Request(RequestType::write, nodeId, value,
      std::shared_ptr<ReadCallback>(), callback);
}

ServerConnection::Request::Request(RequestType type, const UA_NodeId &nodeId,
    const UA_Variant &value, const std::shared_ptr<ReadCallback> &readCallback,
    const std::shared_ptr<WriteCallback> &writeCallback) :
    type(type), readCallback(readCallback), writeCallback(writeCallback) {
  UA_StatusCode status;
  status = UA_NodeId_copy(&nodeId, &this->nodeId);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
  status = UA_Variant_copy(&value, &this->value);
  if (status != UA_STATUSCODE_GOOD) {
    // The destructor is not going to be called when we throw an exception, so
    // we have to ensure that members of the copied node ID are freed.
    UA_NodeId_deleteMembers(&this->nodeId);
    throw UaException(status);
  }
}

ServerConnection::Request::Request(const Request &request) {
  // We have to initialize the nodeId and value so that when calling the delete
  // functions they do not work on uninitialized data.
  UA_NodeId_init(&this->nodeId);
  UA_Variant_init(&this->value);
  *this = request;
}

ServerConnection::Request::Request(Request &&request) {
  // We have to initialize the nodeId and value so that when calling the delete
  // functions they do not work on uninitialized data.
  UA_NodeId_init(&this->nodeId);
  UA_Variant_init(&this->value);
  *this = request;
}

ServerConnection::Request::~Request() {
  UA_NodeId_deleteMembers(&this->nodeId);
  UA_Variant_deleteMembers(&this->value);
}

ServerConnection::Request &ServerConnection::Request::operator=(const Request &request) {
  if (&request == this) {
    return *this;
  }
  UA_NodeId tempNodeId;
  UA_Variant tempValue;
  UA_StatusCode status;
  status = UA_NodeId_copy(&request.nodeId, &tempNodeId);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
  status = UA_Variant_copy(&request.value, &tempValue);
  if (status != UA_STATUSCODE_GOOD) {
    UA_NodeId_deleteMembers(&tempNodeId);
    throw UaException(status);
  }
  // We have to free the members of the old values before initializing them with
  // new ones.
  UA_NodeId_deleteMembers(&this->nodeId);
  UA_Variant_deleteMembers(&this->value);
  this->nodeId = tempNodeId;
  this->value = tempValue;
  this->type = request.type;
  this->readCallback = request.readCallback;
  this->writeCallback = request.writeCallback;
  return *this;
}

ServerConnection::Request &ServerConnection::Request::operator=(Request &&request) {
  if (&request == this) {
    return *this;
  }
  // We have to free the members of the old values before initializing them with
  // new ones.
  UA_NodeId_deleteMembers(&this->nodeId);
  UA_Variant_deleteMembers(&this->value);
  this->nodeId = request.nodeId;
  this->value = request.value;
  // We have to reset the original value and node ID so that their members are
  // not freed when the original request is destroyed.
  UA_NodeId_init(&request.nodeId);
  UA_Variant_init(&request.value);
  this->type = request.type;
  this->readCallback = request.readCallback;
  this->writeCallback = request.writeCallback;
  return *this;
}

ServerConnection::ServerConnection(const std::string &endpointUrl,
    const UA_ClientConfig &config, const std::string &username,
    const std::string &password, bool useAuthentication) :
    config(config), endpointUrl(endpointUrl), username(username), password(
        password), useAuthentication(useAuthentication), shutdownRequested(
        false) {
  this->client = UA_Client_new(this->config);
  if (this->client == nullptr) {
    throw UaException(UA_STATUSCODE_BADOUTOFMEMORY);
  }
  try {
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

bool ServerConnection::connect() {
  UA_StatusCode status;
  if (useAuthentication) {
    status = UA_Client_connect_username(client, endpointUrl.c_str(),
        username.c_str(), password.c_str());
  } else {
    status = UA_Client_connect(client, endpointUrl.c_str());
  }
  if (status == UA_STATUSCODE_GOOD) {
    return true;
  } else {
    errorExtendedPrintf("Could not connect to OPC UA server: %s",
        UA_StatusCode_name(status));
    return false;
  }
}

void ServerConnection::runConnectionThread() {
  while (!shutdownRequested.load(std::memory_order_acquire)) {
    std::unique_lock<std::mutex> lock(mutex);
    if (requestQueue.empty()) {
      connectionThreadCv.wait(lock);
      continue;
    }
    Request request = requestQueue.front();
    requestQueue.pop_front();
    switch (request.getType()) {
    case RequestType::read: {
      UA_StatusCode status;
      UA_Variant value;
      UA_Variant_init(&value);
      status = readInternal(request.getNodeId(), value);
      try {
        if (status == UA_STATUSCODE_GOOD) {
          request.getReadCallback().success(request.getNodeId(), value);
        } else {
          request.getReadCallback().failure(request.getNodeId(), status);
        }
      } catch (...) {
        // We catch all exceptions because an exception in a callback should
        // never stop the connection thread.
        errorExtendedPrintf(
            "Exception from callback caught in connection thread.");
      }
      UA_Variant_deleteMembers(&value);
      break;
    }
    case RequestType::write: {
      UA_StatusCode status;
      status = writeInternal(request.getNodeId(), request.getValue());
      try {
        if (status == UA_STATUSCODE_GOOD) {
          request.getWriteCallback().success(request.getNodeId());
        } else {
          request.getWriteCallback().failure(request.getNodeId(), status);
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

UA_StatusCode ServerConnection::readInternal(const UA_NodeId &nodeId,
    UA_Variant &targetValue) {
  UA_StatusCode status;
  status = UA_Client_readValueAttribute(client, nodeId, &targetValue);
  switch (status) {
  case UA_STATUSCODE_BADCOMMUNICATIONERROR:
  case UA_STATUSCODE_BADCONNECTIONCLOSED:
  case UA_STATUSCODE_BADSERVERNOTCONNECTED:
  case UA_STATUSCODE_BADSESSIONIDINVALID:
    // We do not check the result of UA_Client_disconnect on purpose: Even if
    // there was an error, the next logical step would be resetting the client.
    UA_Client_disconnect(client);
    UA_Client_reset(client);
    if (connect()) {
      status = UA_Client_readValueAttribute(client, nodeId, &targetValue);
    } else {
      return status;
    }
    break;
  default:
    // We proceed normally for all other status codes.
    break;
  }
  return status;
}

UA_StatusCode ServerConnection::writeInternal(const UA_NodeId &nodeId,
    const UA_Variant &value) {
  UA_StatusCode status;
  status = UA_Client_writeValueAttribute(client, nodeId, &value);
  switch (status) {
  case UA_STATUSCODE_BADCOMMUNICATIONERROR:
  case UA_STATUSCODE_BADCONNECTIONCLOSED:
  case UA_STATUSCODE_BADSERVERNOTCONNECTED:
  case UA_STATUSCODE_BADSESSIONIDINVALID:
    // We do not check the result of UA_Client_disconnect on purpose: Even if
    // there was an error, the next logical step would be resetting the client.
    UA_Client_disconnect(client);
    UA_Client_reset(client);
    if (connect()) {
      status = UA_Client_writeValueAttribute(client, nodeId, &value);
    } else {
      return status;
    }
    break;
  default:
    // We proceed normally for all other status codes.
    break;
  }
  return status;
}

}
}
