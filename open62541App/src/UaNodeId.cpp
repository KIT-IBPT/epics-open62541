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

#include "UaNodeId.h"

namespace open62541 {
namespace epics {

UaNodeId::UaNodeId(UA_NodeId const &id) {
  UA_NodeId_init(&this->id);
  auto status = UA_NodeId_copy(&id, &this->id);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
}

UaNodeId::UaNodeId(UaNodeId const &other) {
  UA_NodeId_init(&this->id);
  auto status = UA_NodeId_copy(&other.id, &this->id);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
}

UaNodeId &UaNodeId::operator=(UaNodeId const &other) {
  UA_NodeId tempId;
  UA_NodeId_init(&tempId);
  auto status = UA_NodeId_copy(&other.id, &tempId);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
  UA_NodeId_clear(&this->id);
  this->id = tempId;
  return *this;
}

inline std::string UaNodeId::toString() const {
    UA_String tempStr;
    UA_String_init(&tempStr);
  auto status = UA_NodeId_toString(&this->id, &tempStr);
  if (status != UA_STATUSCODE_GOOD) {
    throw UaException(status);
  }
  std::string result(reinterpret_cast<char *>(tempStr.data), tempStr.length);
  UA_String_clear(&tempStr);
  return result;
}

} // namespace epics
} // namespace open62541

