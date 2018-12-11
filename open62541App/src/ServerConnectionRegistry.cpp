/*
 * Copyright 2017 aquenos GmbH. All rights reserved.
 * http://www.aquenos.com/
 *
 * This software is licensed to the licensee under the terms
 * of the respective license agreement.
 *
 * The EPICS integration code is based on code originally written for the
 * s7nodave device support, Copyright 2012-2015 aquenos GmbH.
 */

#include <stdexcept>

#include "ServerConnectionRegistry.h"

namespace open62541 {
namespace epics {

std::shared_ptr<ServerConnection> ServerConnectionRegistry::getServerConnection(
    const std::string &connectionId) {
  // We have to hold the mutex in order to protect the map from concurrent
  // access.
  std::lock_guard<std::recursive_mutex> lock(mutex);
  auto connection = connections.find(connectionId);
  if (connection == connections.end()) {
    return std::shared_ptr<ServerConnection>();
  } else {
    return connection->second;
  }
}

void ServerConnectionRegistry::registerServerConnection(
    const std::string &connectionId,
    std::shared_ptr<ServerConnection> connection) {
  // We have to hold the mutex in order to protect the map from concurrent
  // access.
  std::lock_guard<std::recursive_mutex> lock(mutex);
  if (connections.count(connectionId)) {
    throw std::runtime_error("Connection ID is already in use.");
  }
  connections.insert(std::make_pair(connectionId, connection));
}

ServerConnectionRegistry ServerConnectionRegistry::instance;

ServerConnectionRegistry::ServerConnectionRegistry() {
}

}
}
