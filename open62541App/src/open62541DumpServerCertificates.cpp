/*
 * Copyright 2019-2024 aquenos GmbH.
 * Copyright 2019-2024 Karlsruhe Institute of Technology.
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
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

extern "C" {
#include <osiFileName.h>

#include "open62541.h"

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
#include <mbedtls/md.h>
#endif // UA_ENABLE_ENCRYPTION_MBEDTLS

#if defined (UA_ENABLE_ENCRYPTION_LIBRESSL) || defined (UA_ENABLE_ENCRYPTION_OPENSSL)
#include <openssl/sha.h>
#endif // UA_ENABLE_ENCRYPTION_OPENSSL
} // extern "C"

#include "UaException.h"

#include "open62541DumpServerCertificates.h"

namespace open62541 {
namespace epics {

namespace {

std::string hexDump(unsigned char *data, std::size_t size) {
  std::string result;
  char buffer[3];
  for (std::size_t i = 0; i < size; ++i) {
    std::snprintf(buffer, sizeof(buffer), "%02hhx", data[i]);
    result += buffer;
  }
  return result;
}

} // anonymous namespace

void dumpServerCertificates(
    std::string const &endpointUrl, std::string const &targetDirectoryPath) {
// We can only use the hash functions from mbed TLS if the device support is
// actually compiled with encryption support.
#ifdef UA_ENABLE_ENCRYPTION
  UA_Client *client = nullptr;
  UA_EndpointDescription *endpointDescriptions = nullptr;
  std::size_t endpointDescriptionsSize = 0;
  try {
    client = UA_Client_new();
    if (!client) {
      throw std::runtime_error("Cannot instantiate UA_Client.");
    }
    auto config = UA_Client_getConfig(client);
    auto statusCode = UA_ClientConfig_setDefault(config);
    if (statusCode != UA_STATUSCODE_GOOD) {
      throw UaException(statusCode);
    }
    statusCode = UA_Client_getEndpoints(
      client, endpointUrl.c_str(), &endpointDescriptionsSize,
      &endpointDescriptions);
    if (statusCode != UA_STATUSCODE_GOOD) {
      throw UaException(statusCode);
    }
#  if defined (UA_ENABLE_ENCRYPTION_MBEDTLS)
    auto mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!mdInfo) {
      throw std::runtime_error(
        "mbedtls_md_info_from_type returned null for MBEDTLS_MD_SHA256.");
    }
    unsigned char digest[MBEDTLS_MD_MAX_SIZE];
    std::size_t const mdSize = mbedtls_md_get_size(mdInfo);
#  elif defined (UA_ENABLE_ENCRYPTION_LIBRESSL) || defined (UA_ENABLE_ENCRYPTION_OPENSSL)
    unsigned char digest[SHA256_DIGEST_LENGTH];
    std::size_t const mdSize = SHA256_DIGEST_LENGTH;
#  else // UA_ENABLE_ENRYPTION_...
#    error "The selected crypto library is not supported."
#  endif // UA_ENABLE_ENCRYPTION_...
    std::unordered_set<std::string> seenFilenames;
    for (std::size_t i = 0; i < endpointDescriptionsSize; ++i) {
      auto serverCertificate = endpointDescriptions[i].serverCertificate;
      if (!serverCertificate.length) {
        continue;
      }
#  if defined (UA_ENABLE_ENCRYPTION_MBEDTLS)
      if (mbedtls_md(mdInfo, serverCertificate.data, serverCertificate.length, digest)) {
        throw std::runtime_error("mbedtls_md failed.");
      }
#  elif defined (UA_ENABLE_ENCRYPTION_LIBRESSL) || defined (UA_ENABLE_ENCRYPTION_OPENSSL)
      SHA256(serverCertificate.data, serverCertificate.length, digest);
#  endif // UA_ENABLE_ENCRYPTION_...
      auto filename = hexDump(digest, mdSize) + ".der";
      // If we have already seen the same filename before (because we have
      // encountered the certificate before), we do not process it again.
      if (!seenFilenames.insert(filename).second) {
        continue;
      }
      std::string path;
      if (targetDirectoryPath.empty()) {
        path = filename;
      } else {
        path = targetDirectoryPath;
        if (path.back() != OSI_PATH_SEPARATOR[0]) {
          path += OSI_PATH_SEPARATOR;
        }
        path += filename;
      }
      try {
        std::ofstream stream(
          path, std::ios::binary | std::ios::out | std::ios::trunc);
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.write(
          reinterpret_cast<char const *>(serverCertificate.data),
          serverCertificate.length);
      } catch (...) {
        throw std::runtime_error(
          std::string("Error while trying to write \"") + path + "\".");
      }
    }
  } catch (...) {
    if (endpointDescriptions) {
      UA_Array_delete(
        endpointDescriptions, endpointDescriptionsSize,
        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    }
    if (client) {
      UA_Client_delete(client);
    }
    throw;
  }
  UA_Array_delete(
    endpointDescriptions, endpointDescriptionsSize,
    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  endpointDescriptions = nullptr;
  endpointDescriptionsSize = 0;
  UA_Client_delete(client);
#else
    throw std::logic_error(
      "The encryption features are not available because the EPICS device support has been compiled without them. Please set USE_MBEDTLS to YES in configure/CONFIG_SITE.local and recompile the device support to enable them.");
#endif
}

} // namespace epics
} // namespace open62541
