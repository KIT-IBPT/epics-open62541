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

#ifndef OPEN62541_EPICS_AAI_RECORD_H
#define OPEN62541_EPICS_AAI_RECORD_H

#include <cstring>

#include <aaiRecord.h>
#include <menuFtype.h>

#include "Open62541InputRecord.h"

namespace open62541 {
namespace epics {

// Anonymous namespace for internal functions.
namespace {

template<typename SourceType, typename DestinationType>
void aaiCopyArray(const SourceType *src, DestinationType *dst,
    std::size_t numberOfSourceElements,
    std::size_t numberOfDestinationElements) {
  if (numberOfSourceElements > numberOfDestinationElements) {
    throw std::invalid_argument(
        "Number of destination elements must be greater than or equal to number of source elements.");
  }
  // When an array has zero elements, its pointer might not be valid so that we
  // have to test for this situation.
  if (typeid(SourceType) == typeid(DestinationType)
      && numberOfSourceElements != 0) {
    std::memcpy(dst, src, sizeof(DestinationType) * numberOfSourceElements);
  } else {
    for (auto i = 0; i < numberOfSourceElements; ++i) {
      dst[i] = src[i];
    }
  }
  // We want to ensure that the remaining elements of the destination array are
  // well-defined.
  std::memset(dst + numberOfSourceElements, 0,
      numberOfDestinationElements - numberOfSourceElements);
}

template<typename SourceType>
void copyToAaiRecordValue(::aaiRecord *record, const SourceType *src,
    std::size_t numberOfSourceElements) {
  switch (record->ftvl) {
  case menuFtypeCHAR:
    aaiCopyArray(src, static_cast<epicsInt8 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeUCHAR:
    aaiCopyArray(src, static_cast<epicsUInt8 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeSHORT:
    aaiCopyArray(src, static_cast<epicsInt16 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeUSHORT:
    aaiCopyArray(src, static_cast<epicsUInt16 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeLONG:
    aaiCopyArray(src, static_cast<epicsInt32 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeULONG:
    aaiCopyArray(src, static_cast<epicsUInt32 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeFLOAT:
    aaiCopyArray(src, static_cast<epicsFloat32 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeDOUBLE:
    aaiCopyArray(src, static_cast<epicsFloat64 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  default:
    throw std::runtime_error("Unsupported FTVL.");
  }
  record->nord = numberOfSourceElements;
}

}

/**
 * Device support class for the aai record.
 */
class Open62541AaiRecord: public Open62541InputRecord<::aaiRecord> {

public:

  /**
   * Creates an instance of the device support for the aai record.
   */
  Open62541AaiRecord(::aaiRecord *record) :
      Open62541InputRecord<::aaiRecord>(record) {
    // We call this method here instead of the base constructor because it can
    // be (and is) overridden in child classes.
    validateRecordAddress();
    if (record->ftvl == menuFtypeSTRING) {
      throw std::invalid_argument("A FTVL of STRING is not supported.");
    } else if (record->ftvl == menuFtypeENUM) {
      throw std::invalid_argument("A FTVL of ENUM is not supported.");
    }
  }

protected:

  virtual void writeRecordValue(const UA_Variant &value) {
    ::aaiRecord *record = getRecord();
    if (UA_Variant_isEmpty(&value)) {
      recGblSetSevr(record, READ_ALARM, INVALID_ALARM);
      throw std::runtime_error("Read variant is empty.");
    }
    if (UA_Variant_isScalar(&value)) {
      throw std::runtime_error(
          "Read variant is a scalar, but an array is needed.");
    }
    std::size_t numberOfSourceElements = value.arrayLength;
    std::size_t numberOfDestinationElements = record->nelm;
    if (numberOfSourceElements > numberOfDestinationElements) {
      errorExtendedPrintf(
          "%s Read %d elements but record can only store %d elements, discarding extra elements.",
          record->name, numberOfSourceElements, numberOfDestinationElements);
      numberOfSourceElements = numberOfDestinationElements;
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
      copyToAaiRecordValue(record, static_cast<UA_Boolean *>(value.data),
          numberOfSourceElements);
      break;
    case UA_TYPES_SBYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::sbyte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      copyToAaiRecordValue(record, static_cast<UA_SByte *>(value.data),
          numberOfSourceElements);
      break;
    case UA_TYPES_BYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::byte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      copyToAaiRecordValue(record, static_cast<UA_Byte *>(value.data),
          numberOfSourceElements);
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
      copyToAaiRecordValue(record, static_cast<UA_UInt16 *>(value.data),
          numberOfSourceElements);
      break;
    case UA_TYPES_INT16:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int16) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      copyToAaiRecordValue(record, static_cast<UA_Int16 *>(value.data),
          numberOfSourceElements);
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
      copyToAaiRecordValue(record, static_cast<UA_UInt32 *>(value.data),
          numberOfSourceElements);
      break;
    case UA_TYPES_INT32:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int32) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      copyToAaiRecordValue(record, static_cast<UA_Int32 *>(value.data),
          numberOfSourceElements);
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
      copyToAaiRecordValue(record, static_cast<UA_UInt64 *>(value.data),
          numberOfSourceElements);
      break;
    case UA_TYPES_INT64:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int64) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.type->typeName);
      }
      copyToAaiRecordValue(record, static_cast<UA_Int64 *>(value.data),
          numberOfSourceElements);
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
      copyToAaiRecordValue(record, static_cast<UA_Float *>(value.data),
          numberOfSourceElements);
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
      copyToAaiRecordValue(record, static_cast<UA_Double *>(value.data),
          numberOfSourceElements);
      break;
    default:
      recGblSetSevr(record, READ_ALARM, INVALID_ALARM);
      throw std::runtime_error(
          std::string("Received unsupported variant type ")
              + value.type->typeName + ".");
    }
  }

private:

  // We do not want to allow copy or move construction or assignment.
  Open62541AaiRecord(const Open62541AaiRecord &) = delete;
  Open62541AaiRecord(Open62541AaiRecord &&) = delete;
  Open62541AaiRecord &operator=(const Open62541AaiRecord &) = delete;
  Open62541AaiRecord &operator=(Open62541AaiRecord &&) = delete;

};

}
}

#endif // OPEN62541_EPICS_AAI_RECORD_H
