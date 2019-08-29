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
#include <list>
#include <mutex>
#include <string>
#include <thread>

extern "C" {
#include "open62541.h"
}

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
    virtual void success(const UA_NodeId &nodeId, const UA_Variant &value) = 0;

    /**
     * Called when a read operation fails. The node ID passed is the node ID
     * specified in the read request. The status code  gives information about
     * the cause of the failure.
     */
    virtual void failure(const UA_NodeId &nodeId, UA_StatusCode statusCode) =0;

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
    virtual void success(const UA_NodeId &nodeId) = 0;

    /**
     * Called when a write operation fails. The node ID passed is the node ID
     * specified in the read request. The status code  gives information about
     * the cause of the failure.
     */
    virtual void failure(const UA_NodeId &nodeId, UA_StatusCode statusCode) =0;

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
   * Reads a node's value. Throws an UaException if there is a problem.
   */
  void read(const UA_NodeId &nodeId, UA_Variant &targetValue);

  /**
   * Reads a node's value asynchronously. When the operation completes, the
   * passed callback is called. This method internally makes a copy of the node
   * ID so that the passed node ID does not have to be kept alive after this
   * method returns.
   */
  void readAsync(const UA_NodeId &nodeId,
      std::shared_ptr<ReadCallback> callback);

  /**
   * Writes to a node's value. Throws an UaException if there is a problem.
   */
  void write(const UA_NodeId &nodeId, const UA_Variant &value);

  /**
   * Writes a node's value asynchronously. When the operation completes, the
   * passed callback is called. This method internally makes a copy of the node
   * ID and value so that the passed node ID and value do not have to be kept
   * alive after this method returns.
   */
  void writeAsync(const UA_NodeId &nodeId, const UA_Variant &value,
      std::shared_ptr<WriteCallback> callback);

private:

  enum class RequestType {
    read, write
  };

  class Request {

  public:

    Request(const Request &request);
    Request(Request &&request);
    ~Request();

    Request &operator=(const Request &request);
    Request &operator=(Request &&request);

    static Request readRequest(const UA_NodeId &nodeId,
        const std::shared_ptr<ReadCallback> &callback);
    static Request writeRequest(const UA_NodeId &nodeId,
        const UA_Variant &value,
        const std::shared_ptr<WriteCallback> &callback);

    inline RequestType getType() const {
      return type;
    }

    inline UA_NodeId const & getNodeId() const {
      return nodeId;
    }

    inline UA_Variant const & getValue() const {
      return value;
    }

    inline ReadCallback& getReadCallback() {
      if (!readCallback) {
        throw std::runtime_error("Read callback not available.");
      }
      return *readCallback;
    }

    inline WriteCallback& getWriteCallback() {
      return *writeCallback;
    }

  private:

    Request(RequestType type, const UA_NodeId &nodeId, const UA_Variant &value,
        const std::shared_ptr<ReadCallback> &readCallback,
        const std::shared_ptr<WriteCallback> &writeCallback);

    RequestType type;
    UA_NodeId nodeId;
    UA_Variant value;
    std::shared_ptr<ReadCallback> readCallback;
    std::shared_ptr<WriteCallback> writeCallback;

  };

  std::string endpointUrl;
  std::string username;
  std::string password;
  bool useAuthentication;

  std::mutex mutex;
  std::thread connectionThread;
  std::condition_variable connectionThreadCv;
  std::atomic<bool> shutdownRequested;
  std::list<Request> requestQueue;
  UA_Client *client;

  // We do not want to allow copy or move construction or assignment.
  ServerConnection(const ServerConnection &) = delete;
  ServerConnection(ServerConnection &&) = delete;
  ServerConnection &operator=(const ServerConnection &) = delete;
  ServerConnection &operator=(ServerConnection &&) = delete;

  ServerConnection(const std::string &endpointUrl,
      const std::string &username, const std::string &password,
      bool useAuthentication);

  bool connect();

  void runConnectionThread();

  UA_StatusCode readInternal(const UA_NodeId &nodeId, UA_Variant &targetValue);
  UA_StatusCode writeInternal(const UA_NodeId &nodeId, const UA_Variant &value);

};

}
}

#endif // OPEN62541_EPICS_SERVER_CONNECTION_H
