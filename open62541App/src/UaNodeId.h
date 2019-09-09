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

#ifndef OPEN62541_EPICS_UA_NODE_ID_H
#define OPEN62541_EPICS_UA_NODE_ID_H

#include <cstdint>
#include <functional>
#include <string>

extern "C" {
#include "open62541.h"
}

#include "UaException.h"

namespace open62541 {
namespace epics {

/**
 * OPC UA node ID. This is a wrapper around UA_NodeId that takes care of
 * ensuring that memory is correctly handled when creating, copying and deleting
 * node IDs.
 */
class UaNodeId {

public:

  /**
   * Creates an empty node ID. Such an object does not represent a valid node
   * ID, but it can safely be assigned or deleted.
   */
  inline UaNodeId() {
    UA_NodeId_init(&this->id);
  }

  /**
   * Creates a node ID that is a copy of the passed node ID.
   */
  UaNodeId(UaNodeId const &other);

  /**
   * Creates a node ID moving the data from another node ID.
   */
  inline UaNodeId(UaNodeId &&other) {
    this->id = other.id;
    UA_NodeId_init(&other.id);
  }

  /**
   * Creates a node ID that is a copy of the passed node ID.
   */
  UaNodeId(UA_NodeId const &id);

  /**
   * Creates a node ID moving the data from another node ID.
   */
  UaNodeId(UA_NodeId &&id) {
    this->id = id;
    UA_NodeId_init(&id);
  }

  /**
   * Destructor.
   */
  inline ~UaNodeId() {
    UA_NodeId_clear(&this->id);
  }

  UaNodeId &operator=(UaNodeId const &other);

  inline UaNodeId &operator=(UaNodeId &&other) {
    UA_NodeId_clear(&this->id);
    this->id = other.id;
    UA_NodeId_init(&other.id);
    return *this;
  }

  inline bool operator==(UaNodeId const &other) const {
    return UA_NodeId_equal(&this->id, &other.id);
  }

  inline bool operator<(UaNodeId const &other) const {
    return UA_NodeId_order(&this->id, &other.id) == UA_ORDER_LESS;
  }

  inline bool operator<=(UaNodeId const &other) const {
    auto order = UA_NodeId_order(&this->id, &other.id);
    return order == UA_ORDER_LESS || order == UA_ORDER_EQ;
  }

  inline bool operator>(UaNodeId const &other) const {
    return UA_NodeId_order(&this->id, &other.id) == UA_ORDER_MORE;
  }

  inline bool operator>=(UaNodeId const &other) const {
    auto order = UA_NodeId_order(&this->id, &other.id);
    return order == UA_ORDER_MORE || order == UA_ORDER_EQ;
  }

  inline explicit operator bool() const {
    return !UA_NodeId_isNull(&this->id);
  }

  /**
   * Creates and returns a node ID of type UA_NODEIDTYPE_BYTESTRING.
   */
  inline static UaNodeId createByteString(std::uint16_t nsIndex,
      std::string const &identifier) {
    auto internalId = UA_NODEID_BYTESTRING_ALLOC(nsIndex, identifier.c_str());
    return UaNodeId(std::move(internalId));
  }

  /**
   * Creates and returns a node ID of type UA_NODEIDTYPE_GUID.
   */
  inline static UaNodeId createByteString(std::uint16_t nsIndex,
      UA_Guid const &identifier) {
    auto internalId = UA_NODEID_GUID(nsIndex, identifier);
    return UaNodeId(std::move(internalId));
  }

  /**
   * Creates and returns a node ID of type UA_NODEIDTYPE_NUMERIC.
   */
  inline static UaNodeId createNumeric(std::uint16_t nsIndex,
      std::uint32_t identifier) {
    auto internalId = UA_NODEID_NUMERIC(nsIndex, identifier);
    return UaNodeId(std::move(internalId));
  }

  /**
   * Creates and returns a node ID of type UA_NODEIDTYPE_STRING.
   */
  inline static UaNodeId createString(std::uint16_t nsIndex,
      std::string const &identifier) {
    auto internalId = UA_NODEID_STRING_ALLOC(nsIndex, identifier.c_str());
    return UaNodeId(std::move(internalId));
  }

  /**
   * Returns a reference to the underlying node ID as used by the open62541
   * library.
   */
  inline UA_NodeId const & get() const {
    return id;
  }

  /**
   * Returns a string representation of this node ID.
   */
  std::string toString() const;

private:

  UA_NodeId id;

};

} // namespace epics
} // namespace open62541

namespace std {

/**
 * Hasher for the NodeId type.
 */
template<> struct hash<open62541::epics::UaNodeId> {

  inline std::size_t operator()(open62541::epics::UaNodeId const &nodeId) const {
    return UA_NodeId_hash(&nodeId.get());
  }

};

} // namespace std

#endif // OPEN62541_EPICS_UA_NODE_ID_H
