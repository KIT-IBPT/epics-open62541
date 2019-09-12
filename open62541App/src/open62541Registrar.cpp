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

#include <cstring>
#include <exception>

#include <epicsExport.h>
#include <iocsh.h>

#include "open62541DumpServerCertificates.h"
#include "open62541Error.h"
#include "ServerConnectionRegistry.h"
#include "UaException.h"

using namespace open62541::epics;

extern "C" {

// Data structures needed for the iocsh open62541ConnectionSetup function.
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
 * Implementation of the iocsh open62541ConnectionSetup function. This function
 * creates a connection to an OPC UA server.
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
  if (!endpointUrl) {
    errorPrintf(
      "Could not setup the connection: Endpoint URL must be specified.");
    return;
  }
  if (!std::strlen(endpointUrl)) {
    errorPrintf(
      "Could not setup the connection: Endpoint URL must not be empty.");
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
  } catch (const std::exception &e) {
    errorPrintf("Could not setup the connection: %s", e.what());
  }
  ServerConnectionRegistry::getInstance().registerServerConnection(connectionId,
      connection);
}

// Data structures needed for the iocsh open62541ConnectionSetupEncrypted
// function.
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg0 = {
  "connection ID",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg1 = {
  "endpoint URL",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg2 = {
  "username",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg3 = {
  "password",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg4 = {
  "security mode",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg5 = {
  "client certificate path",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg6 = {
  "client key path",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg7 = {
  "server cert path",
  iocshArgString
};
static const iocshArg iocshOpen62541ConnectionSetupEncryptedArg8 = {
  "client application URI",
  iocshArgString
};
static const iocshArg * const iocshOpen62541ConnectionSetupEncryptedArgs[] = {
  &iocshOpen62541ConnectionSetupEncryptedArg0,
  &iocshOpen62541ConnectionSetupEncryptedArg1,
  &iocshOpen62541ConnectionSetupEncryptedArg2,
  &iocshOpen62541ConnectionSetupEncryptedArg3,
  &iocshOpen62541ConnectionSetupEncryptedArg4,
  &iocshOpen62541ConnectionSetupEncryptedArg5,
  &iocshOpen62541ConnectionSetupEncryptedArg6,
  &iocshOpen62541ConnectionSetupEncryptedArg7,
  &iocshOpen62541ConnectionSetupEncryptedArg8
};
static const iocshFuncDef iocshOpen62541ConnectionSetupEncryptedFuncDef = {
  "open62541ConnectionSetupEncrypted", 9,
  iocshOpen62541ConnectionSetupEncryptedArgs
};

/**
 * Implementation of the iocsh open62541ConnectionSetupEncrypted function. This
 * function creates a connection to an OPC UA server.
 */
static void iocshOpen62541ConnectionSetupEncryptedFunc(const iocshArgBuf *args)
    noexcept {
  char const *connectionId = args[0].sval;
  char const *endpointUrl = args[1].sval;
  char const *username = args[2].sval;
  char const *password = args[3].sval;
  char const *securityModeString = args[4].sval;
  char const *clientCertPath = args[5].sval;
  char const *clientKeyPath = args[6].sval;
  char const *serverCertPath = args[7].sval;
  char const *applicationUri = args[8].sval;
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
  if (!endpointUrl) {
    errorPrintf(
      "Could not setup the connection: Endpoint URL must be specified.");
    return;
  }
  if (!std::strlen(endpointUrl)) {
    errorPrintf(
      "Could not setup the connection: Endpoint URL must not be empty.");
    return;
  }
  if (!securityModeString || !strlen(securityModeString)) {
    securityModeString = "none";
  }
  ServerConnection::SecurityMode securityMode;
  if (!strcasecmp(securityModeString, "none")) {
    securityMode = ServerConnection::SecurityMode::none;
  } else if (!strcasecmp(securityModeString, "sign")) {
    securityMode = ServerConnection::SecurityMode::sign;
  } else if (!strcasecmp(securityModeString, "sign & encrypt")) {
    securityMode = ServerConnection::SecurityMode::signAndEncrypt;
  } else {
    errorPrintf(
      "Could not setup the connection: The security mode must be one of \"invalid\", \"none\", \"sign\", or \"sign & encrypt\".");
    return;
  }
  if (!clientCertPath) {
    errorPrintf(
      "Could not setup the connection: Client certificate path must be specified.");
    return;
  }
  if (!std::strlen(clientCertPath)) {
    errorPrintf(
      "Could not setup the connection: Client certificate path must not be empty.");
    return;
  }
  if (!clientKeyPath) {
    errorPrintf(
      "Could not setup the connection: Client key path must be specified.");
    return;
  }
  if (!std::strlen(clientKeyPath)) {
    errorPrintf(
      "Could not setup the connection: Client key path must not be empty.");
    return;
  }
  if (!serverCertPath) {
    serverCertPath = "";
  }
  if (!applicationUri) {
    applicationUri = "";
  }
  std::shared_ptr<ServerConnection> connection;
  try {
    if (username && std::strlen(username)) {
      connection = std::make_shared<ServerConnection>(
        endpointUrl, username, password ? password : "", securityMode,
        clientCertPath, clientKeyPath, serverCertPath, applicationUri);
    } else {
      connection = std::make_shared<ServerConnection>(
        endpointUrl, securityMode, clientCertPath, clientKeyPath,
        serverCertPath, applicationUri);
    }
  } catch (const UaException &e) {
    errorPrintf("Could not setup the connection: %s", e.what());
    return;
  } catch (const std::exception &e) {
    errorPrintf("Could not setup the connection: %s", e.what());
  }
  ServerConnectionRegistry::getInstance().registerServerConnection(connectionId,
      connection);
}

// Data structures needed for the iocsh open62541DumpServerCertificates
// function.
static const iocshArg iocshOpen62541DumpServerCertificatesArg0 = {
  "endpoint URL",
  iocshArgString
};
static const iocshArg iocshOpen62541DumpServerCertificatesArg1 = {
  "target directory",
  iocshArgString
};
static const iocshArg * const iocshOpen62541DumpServerCertificatesArgs[] = {
  &iocshOpen62541DumpServerCertificatesArg0,
  &iocshOpen62541DumpServerCertificatesArg1
};
static const iocshFuncDef iocshOpen62541DumpServerCertificatesFuncDef = {
  "open62541DumpServerCertificates", 2, iocshOpen62541DumpServerCertificatesArgs
};

/**
 * Implementation of the iocsh open62541DumpServerCertificates function. This
 * function dumps all the certificates presented by a sepcific server to files
 * inside a target directory.
 */
static void iocshOpen62541DumpServerCertificatesFunc(const iocshArgBuf *args)
    noexcept {
  char const *endpointUrl = args[0].sval;
  char const *targetDirectory = args[1].sval;
  // Verify and convert the parameters.
  if (!endpointUrl) {
    errorPrintf(
      "Could not dump server certificates: Endpoint URL must be specified.");
    return;
  }
  if (!std::strlen(endpointUrl)) {
    errorPrintf(
      "Could not dump server certificates: Endpoint URL must not be empty.");
    return;
  }
  if (!targetDirectory) {
    targetDirectory = "";
  }
  try {
    dumpServerCertificates(endpointUrl, targetDirectory);
  } catch (const UaException &e) {
    errorPrintf("Could not dump the certificates: %s", e.what());
    return;
  } catch (const std::exception &e) {
    errorPrintf("Could not dump the certificates: %s", e.what());
  }
}

// Data structures needed for the iocsh open62541SetSubscriptionLifetimeCount
// function.
static const iocshArg iocshOpen62541SetSubscriptionLifetimeCountArg0 = {
  "connection ID", iocshArgString
};
static const iocshArg iocshOpen62541SetSubscriptionLifetimeCountArg1 = {
  "subscription ID", iocshArgString
};
static const iocshArg iocshOpen62541SetSubscriptionLifetimeCountArg2 = {
  "lifetime count", iocshArgInt
};

static const iocshArg * const iocshOpen62541SetSubscriptionLifetimeCountArgs[] = {
  &iocshOpen62541SetSubscriptionLifetimeCountArg0,
  &iocshOpen62541SetSubscriptionLifetimeCountArg1,
  &iocshOpen62541SetSubscriptionLifetimeCountArg2
};
static const iocshFuncDef iocshOpen62541SetSubscriptionLifetimeCountFuncDef = {
  "open62541SetSubscriptionLifetimeCount", 3,
  iocshOpen62541SetSubscriptionLifetimeCountArgs
};

/**
 * Implementation of the iocsh open62541SetSubscriptionLifetimeCount function.
 * This function sets the requested lifetime count for a specific subscription
 * associated with a specific connection.
 */
static void iocshOpen62541SetSubscriptionLifetimeCountFunc(
    const iocshArgBuf *args) noexcept {
  char *connectionId = args[0].sval;
  char *subscriptionId = args[1].sval;
  int lifetimeCount = args[2].ival;
  // Verify and convert the parameters.
  if (!connectionId) {
    errorPrintf(
      "Could not set the subscription lifetime count: Connection ID must be specified.");
    return;
  }
  if (!std::strlen(connectionId)) {
    errorPrintf(
      "Could not set the subscription lifetime count: Connection ID must not be empty.");
    return;
  }
  if (!subscriptionId) {
    errorPrintf(
      "Could not set the subscription lifetime count: Subscription ID must be specified.");
    return;
  }
  if (!std::strlen(subscriptionId)) {
    errorPrintf(
      "Could not set the subscription lifetime count: Subscription ID must not be empty.");
    return;
  }
  if (lifetimeCount < 0) {
    errorPrintf(
      "Could not set the subscription lifetime count: The lifetime count cannot be negative.");
    return;
  }
  std::shared_ptr<ServerConnection> connection =
    ServerConnectionRegistry::getInstance().getServerConnection(connectionId);
  if (!connection) {
    errorPrintf(
      "Could not set the subscription lifetime count: The connection with the ID \"%s\" does not exist.",
      connectionId);
    return;
  }
  connection->setSubscriptionLifetimeCount(subscriptionId, lifetimeCount);
}

// Data structures needed for the iocsh
// open62541SetSubscriptionMaxKeepAliveCount function.
static const iocshArg iocshOpen62541SetSubscriptionMaxKeepAliveCountArg0 = {
  "connection ID", iocshArgString
};
static const iocshArg iocshOpen62541SetSubscriptionMaxKeepAliveCountArg1 = {
  "subscription ID", iocshArgString
};
static const iocshArg iocshOpen62541SetSubscriptionMaxKeepAliveCountArg2 = {
  "max. keep alive count", iocshArgInt
};

static const iocshArg * const iocshOpen62541SetSubscriptionMaxKeepAliveCountArgs[] = {
  &iocshOpen62541SetSubscriptionMaxKeepAliveCountArg0,
  &iocshOpen62541SetSubscriptionMaxKeepAliveCountArg1,
  &iocshOpen62541SetSubscriptionMaxKeepAliveCountArg2
};
static const iocshFuncDef iocshOpen62541SetSubscriptionMaxKeepAliveCountFuncDef = {
  "open62541SetSubscriptionMaxKeepAliveCount", 3,
  iocshOpen62541SetSubscriptionMaxKeepAliveCountArgs
};

/**
 * Implementation of the iocsh open62541SetSubscriptionMaxKeepAliveCount
 * function. This function sets the requested max. keep alive count for a
 * specific subscription associated with a specific connection.
 */
static void iocshOpen62541SetSubscriptionMaxKeepAliveCountFunc(
    const iocshArgBuf *args) noexcept {
  char *connectionId = args[0].sval;
  char *subscriptionId = args[1].sval;
  int maxKeepAliveCount = args[2].ival;
  // Verify and convert the parameters.
  if (!connectionId) {
    errorPrintf(
      "Could not set the subscription max. keep alive count: Connection ID must be specified.");
    return;
  }
  if (!std::strlen(connectionId)) {
    errorPrintf(
      "Could not set the subscription max. keep alive count: Connection ID must not be empty.");
    return;
  }
  if (!subscriptionId) {
    errorPrintf(
      "Could not set the subscription max. keep alive count: Subscription ID must be specified.");
    return;
  }
  if (!std::strlen(subscriptionId)) {
    errorPrintf(
      "Could not set the subscription max. keep alive count: Subscription ID must not be empty.");
    return;
  }
  if (maxKeepAliveCount < 0) {
    errorPrintf(
      "Could not set the subscription max. keep alive count: The MaxKeepAlive count cannot be negative.");
    return;
  }
  std::shared_ptr<ServerConnection> connection =
    ServerConnectionRegistry::getInstance().getServerConnection(connectionId);
  if (!connection) {
    errorPrintf(
      "Could not set the subscription max. keep alive count: The connection with the ID \"%s\" does not exist.",
      connectionId);
    return;
  }
  connection->setSubscriptionMaxKeepAliveCount(subscriptionId, maxKeepAliveCount);
}

// Data structures needed for the iocsh
// open62541SetSubscriptionPublishingInterval function.
static const iocshArg iocshOpen62541SetSubscriptionPublishingIntervalArg0 = {
  "connection ID", iocshArgString
};
static const iocshArg iocshOpen62541SetSubscriptionPublishingIntervalArg1 = {
  "subscription ID", iocshArgString
};
static const iocshArg iocshOpen62541SetSubscriptionPublishingIntervalArg2 = {
  "publishing interval (in ms)", iocshArgDouble
};

static const iocshArg * const iocshOpen62541SetSubscriptionPublishingIntervalArgs[] = {
  &iocshOpen62541SetSubscriptionPublishingIntervalArg0,
  &iocshOpen62541SetSubscriptionPublishingIntervalArg1,
  &iocshOpen62541SetSubscriptionPublishingIntervalArg2
};
static const iocshFuncDef iocshOpen62541SetSubscriptionPublishingIntervalFuncDef = {
  "open62541SetSubscriptionPublishingInterval", 3,
  iocshOpen62541SetSubscriptionPublishingIntervalArgs
};

/**
 * Implementation of the iocsh open62541SetSubscriptionPublishingInterval function.
 * This function sets the requested publishing interval for a specific subscription
 * associated with a specific connection.
 */
static void iocshOpen62541SetSubscriptionPublishingIntervalFunc(
    const iocshArgBuf *args) noexcept {
  char *connectionId = args[0].sval;
  char *subscriptionId = args[1].sval;
  double publishingInterval = args[2].dval;
  // Verify and convert the parameters.
  if (!connectionId) {
    errorPrintf(
      "Could not set the subscription publishing interval: Connection ID must be specified.");
    return;
  }
  if (!std::strlen(connectionId)) {
    errorPrintf(
      "Could not set the subscription publishing interval: Connection ID must not be empty.");
    return;
  }
  if (!subscriptionId) {
    errorPrintf(
      "Could not set the subscription publishing interval: Subscription ID must be specified.");
    return;
  }
  if (!std::strlen(subscriptionId)) {
    errorPrintf(
      "Could not set the subscription publishing interval: Subscription ID must not be empty.");
    return;
  }
  std::shared_ptr<ServerConnection> connection =
    ServerConnectionRegistry::getInstance().getServerConnection(connectionId);
  if (!connection) {
    errorPrintf(
      "Could not set the subscription publishing interval: The connection with the ID \"%s\" does not exist.",
      connectionId);
    return;
  }
  connection->setSubscriptionPublishingInterval(subscriptionId, publishingInterval);
}

/**
 * Registrar that registers the iocsh commands.
 */
static void open62541Registrar() {
  ::iocshRegister(
    &iocshOpen62541ConnectionSetupFuncDef,
    iocshOpen62541ConnectionSetupFunc);
  ::iocshRegister(
    &iocshOpen62541ConnectionSetupEncryptedFuncDef,
    iocshOpen62541ConnectionSetupEncryptedFunc);
  ::iocshRegister(
    &iocshOpen62541DumpServerCertificatesFuncDef,
    iocshOpen62541DumpServerCertificatesFunc);
  ::iocshRegister(
    &iocshOpen62541SetSubscriptionLifetimeCountFuncDef,
    iocshOpen62541SetSubscriptionLifetimeCountFunc);
  ::iocshRegister(
    &iocshOpen62541SetSubscriptionMaxKeepAliveCountFuncDef,
    iocshOpen62541SetSubscriptionMaxKeepAliveCountFunc);
  ::iocshRegister(
    &iocshOpen62541SetSubscriptionPublishingIntervalFuncDef,
    iocshOpen62541SetSubscriptionPublishingIntervalFunc);
}

epicsExportRegistrar(open62541Registrar);

} // extern "C"
