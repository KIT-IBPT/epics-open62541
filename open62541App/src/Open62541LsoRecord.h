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

#ifndef OPEN62541_EPICS_LSO_RECORD_H
#define OPEN62541_EPICS_LSO_RECORD_H

#include <cstring>
#include <cstdint>

#include <lsoRecord.h>

#include "Open62541OutputRecord.h"

namespace open62541 {
namespace epics {

/**
 * Device support class for the lso record.
 */
class Open62541LsoRecord: public Open62541OutputRecord<::lsoRecord> {

public:

  /**
   * Creates an instance of the device support for the specified record.
   */
  Open62541LsoRecord(::lsoRecord *record) : Open62541OutputRecord(record) {
    // We call this method here instead of the base constructor because it can
    // be (and is) overridden in child classes.
    validateRecordAddress();
  }

protected:

  UaVariant readRecordValue() {
    const Open62541RecordAddress &address = getRecordAddress();
    Open62541RecordAddress::DataType dataType = address.getDataType();
    // If no data type has been specified, we assume that the OPC UA variable
    // is a string (probably the most frequent case for lso records).
    if (dataType == Open62541RecordAddress::DataType::unspecified) {
      dataType = Open62541RecordAddress::DataType::string;
    }
    // The lso record ensures that strings are always null-terminated.
    // We could still support strings that contain an intermediate null-byte,
    // but it is very likely that this would sometimes cause use to pick up
    // "dirt" left in memory from earlier uses, so we only use the portion of
    // the string up to the first null-byte.
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode status;
    switch (dataType) {
    case Open62541RecordAddress::DataType::string: {
      UA_String valueItem;
      auto &valField = getRecord()->val;
      valueItem.data = reinterpret_cast<std::uint8_t *>(valField);
      valueItem.length = getRecord()->len;
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_STRING]);
      break;
    }
    case Open62541RecordAddress::DataType::byteString: {
      UA_ByteString valueItem;
      auto &valField = getRecord()->val;
      valueItem.data = reinterpret_cast<std::uint8_t *>(valField);
      valueItem.length = getRecord()->len;
      status = UA_Variant_setScalarCopy(&value, &valueItem,
          &UA_TYPES[UA_TYPES_BYTESTRING]);
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
    return UaVariant(std::move(value));
  }

  /**
   * Validates the record address. In contrast to the implementation in the
   * parent class, this implementation checks that a data type supported by this
   * record (string or byte-string) is specified.
   */
  virtual void validateRecordAddress() {
    Open62541OutputRecord<::lsoRecord>::validateRecordAddress();
    auto dataType = this->getRecordAddress().getDataType();
    if (dataType != Open62541RecordAddress::DataType::unspecified
        && dataType != Open62541RecordAddress::DataType::byteString
        && dataType != Open62541RecordAddress::DataType::string) {
      throw std::invalid_argument("String records only support string types.");
    }
  }

  void writeRecordValue(const UaVariant &value) {
    if (!value) {
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error("Read variant is empty.");
    }
    if (!value.isScalar()) {
      throw std::runtime_error(
          "Read variant is an array, but a scalar is needed.");
    }
    const Open62541RecordAddress &address = getRecordAddress();
    switch (value.getType().typeKind) {
    case UA_DATATYPEKIND_STRING: {
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType()
              != Open62541RecordAddress::DataType::string) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      auto stringValue = value.getData<UA_String>();
      auto stringValueSize = stringValue->length;
      // The max. string size (including the terminating null byte) is determined
      // by the SIZV field.
      auto fieldSize = this->getRecord()->sizv - 1;
      auto copySize = (stringValueSize < fieldSize) ? stringValueSize : fieldSize;
      std::strncpy(
        this->getRecord()->val,
        reinterpret_cast<char const *>(stringValue->data), copySize);
      // We want to ensure that the resulting string is always null-terminated.
      this->getRecord()->val[copySize] = '\0';
      this->getRecord()->len = copySize;
      break;
    }
    case UA_DATATYPEKIND_BYTESTRING: {
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::byteString) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      auto stringValue = value.getData<UA_ByteString>();
      auto stringValueSize = stringValue->length;
      // The max. string size (including the terminating null byte) is determined
      // by the SIZV field.
      auto fieldSize = this->getRecord()->sizv - 1;
      auto copySize = (stringValueSize < fieldSize) ? stringValueSize : fieldSize;
      std::strncpy(
        this->getRecord()->val,
        reinterpret_cast<char const *>(stringValue->data), copySize);
      // We want to ensure that the resulting string is always null-terminated.
      this->getRecord()->val[copySize] = '\0';
      this->getRecord()->len = copySize;
      break;
    }
    default:
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error(
          std::string("Received unsupported variant type ")
              + value.getType().typeName + ".");
    }
  }

private:

  // We do not want to allow copy or move construction or assignment.
  Open62541LsoRecord(const Open62541LsoRecord &) = delete;
  Open62541LsoRecord(Open62541LsoRecord &&) = delete;
  Open62541LsoRecord &operator=(const Open62541LsoRecord &) = delete;
  Open62541LsoRecord &operator=(Open62541LsoRecord &&) = delete;

};

}
}

#endif // OPEN62541_EPICS_LSO_RECORD_H
