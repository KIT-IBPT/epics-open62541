/*
 * Copyright 2019 aquenos GmbH.
 * Copyright 2019 Karlsruhe Institute of Technology.
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

#ifndef OPEN62541_EPICS_DUMP_SERVER_CERTIFICATES_H
#define OPEN62541_EPICS_DUMP_SERVER_CERTIFICATES_H

#include <string>

namespace open62541 {
namespace epics {

/**
 * Connects to the specified server and dumps all the certificates presented by
 * that server (as part of the available endpoints) into the specified target
 * directory. Each certificate is written to a file that uses the SHA-256 hash
 * of its content as the name. Existing files are overwritten.
 */
void dumpServerCertificates(
    std::string const &endpointUrl, std::string const &targetDirectoryPath);

} // namespace epics
} // namespace open62541

#endif // OPEN62541_EPICS_DUMP_SERVER_CERTIFICATES_H
