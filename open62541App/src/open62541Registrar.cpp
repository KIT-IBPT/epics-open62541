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

#include <cstring>

#include <epicsExport.h>
#include <iocsh.h>

#include "open62541Error.h"
#include "ServerConnectionRegistry.h"
#include "UaException.h"

using namespace open62541::epics;

extern "C" {

// Data structures needed for the iocsh mrfMapInterruptToEvent function.
static const iocshArg iocshOpen62541ConnectionSetupArg0 = { "connection ID",
    iocshArgString };
static const iocshArg iocshOpen62541ConnectionSetupArg1 = { "endpoint URL",
    iocshArgString };
static const iocshArg iocshOpen62541ConnectionSetupArg2 = { "username",
    iocshArgString };
static const iocshArg iocshOpen62541ConnectionSetupArg3 = { "password",
    iocshArgString };
static const iocshArg * const iocshOpen62541ConnectionSetupArgs[] = {
    &iocshOpen62541ConnectionSetupArg0, &iocshOpen62541ConnectionSetupArg1,
    &iocshOpen62541ConnectionSetupArg2, &iocshOpen62541ConnectionSetupArg3 };
static const iocshFuncDef iocshOpen62541ConnectionSetupFuncDef = {
    "open62541ConnectionSetup", 4, iocshOpen62541ConnectionSetupArgs };

/**
 * Implementation of the iocsh mrfMapInterruptToEvent function. This function
 * maps the interrupt for a specific device to an EPICS event. The interrupt
 * flag mask defines for which kind of interrupts an event is triggered. An
 * event is only triggered when at least one of the bits in the specified mask
 * is also set in the interrupt flags of the interrupt event.
 */
static void iocshOpen62541ConnectionSetupFunc(const iocshArgBuf *args)
    noexcept {
  char *connectionId = args[0].sval;
  char *endpointUrl = args[1].sval;
  char *username = args[2].sval;
  char *password = args[3].sval;
  // Verify and convert the parameters.
  if (!connectionId) {
    errorPrintf(
        "Could not setup the connection: Connection ID must be specified.");
    return;
  }
  if (!std::strlen(connectionId)) {
    errorPrintf(
        "Could not setup the connection: Connection ID must not be empty.");
    return;
  }
  std::shared_ptr<ServerConnection> connection;
  try {
    if (username && std::strlen(username)) {
      connection = std::make_shared<ServerConnection>(endpointUrl, username,
          password ? password : "");
    } else {
      connection = std::make_shared<ServerConnection>(endpointUrl);
    }
  } catch (const UaException &e) {
    errorPrintf("Could not setup the connection: %s", e.what());
    return;
  }
  ServerConnectionRegistry::getInstance().registerServerConnection(connectionId,
      connection);
}

/**
 * Registrar that registers the iocsh commands.
 */
static void open62541Registrar() {
  ::iocshRegister(&iocshOpen62541ConnectionSetupFuncDef,
      iocshOpen62541ConnectionSetupFunc);
}

epicsExportRegistrar (open62541Registrar);

} // extern "C"
