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

#ifndef OPEN62541_EPICS_STRINGIN_RECORD_H
#define OPEN62541_EPICS_STRINGIN_RECORD_H

#include <cstring>

#include <stringinRecord.h>

#include "Open62541InputRecord.h"

namespace open62541 {
namespace epics {

/**
 * Device support class for the stringin record.
 */
class Open62541StringinRecord: public Open62541InputRecord<::stringinRecord> {

public:

  /**
   * Creates an instance of the device support for the stringin record.
   */
  Open62541StringinRecord(::stringinRecord *record) :
      Open62541InputRecord(record) {
    // We call this method here instead of the base constructor because it can
    // be (and is) overridden in child classes.
    validateRecordAddress();
  }

protected:

  /**
   * Validates the record address. In contrast to the implementation in the
   * parent class, this implementation checks that a data type supported by this
   * record (string or byte-string) is specified.
   */
  virtual void validateRecordAddress() {
    Open62541InputRecord<::stringinRecord>::validateRecordAddress();
    auto dataType = this->getRecordAddress().getDataType();
    if (dataType != Open62541RecordAddress::DataType::unspecified
        && dataType != Open62541RecordAddress::DataType::byteString
        && dataType != Open62541RecordAddress::DataType::string) {
      throw std::invalid_argument("String records only support string types.");
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
    case UA_TYPES_STRING: {
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
      // by the size of the VAL field (which is typically declared as char[40]).
      auto fieldSize = sizeof(this->getRecord()->val) - 1;
      auto copySize = (stringValueSize < fieldSize) ? stringValueSize : fieldSize;
      std::strncpy(
        this->getRecord()->val,
        reinterpret_cast<char const *>(stringValue->data), copySize);
      // We want to ensure that the resulting string is always null-terminated.
      this->getRecord()->val[copySize] = '\0';
      break;
    }
    case UA_TYPES_BYTESTRING: {
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
      // by the size of the VAL field (which is typically declared as char[40]).
      auto fieldSize = sizeof(this->getRecord()->val) - 1;
      auto copySize = (stringValueSize < fieldSize) ? stringValueSize : fieldSize;
      std::strncpy(
        this->getRecord()->val,
        reinterpret_cast<char const *>(stringValue->data), copySize);
      // We want to ensure that the resulting string is always null-terminated.
      this->getRecord()->val[copySize] = '\0';
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
  Open62541StringinRecord(const Open62541StringinRecord &) = delete;
  Open62541StringinRecord(Open62541StringinRecord &&) = delete;
  Open62541StringinRecord &operator=(const Open62541StringinRecord &) = delete;
  Open62541StringinRecord &operator=(Open62541StringinRecord &&) = delete;

};

}
}

#endif // OPEN62541_EPICS_STRINGIN_RECORD_H
