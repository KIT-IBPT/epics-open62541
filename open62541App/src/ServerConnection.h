/*
 * Copyright 2017-2019 aquenos GmbH.
 * Copyright 2017-2019 Karlsruhe Institute of Technology.
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

#ifndef OPEN62541_EPICS_SERVER_CONNECTION_H
#define OPEN62541_EPICS_SERVER_CONNECTION_H

// There is a bug in the C++ standard library of certain versions of the macOS
// SDK that causes a problem when including <condition_variable>. The workaround
// for this is defining the _DARWIN_C_SOURCE preprocessor macro.
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "UaNodeId.h"
#include "UaVariant.h"

namespace open62541 {
namespace epics {

/**
 * Connection to an OPC UA server. This class hides the details of how the
 * connection is established (and reestablished if needed) and implements the
 * I/O logic. Its methods are safe for concurrent use by multiple threads.
 */
class ServerConnection {

public:

  /**
   * Interface for a read callback. Read callbacks allow reading from a node in
   * an asynchronous way, so that the calling code does not have to wait until
   * the operation finishes.
   */
  class ReadCallback {

  public:

    /**
     * Called when the operation succeeds. The node ID passed is the node ID
     * specified in the read request. The value passed is the value read from
     * the server.
     */
    virtual void success(const UaNodeId &nodeId, const UaVariant &value) = 0;

    /**
     * Called when a read operation fails. The node ID passed is the node ID
     * specified in the read request. The status code  gives information about
     * the cause of the failure.
     */
    virtual void failure(const UaNodeId &nodeId, UA_StatusCode statusCode) =0;

    /**
     * Default constructor.
     */
    ReadCallback() {
    }

    /**
     * Destructor. Virtual classes should have a virtual destructor.
     */
    virtual ~ReadCallback() {
    }

    // We do not want to allow copy or move construction or assignment.
    ReadCallback(const ReadCallback &) = delete;
    ReadCallback(ReadCallback &&) = delete;
    ReadCallback &operator=(const ReadCallback &) = delete;
    ReadCallback &operator=(ReadCallback &&) = delete;

  };

  /**
   * Interface for a monitored item callback. Monitored utem callbacks are
   * called when a notification about a data change is received for a monitored
   * item.
   */
  class MonitoredItemCallback {

  public:

    /**
     * Called when a successful notification is received. The node ID passed is
     * the node ID specified in the read request. The value passed is the value
     * received from the server.
     */
    virtual void success(const UaNodeId &nodeId, const UaVariant &value) = 0;

    /**
     * Called when there is a problem with the subscription (e.g. the connection
     * is interrupted or the subscription cannot be registered with the server).
     */
    virtual void failure(const UaNodeId &nodeId, UA_StatusCode statusCode) =0;

    /**
     * Default constructor.
     */
    MonitoredItemCallback() {
    }

    /**
     * Destructor. Virtual classes should have a virtual destructor.
     */
    virtual ~MonitoredItemCallback() {
    }

    // We do not want to allow copy or move construction or assignment.
    MonitoredItemCallback(const MonitoredItemCallback &) = delete;
    MonitoredItemCallback(MonitoredItemCallback &&) = delete;
    MonitoredItemCallback &operator=(const MonitoredItemCallback &) = delete;
    MonitoredItemCallback &operator=(MonitoredItemCallback &&) = delete;

  };

  /**
   * Interface for a write callback. Write callbacks allow writing to a node
   * in an asynchronous way, so that the calling code does not have to wait
   * until the operation finishes.
   */
  class WriteCallback {

  public:

    /**
     * Called when the operation succeeds. The node ID passed is the node ID
     * specified in the write request.
     */
    virtual void success(const UaNodeId &nodeId) = 0;

    /**
     * Called when a write operation fails. The node ID passed is the node ID
     * specified in the read request. The status code  gives information about
     * the cause of the failure.
     */
    virtual void failure(const UaNodeId &nodeId, UA_StatusCode statusCode) =0;

    /**
     * Default constructor.
     */
    WriteCallback() {
    }

    /**
     * Destructor. Virtual classes should have a virtual destructor.
     */
    virtual ~WriteCallback() {
    }

    // We do not want to allow copy or move construction or assignment.
    WriteCallback(const WriteCallback &) = delete;
    WriteCallback(WriteCallback &&) = delete;
    WriteCallback &operator=(const WriteCallback &) = delete;
    WriteCallback &operator=(WriteCallback &&) = delete;

  };

  /**
   * Create a server connection for the specified endpoint.
   */
  ServerConnection(const std::string &endpointUrl);

  /**
   * Create a server connection for the specified endpoint, using the
   * specified credentials for authentication.
   */
  ServerConnection(const std::string &endpointUrl, const std::string &username,
      const std::string &password);

  /**
   * Destructor.
   */
  ~ServerConnection();

  /**
   * Registers a monitored item with this server connection.
   *
   * This registers a monitor for the node specified through the node ID. The is
   * going to send a notification when the value of the node changes and the
   * specified callback is going to be called.
   *
   * The subscription name identifies the subscription that is used for the
   * monitored item. Many monitored items can share the same subscription. If no
   * subscription with the specified name exists yet, it is automatically
   * created.
   *
   * The sampling interval specifies how often the server should check for a new
   * value and add it to the queue of notifications. The queue size specified the
   * requested size of this queue. The discard oldest flag defines whether in
   * case this queue is full the oldest (true) or the newest (false) value
   * should be discarded.
   *
   * Please note that the sampling interval and the queue size are just
   * suggestions to the server. The server may still choose different setitngs.
   * Even if the sampling interval is very short, notifications will only be
   * sent according to the publishing interval of the associated subscription.
   */
  void addMonitoredItem(const std::string &subscriptionName,
      const UaNodeId &nodeId,
      std::shared_ptr<MonitoredItemCallback> const &callback,
      double samplingInterval, std::uint32_t queueSize, bool discardOldest);

  /**
   * Returns the lifetime count for the specified subscription.
   *
   * The lifetime count defines how many publishing intervals may pass without
   * receiving a publishing request from the client before the server considers
   * the client inactive and removes the subscription.
   *
   * If the lifetime count has not been set explicitly, the default value
   * (10000) is returned.
   */
  std::uint32_t getSubscriptionLifetimeCount(const std::string &name);

  /**
   * Returns the max. keep-alive count for the specified subscription.
   *
   * The max. keep-alive count defines how many publishing intervals may pass
   * without any notifications being sent to the client. If this number of
   * intervals is exceeded, the server sends an empty notification to the client
   * in order to let it know that the server is still active.
   *
   * If the max. keep-alive count has not been set explicitly, the default value
   * (10) is returned.
   */
  std::uint32_t getSubscriptionMaxKeepAliveCount(const std::string &name);

  /**
   * Returns the publishing interval for the specified subscription.
   *
   * The publishing interval defines the time (in milliseconds) that the server
   * waits between checking whether there are any queued notifications for a
   * subscription.
   *
   * If the publishing interval has not been set explicitly, the default value
   * (500 ms) is returned.
   */
  double getSubscriptionPublishingInterval(const std::string &name);

  /**
   * Reads a node's value. Throws an UaException if there is a problem.
   */
  UaVariant read(const UaNodeId &nodeId);

  /**
   * Reads a node's value asynchronously. When the operation completes, the
   * passed callback is called. This method internally makes a copy of the node
   * ID so that the passed node ID does not have to be kept alive after this
   * method returns.
   */
  void readAsync(const UaNodeId &nodeId,
      std::shared_ptr<ReadCallback> callback);

  /**
   * Unregisters a monitored item from this server connection.
   *
   * If the specified callback has not been previously registered for the
   * specified subscription and node ID, this method does nothing.
   */
  void removeMonitoredItem(const std::string &subscriptionName,
      const UaNodeId &nodeId,
      std::shared_ptr<MonitoredItemCallback> const &callback);

  /**
   * Sets the lifetime count for the specified subscription.
   *
   * The lifetime count defines how many publishing intervals may pass without
   * receiving a publishing request from the client before the server considers
   * the client inactive and removes the subscription.
   *
   * In most cases, the default value will be fine and there will be no reason
   * to change this setting.
   *
   * Making changes to this setting has no effect when the corresponding
   * subscription has already been created.
   */
  void setSubscriptionLifetimeCount(const std::string &name,
      std::uint32_t lifetimeCount);

  /**
   * Sets the max. keep-alive count for the specified subscription.
   *
   * The max. keep-alive count defines how many publishing intervals may pass
   * without any notifications being sent to the client. If this number of
   * intervals is exceeded, the server sends an empty notification to the client
   * in order to let it know that the server is still active.
   *
   * In most cases, the default value will be fine and there will be no reason
   * to change this setting.
   *
   * Making changes to this setting has no effect when the corresponding
   * subscription has already been created.
   */
  void setSubscriptionMaxKeepAliveCount(const std::string &name,
      std::uint32_t maxKeepAliveCount);

  /**
   * Sets the publishing interface for the specified subscription.
   *
   * The publishing interval defines the time (in milliseconds) that the server
   * waits between checking whether there are any queued notifications for a
   * subscription.
   *
   * Making changes to this setting has no effect when the corresponding
   * subscription has already been created.
   */
  void setSubscriptionPublishingInterval(const std::string &name,
      double publishingInterval);

  /**
   * Writes to a node's value. Throws an UaException if there is a problem.
   */
  void write(const UaNodeId &nodeId, const UaVariant &value);

  /**
   * Writes a node's value asynchronously. When the operation completes, the
   * passed callback is called. This method internally makes a copy of the node
   * ID and value so that the passed node ID and value do not have to be kept
   * alive after this method returns.
   */
  void writeAsync(const UaNodeId &nodeId, const UaVariant &value,
      std::shared_ptr<WriteCallback> callback);

private:

  struct MonitoredItem {

    bool active = false;
    std::shared_ptr<MonitoredItemCallback> callback;
    bool discardOldest;
    std::uint32_t monitoredItemId;
    UaNodeId nodeId;
    std::uint32_t queueSize;
    double samplingInterval;

    inline MonitoredItem(std::shared_ptr<MonitoredItemCallback> const &callback,
        bool discardOldest, UaNodeId const &nodeId, std::uint32_t queueSize,
        double samplingInterval) : callback(callback),
        discardOldest(discardOldest), nodeId(nodeId), queueSize(queueSize),
        samplingInterval(samplingInterval) {
    }

  };

  // Using std::variant would be much more elegant than using a common base
  // class and std::unique_ptr, but we want to avoid a dependency on C++ 17.
  enum class RequestType {
    addMonitoredItem, read, removeMonitoredItem, write
  };

  struct Request {

    RequestType type;

    Request(RequestType type) : type(type) {}

    // We want the base class to be virtual so that we can use dynamic_cast.
    // The easiest way for getting this is making the destructor virtual.
    virtual ~Request() noexcept {}

  };

  struct AddMonitoredItemRequest : Request {

    std::shared_ptr<MonitoredItemCallback> callback;
    bool discardOldest;
    UaNodeId nodeId;
    std::uint32_t queueSize;
    double samplingInterval;
    std::string subscription;

    inline AddMonitoredItemRequest(
        std::shared_ptr<MonitoredItemCallback> const &callback,
        bool discardOldest, UaNodeId const &nodeId, std::uint32_t queueSize,
        double samplingInterval, std::string const &subscription)
        : Request(RequestType::addMonitoredItem), callback(callback),
        discardOldest(discardOldest), nodeId(nodeId), queueSize(queueSize),
        samplingInterval(samplingInterval), subscription(subscription) {
      if (!callback) {
        throw std::invalid_argument("The callback must not be null.");
      }
    }

  };

  struct ReadRequest : Request {

    std::shared_ptr<ReadCallback> callback;
    UaNodeId nodeId;

    inline ReadRequest(std::shared_ptr<ReadCallback> const &callback,
        UaNodeId const &nodeId) : Request(RequestType::read),
        callback(callback), nodeId(nodeId) {
      if (!callback) {
        throw std::invalid_argument("The callback must not be null.");
      }
    }

  };

  struct RemoveMonitoredItemRequest : Request {

    std::shared_ptr<MonitoredItemCallback> callback;
    UaNodeId nodeId;
    std::string subscription;

    inline RemoveMonitoredItemRequest(
        std::shared_ptr<MonitoredItemCallback> const &callback,
        UaNodeId const &nodeId, std::string const &subscription)
        : Request(RequestType::removeMonitoredItem), callback(callback),
        nodeId(nodeId), subscription(subscription) {
      if (!callback) {
        throw std::invalid_argument("The callback must not be null.");
      }
    }

  };

  struct WriteRequest : Request {


    std::shared_ptr<WriteCallback> callback;
    UaNodeId nodeId;
    UaVariant value;
    inline WriteRequest(std::shared_ptr<WriteCallback> const &callback,
        UaNodeId const &nodeId, UaVariant const &value)
        : Request(RequestType::write), callback(callback), nodeId(nodeId) {
      if (!callback) {
        throw std::invalid_argument("The callback must not be null.");
      }
    }

  };

  struct Subscription {

    bool active = false;
    std::uint32_t lifetimeCount = 10000;
    std::uint32_t maxKeepAliveCount = 10;
    std::unordered_map<UaNodeId, std::vector<MonitoredItem>> monitoredItems;
    double publishingInterval = 500.0;
    std::uint32_t subscriptionId;

  };

  std::string endpointUrl;
  std::string username;
  std::string password;
  bool useAuthentication;

  std::mutex mutex;
  std::thread connectionThread;
  std::atomic<bool> shutdownRequested;
  std::unordered_map<std::string, Subscription> subscriptions;
  std::list<std::unique_ptr<Request>> requestQueue;
  std::condition_variable requestQueueCv;
  std::mutex requestQueueMutex;
  UA_Client *client;

  // We do not want to allow copy or move construction or assignment.
  ServerConnection(const ServerConnection &) = delete;
  ServerConnection(ServerConnection &&) = delete;
  ServerConnection &operator=(const ServerConnection &) = delete;
  ServerConnection &operator=(ServerConnection &&) = delete;

  ServerConnection(const std::string &endpointUrl,
      const std::string &username, const std::string &password,
      bool useAuthentication);

  void activateMonitoredItem(Subscription &subscription,
      MonitoredItem &monitoredItem);
  void activateSubscription(Subscription &subscription);
  void addMonitoredItemInternal(const std::string &subscriptionName,
      const UaNodeId &nodeId,
      std::shared_ptr<MonitoredItemCallback> const &callback,
      double samplingInterval, std::uint32_t queueSize, bool discardOldest);
  bool connect();
  void deactivateMonitoredItem(Subscription &subscription,
      MonitoredItem &monitoredItem);
  void deactivateSubscription(Subscription &subscription);
  bool maybeResetConnection(UA_StatusCode statusCode);
  UaVariant readInternal(const UaNodeId &nodeId);
  void removeMonitoredItemInternal(const std::string &subscriptionName,
      const UaNodeId &nodeId,
      std::shared_ptr<MonitoredItemCallback> const &callback);
  void runConnectionThread();
  void writeInternal(const UaNodeId &nodeId, const UaVariant &value);

  static void monitoredItemDataChangeNotificationCallback(UA_Client *client,
      UA_UInt32 subscriptionId, void *subscriptionContext,
      UA_UInt32 monitoredItemId, void *monitoredItemContext,
      UA_DataValue *value);

};

}
}

#endif // OPEN62541_EPICS_SERVER_CONNECTION_H
