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

#ifndef OPEN62541_EPICS_AO_RECORD_H
#define OPEN62541_EPICS_AO_RECORD_H

#include <cmath>

#include <aoRecord.h>

#include "Open62541OutputRecord.h"

namespace open62541 {
namespace epics {

/**
 * Device support class for the ao record.
 */
class Open62541AoRecord: public Open62541OutputRecord<::aoRecord> {

public:

  /**
   * Creates an instance of the device support for the specified record.
   */
  Open62541AoRecord(::aoRecord *record) :
      Open62541OutputRecord(record), skipConversion(false) {
    // We call this method here instead of the base constructor because it can
    // be (and is) overridden in child classes.
    validateRecordAddress();
  }

  /**
   * Initializes the record's value by reading from the underlying OPC UA
   * variable. Returns 0 if the value has been written to the RVAL field and
   * should be converted and 2 if the value has been written to the VAL field
   * and should be used as-is.
   */
  long initializeAoRecord() {
    Open62541OutputRecord::initializeRecord();
    if (skipConversion) {
      return 2;
    } else {
      return 0;
    }
  }

protected:

  UA_Variant readRecordValue() {
    const Open62541RecordAddress &address = getRecordAddress();
    Open62541RecordAddress::DataType dataType = address.getDataType();
    // If no data type has been specified, we assume that the OPC UA variable
    // is a double (probably the most frequent case for ao records).
    if (dataType == Open62541RecordAddress::DataType::unspecified) {
      dataType = Open62541RecordAddress::DataType::doubleType;
    }
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode status;
    // Obviously, some conversions (e.g. to boolean or to unsigned types) are
    // going to be lossy. However, we use the same logic for deciding whether
    // to assume conversion is enabled (if not specified explicitly) that we
    // also use for the ai record. This way, there is some symmetry which should
    // be easier to understand for users.
    switch (dataType) {
    case Open62541RecordAddress::DataType::boolean: {
      UA_Boolean valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        valueItem = getRecord()->val != 0.0 && !std::isnan(getRecord()->val);
      } else {
        valueItem = getRecord()->rval;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_BOOLEAN]);
      break;
    }
    case Open62541RecordAddress::DataType::sbyte: {
      UA_SByte valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        valueItem = getRecord()->val;
      } else {
        valueItem = getRecord()->rval;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_SBYTE]);
      break;
    }
    case Open62541RecordAddress::DataType::byte: {
      UA_Byte valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        valueItem = getRecord()->val;
      } else {
        valueItem = getRecord()->rval;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_BYTE]);
      break;
    }
    case Open62541RecordAddress::DataType::uint16: {
      UA_UInt16 valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        valueItem = getRecord()->val;
      } else {
        valueItem = getRecord()->rval;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_UINT16]);
      break;
    }
    case Open62541RecordAddress::DataType::int16: {
      UA_Int16 valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        valueItem = getRecord()->val;
      } else {
        valueItem = getRecord()->rval;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_INT16]);
      break;
    }
    case Open62541RecordAddress::DataType::uint32: {
      UA_UInt32 valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        valueItem = getRecord()->rval;
      } else {
        valueItem = getRecord()->val;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_UINT32]);
      break;
    }
    case Open62541RecordAddress::DataType::int32: {
      UA_Int32 valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        valueItem = getRecord()->val;
      } else {
        valueItem = getRecord()->rval;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_INT32]);
      break;
    }
    case Open62541RecordAddress::DataType::uint64: {
      UA_UInt64 valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        valueItem = getRecord()->rval;
      } else {
        valueItem = getRecord()->val;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_UINT64]);
      break;
    }
    case Open62541RecordAddress::DataType::int64: {
      UA_Int64 valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        valueItem = getRecord()->rval;
      } else {
        valueItem = getRecord()->val;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_INT64]);
      break;
    }
    case Open62541RecordAddress::DataType::floatType: {
      UA_Float valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        valueItem = getRecord()->rval;
      } else {
        valueItem = getRecord()->val;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_FLOAT]);
      break;
    }
    case Open62541RecordAddress::DataType::doubleType: {
      UA_Double valueItem;
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        valueItem = getRecord()->rval;
      } else {
        valueItem = getRecord()->val;
      }
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_DOUBLE]);
      break;
    }
    default:
      throw std::runtime_error(
          std::string("Unsupported data type: ")
              + Open62541RecordAddress::nameForDataType(dataType));
      break;
    }
    if (status != UA_STATUSCODE_GOOD) {
      throw UaException(status);
    }
    return value;
  }

  /**
   * Validates the record address. In contrast to the implementation in the
   * parent class, this implementation actually allows a conversion mode to be
   * selected.
   */
  virtual void validateRecordAddress() {
    // We actually do not need any checks here.
  }

  void writeRecordValue(const UA_Variant &value) {
    if (UA_Variant_isEmpty(&value)) {
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error("Read variant is empty.");
    }
    if (!UA_Variant_isScalar(&value)) {
      throw std::runtime_error(
          "Read variant is an array, but a scalar is needed.");
    }
    const Open62541RecordAddress &address = getRecordAddress();
    switch (value.type->typeIndex) {
    case UA_TYPES_BOOLEAN:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::boolean) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val =
            (*static_cast<UA_Boolean *>(value.data)) ? 1.0 : 0.0;
        skipConversion = true;
      } else {
        this->getRecord()->rval =
            (*static_cast<UA_Boolean *>(value.data)) ? 1 : 0;
      }
      break;
    case UA_TYPES_SBYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::sbyte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *static_cast<UA_SByte *>(value.data);
        skipConversion = true;
      } else {
        this->getRecord()->rval = *static_cast<UA_SByte *>(value.data);
      }
      break;
    case UA_TYPES_BYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::byte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *static_cast<UA_Byte *>(value.data);
        skipConversion = true;
      } else {
        this->getRecord()->rval = *static_cast<UA_Byte *>(value.data);
      }
      break;
    case UA_TYPES_UINT16:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::uint16) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *static_cast<UA_UInt16 *>(value.data);
        skipConversion = true;
      } else {
        this->getRecord()->rval = *static_cast<UA_UInt16 *>(value.data);
      }
      break;
    case UA_TYPES_INT16:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int16) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *static_cast<UA_Int16 *>(value.data);
        skipConversion = true;
      } else {
        this->getRecord()->rval = *static_cast<UA_Int16 *>(value.data);
      }
      break;
    case UA_TYPES_UINT32:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::uint32) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      // When using the UInt32 data type, we use the direct mode as a default.
      // A UInt32 might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *static_cast<UA_UInt32 *>(value.data);
      } else {
        this->getRecord()->val = *static_cast<UA_UInt32 *>(value.data);
        skipConversion = true;
      }
      break;
    case UA_TYPES_INT32:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int32) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *static_cast<UA_Int32 *>(value.data);
        skipConversion = true;
      } else {
        this->getRecord()->rval = *static_cast<UA_Int32 *>(value.data);
      }
      break;
    case UA_TYPES_UINT64:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::uint64) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      // When using the UInt64 data type, we use the direct mode as a default.
      // A UInt64 might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *static_cast<UA_UInt64 *>(value.data);
      } else {
        this->getRecord()->val = *static_cast<UA_UInt64 *>(value.data);
        skipConversion = true;
      }
      break;
    case UA_TYPES_INT64:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int64) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      // When using the Int64 data type, we use the direct mode as a default.
      // An Int64 might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *static_cast<UA_Int64 *>(value.data);
      } else {
        this->getRecord()->val = *static_cast<UA_Int64 *>(value.data);
        skipConversion = true;
      }
      break;
    case UA_TYPES_FLOAT:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::floatType) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      // When using the Float data type, we use the direct mode as a default.
      // A Float might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *static_cast<UA_Float *>(value.data);
      } else {
        this->getRecord()->val = *static_cast<UA_Float *>(value.data);
        skipConversion = true;
      }
      break;
    case UA_TYPES_DOUBLE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::doubleType) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      // When using the Double data type, we use the direct mode as a default.
      // A Double might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *static_cast<UA_Double *>(value.data);
      } else {
        this->getRecord()->val = *static_cast<UA_Double *>(value.data);
        skipConversion = true;
      }
      break;
    default:
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error(
          std::string("Received unsupported variant type ")
              + value.type->typeName + ".");
    }
  }

private:

  // We do not want to allow copy or move construction or assignment.
  Open62541AoRecord(const Open62541AoRecord &) = delete;
  Open62541AoRecord(Open62541AoRecord &&) = delete;
  Open62541AoRecord &operator=(const Open62541AoRecord &) = delete;
  Open62541AoRecord &operator=(Open62541AoRecord &&) = delete;

  bool skipConversion;

};

}
}

#endif // OPEN62541_EPICS_AO_RECORD_H
