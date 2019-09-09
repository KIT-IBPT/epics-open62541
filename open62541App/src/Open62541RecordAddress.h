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

#ifndef OPEN62541_EPICS_RECORD_ADDRESS_H
#define OPEN62541_EPICS_RECORD_ADDRESS_H

#include <cstdint>
#include <string>

extern "C" {
#include "open62541.h"
}

#include "UaNodeId.h"

namespace open62541 {
namespace epics {

/**
 * Record address for the open62541 device support.
 */
class Open62541RecordAddress {

public:

  /**
   * Conversion mode for ai / ao records.
   */
  enum class ConversionMode {

    /**
     * Automatically select conversion mode based on the OPC UA data type.
     */
    automatic,

    /**
     * Do convert. Use the RVAL field so that the record's conversion routines
     * apply.
     */
    convert,

    /**
     * Do not convert. Use the VAL field directly.
     */
    direct

  };

  /**
   * OPC UA data type that can be set as part of a record address.
   */
  enum class DataType {

    /**
     * No data type has been specified.
     */
    unspecified,

    /**
     * OPC UA Boolean type.
     */
    boolean,

    /**
     * OPC UA SByte type.
     */
    sbyte,

    /**
     * OPC UA Byte type.
     */
    byte,

    /**
     * OPC UA Int16 type.
     */
    int16,

    /**
     * OPC UA UInt16 type.
     */
    uint16,

    /**
     * OPC UA Int32 type.
     */
    int32,

    /**
     * OPC UA UInt32 type.
     */
    uint32,

    /**
     * OPC UA Int64 type.
     */
    int64,

    /**
     * OPC UA UInt64 type.
     */
    uint64,

    /**
     * OPC UA Float type.
     */
    floatType,

    /**
     * OPC UA Double type.
     */
    doubleType

  };

  /**
   * Returns the name of a data type.
   */
  static const char *nameForDataType(DataType dataType) {
    switch (dataType) {
    case DataType::boolean:
      return "Boolean";
    case DataType::sbyte:
      return "SByte";
    case DataType::byte:
      return "Byte";
    case DataType::int16:
      return "Int16";
    case DataType::uint16:
      return "UInt16";
    case DataType::int32:
      return "Int32";
    case DataType::uint32:
      return "UInt32";
    case DataType::int64:
      return "Int64";
    case DataType::uint64:
      return "UInt64";
    case DataType::floatType:
      return "Float";
    case DataType::doubleType:
      return "Double";
    default:
      return "<unknown>";
    }
  }

  /**
   * Creates a record address from a string. Throws an std::invalid_argument
   * exception if the address string does not specify a valid address.
   */
  Open62541RecordAddress(const std::string &addressString);

  /**
   * Returns the string identifying the connection.
   */
  inline const std::string &getConnectionId() const {
    return connectionId;
  }

  /**
   * Returns the selected conversion mode.
   */
  inline ConversionMode getConversionMode() const {
    return conversionMode;
  }

  /**
   * Returns the data-type specified for the node.
   */
  inline DataType getDataType() const {
    return dataType;
  }

  /**
   * Returns the node ID of the node to which the record is mapped..
   */
  inline const UaNodeId &getNodeId() const {
    return nodeId;
  }

  /**
   * Returns the sampling interval (in millisecond) that shall be used when
   * monitoring the node. For output records or input records that do not
   * operate in monitoring mode (SCAN is not set to I/O Intr), this setting does
   * not have any effects.
   *
   * If the address does not specify a sampling interval, NaN is returned. This
   * means that the sampling interval will be set to be the same as the
   * publishing interval of the associated subscription.
   */
  inline double getSamplingInterval() const {
    return samplingInterval;
  }

  /**
   * Returns the subscription that shal lbe used when monitoring the node. For
   * output records or innput records that do not operate in monitoring mode
   * (SCAN is not set to I/O Intr), this setting does not have any effects.
   *
   * If the address does not specify a subscription, "default" is returned.
   */
  inline std::string const &getSubscription() const {
    return subscription;
  }

  /**
   * Tells whether the record should be initialized with the value read from the
   * device. If <code>true</code>, the current value is read once during record
   * initialization. If <code>false</code>, the value is never read. This flag
   * is always <code>false</code> if the verify flag is <code>false</code>. For
   * input records, this flag does not have any effects.
   */
  inline bool isReadOnInit() const {
    return readOnInit;
  }

private:

  std::string connectionId;
  ConversionMode conversionMode;
  DataType dataType;
  UaNodeId nodeId;
  bool readOnInit;
  double samplingInterval;
  std::string subscription;

};

}
}

#endif // OPEN62541_EPICS_RECORD_ADDRESS_H
