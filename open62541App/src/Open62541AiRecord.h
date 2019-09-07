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

#ifndef OPEN62541_EPICS_AI_RECORD_H
#define OPEN62541_EPICS_AI_RECORD_H

#include <aiRecord.h>

#include "Open62541InputRecord.h"

namespace open62541 {
namespace epics {

/**
 * Device support class for the ai record.
 */
class Open62541AiRecord: public Open62541InputRecord<::aiRecord> {

public:

  /**
   * Creates an instance of the device support for the ai record.
   */
  Open62541AiRecord(::aiRecord *record) :
      Open62541InputRecord<::aiRecord>(record), skipConversion(false) {
    // We call this method here instead of the base constructor because it can
    // be (and is) overridden in child classes.
    validateRecordAddress();
  }

  /**
   * Processes the record. This is a wrapper around the regular processRecord
   * method that also signals whether the value in the RVAL field should be
   * converted in order to to calculate the value of the VAL field or whether
   * the value of the VAL field should be used as-is.
   */
  long processAiRecord() {
    skipConversion = false;
    processRecord();
    if (skipConversion) {
      return 2;
    } else {
      return 0;
    }
  }

protected:

  /**
   * Validates the record address. In contrast to the implementation in the
   * parent class, this implementation actually allows a conversion mode to be
   * selected.
   */
  virtual void validateRecordAddress() {
    const Open62541RecordAddress &address { this->getRecordAddress() };
    if (!address.isReadOnInit()) {
      throw std::invalid_argument(
          "The no_read_on_init flag is not supported for input records.");
    }
  }

  virtual void writeRecordValue(const UaVariant &value) {
    if (!value) {
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error("Read variant is empty.");
    }
    if (!value.isScalar()) {
      throw std::runtime_error(
          "Read variant is an array, but a scalar is needed.");
    }
    const Open62541RecordAddress &address = getRecordAddress();
    switch (value.getType().typeIndex) {
    case UA_TYPES_BOOLEAN:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::boolean) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val =
            (*value.getData<UA_Boolean>()) ? 1.0 : 0.0;
        skipConversion = true;
      } else {
        this->getRecord()->rval =
            (*value.getData<UA_Boolean>()) ? 1 : 0;
      }
      break;
    case UA_TYPES_SBYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::sbyte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *value.getData<UA_SByte>();
        skipConversion = true;
      } else {
        this->getRecord()->rval = *value.getData<UA_SByte>();
      }
      break;
    case UA_TYPES_BYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::byte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *value.getData<UA_Byte>();
        skipConversion = true;
      } else {
        this->getRecord()->rval = *value.getData<UA_Byte>();
      }
      break;
    case UA_TYPES_UINT16:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::uint16) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *value.getData<UA_UInt16>();
        skipConversion = true;
      } else {
        this->getRecord()->rval = *value.getData<UA_UInt16>();
      }
      break;
    case UA_TYPES_INT16:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int16) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *value.getData<UA_Int16>();
        skipConversion = true;
      } else {
        this->getRecord()->rval = *value.getData<UA_Int16>();
      }
      break;
    case UA_TYPES_UINT32:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::uint32) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      // When using the UInt32 data type, we use the direct mode as a default.
      // A UInt32 might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *value.getData<UA_UInt32>();
      } else {
        this->getRecord()->val = *value.getData<UA_UInt32>();
        skipConversion = true;
      }
      break;
    case UA_TYPES_INT32:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int32) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::direct) {
        this->getRecord()->val = *value.getData<UA_Int32>();
        skipConversion = true;
      } else {
        this->getRecord()->rval = *value.getData<UA_Int32>();
      }
      break;
    case UA_TYPES_UINT64:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::uint64) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      // When using the UInt64 data type, we use the direct mode as a default.
      // A UInt64 might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *value.getData<UA_UInt64>();
      } else {
        this->getRecord()->val = *value.getData<UA_UInt64>();
        skipConversion = true;
      }
      break;
    case UA_TYPES_INT64:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int64) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      // When using the Int64 data type, we use the direct mode as a default.
      // An Int64 might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *value.getData<UA_Int64>();
      } else {
        this->getRecord()->val = *value.getData<UA_Int64>();
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
                + " but got " + value.getType().typeName);
      }
      // When using the Float data type, we use the direct mode as a default.
      // A Float might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *value.getData<UA_Float>();
      } else {
        this->getRecord()->val = *value.getData<UA_Float>();
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
                + " but got " + value.getType().typeName);
      }
      // When using the Double data type, we use the direct mode as a default.
      // A Double might not fit into an EPICS long, so directly writing to VAL
      // makes sense.
      if (address.getConversionMode()
          == Open62541RecordAddress::ConversionMode::convert) {
        this->getRecord()->rval = *value.getData<UA_Double>();
      } else {
        this->getRecord()->val = *value.getData<UA_Double>();
        skipConversion = true;
      }
      break;
    default:
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error(
          std::string("Received unsupported variant type ")
              + value.getType().typeName + ".");
    }
  }

private:

  bool skipConversion;

  // We do not want to allow copy or move construction or assignment.
  Open62541AiRecord(const Open62541AiRecord &) = delete;
  Open62541AiRecord(Open62541AiRecord &&) = delete;
  Open62541AiRecord &operator=(const Open62541AiRecord &) = delete;
  Open62541AiRecord &operator=(Open62541AiRecord &&) = delete;

};

}
}

#endif // OPEN62541_EPICS_AI_RECORD_H
