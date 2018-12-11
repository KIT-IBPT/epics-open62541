/*
 * Copyright 2017 aquenos GmbH.
 * Copyright 2017 Karlsruhe Institute of Technology.
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

#ifndef OPEN62541_EPICS_SERVER_CONNECTION_REGISTRY_H
#define OPEN62541_EPICS_SERVER_CONNECTION_REGISTRY_H

// There is a bug in the C++ standard library of certain versions of the macOS
// SDK that causes a problem when including <mutex>. The workaround for this is
// defining the _DARWIN_C_SOURCE preprocessor macro.
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif

#include <mutex>
#include <unordered_map>

#include "ServerConnection.h"

namespace open62541 {
namespace epics {

/**
 * Registry holding server connections. Server connections are registered with
 * the registry during initialization and can then be retrieved for use by
 * different records.
 * This class implements the singleton pattern and the only instance is returned
 * by the {@link #getInstance()} function.
 */
class ServerConnectionRegistry {

public:

  /**
   * Returns the only instance of this class.
   */
  inline static ServerConnectionRegistry &getInstance() {
    return instance;
  }

  /**
   * Returns the connection with the specified ID. If no connection with the ID
   * has been registered, a pointer to null is returned.
   */
  std::shared_ptr<ServerConnection> getServerConnection(
      const std::string &deviceId);

  /**
   * Registers a connection under the specified ID. Throws an exception if the
   * connection cannot be registered because the specified ID is already in use.
   */
  void registerServerConnection(const std::string &connectionId,
      std::shared_ptr<ServerConnection> connection);

private:

  // We do not want to allow copy or move construction or assignment.
  ServerConnectionRegistry(const ServerConnectionRegistry &) = delete;
  ServerConnectionRegistry(ServerConnectionRegistry &&) = delete;
  ServerConnectionRegistry &operator=(const ServerConnectionRegistry &) = delete;
  ServerConnectionRegistry &operator=(ServerConnectionRegistry &&) = delete;

  static ServerConnectionRegistry instance;

  std::unordered_map<std::string, std::shared_ptr<ServerConnection>> connections;
  std::recursive_mutex mutex;

  ServerConnectionRegistry();

};

}
}

#endif // OPEN62541_EPICS_SERVER_CONNECTION_REGISTRY_H
