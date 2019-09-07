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

#ifndef OPEN62541_EPICS_AAO_RECORD_H
#define OPEN62541_EPICS_AAO_RECORD_H

#include <cmath>

#include <aaoRecord.h>
#include <dbAccessDefs.h>
#include <dbStaticLib.h>
#include <menuFtype.h>

#include "Open62541OutputRecord.h"

namespace open62541 {
namespace epics {

// Anonymous namespace for internal functions.
namespace {

template<typename SourceType, typename DestinationType>
void aaoCopyArray(const SourceType *src, DestinationType *dst,
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
  // well-defined. When an array has zero elements, its pointer might not be
  // valid so that we have to test for this situation.
  if (numberOfDestinationElements != 0) {
    std::memset(dst + numberOfSourceElements, 0,
        numberOfDestinationElements - numberOfSourceElements);
  }
}

template<typename DestinationType>
UaVariant copyFromAaoRecordValue(::aaoRecord *record,
    const UA_DataType *dstType) {
  // This is just a safety check. The record support should already ensure that
  // this is always true.
  if (record->nord > record->nelm) {
    throw std::runtime_error("NORD is greater than NELM.");
  }
  // This code relies on the fact that copyArray only throws an exception if
  // the number of destination elements is less than the number of source
  // elements. As we pass the same number for both, we know that the function
  // is never going to throw.
  DestinationType *buffer = static_cast<DestinationType *>(UA_Array_new(
      record->nord, dstType));
  if (!buffer) {
    throw UaException(UA_STATUSCODE_BADOUTOFMEMORY);
  }
  switch (record->ftvl) {
  case menuFtypeCHAR:
    aaoCopyArray(static_cast<epicsInt8 *>(record->bptr), buffer, record->nord,
        record->nord);
    break;
  case menuFtypeUCHAR:
    aaoCopyArray(static_cast<epicsUInt8 *>(record->bptr), buffer, record->nord,
        record->nord);
    break;
  case menuFtypeSHORT:
    aaoCopyArray(static_cast<epicsInt16 *>(record->bptr), buffer, record->nord,
        record->nord);
    break;
  case menuFtypeUSHORT:
    aaoCopyArray(static_cast<epicsUInt16 *>(record->bptr), buffer, record->nord,
        record->nord);
    break;
  case menuFtypeLONG:
    aaoCopyArray(static_cast<epicsInt32 *>(record->bptr), buffer, record->nord,
        record->nord);
    break;
  case menuFtypeULONG:
    aaoCopyArray(static_cast<epicsUInt32 *>(record->bptr), buffer, record->nord,
        record->nord);
    break;
  case menuFtypeFLOAT:
    aaoCopyArray(static_cast<epicsFloat32 *>(record->bptr), buffer,
        record->nord, record->nord);
    break;
  case menuFtypeDOUBLE:
    aaoCopyArray(static_cast<epicsFloat64 *>(record->bptr), buffer,
        record->nord, record->nord);
    break;
  default:
    UA_Array_delete(buffer, record->nord, dstType);
    buffer = nullptr;
    throw std::runtime_error("Unsupported FTVL.");
  }
  // We use UA_Variant_setArray instead of UA_Variant_setArrayCopy. This way,
  // the actual data does not have to be copied again and the buffer that we
  // have allocated is deleted when the variant is deleted.
  UA_Variant dst;
  UA_Variant_init(&dst);
  UA_Variant_setArray(&dst, buffer, record->nord, dstType);
  return UaVariant(std::move(dst));
}

template<typename SourceType>
void copyToAaoRecordValue(::aaoRecord *record, const SourceType *src,
    std::size_t numberOfSourceElements) {
  switch (record->ftvl) {
  case menuFtypeCHAR:
    aaoCopyArray(src, static_cast<epicsInt8 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeUCHAR:
    aaoCopyArray(src, static_cast<epicsUInt8 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeSHORT:
    aaoCopyArray(src, static_cast<epicsInt16 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeUSHORT:
    aaoCopyArray(src, static_cast<epicsUInt16 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeLONG:
    aaoCopyArray(src, static_cast<epicsInt32 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeULONG:
    aaoCopyArray(src, static_cast<epicsUInt32 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeFLOAT:
    aaoCopyArray(src, static_cast<epicsFloat32 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  case menuFtypeDOUBLE:
    aaoCopyArray(src, static_cast<epicsFloat64 *>(record->bptr),
        numberOfSourceElements, record->nelm);
    break;
  default:
    throw std::runtime_error("Unsupported FTVL.");
  }
  record->nord = numberOfSourceElements;
}

}

/**
 * Device support class for the ao record.
 */
class Open62541AaoRecord: public Open62541OutputRecord<::aaoRecord> {

public:

  /**
   * Creates an instance of the device support for the specified record.
   */
  Open62541AaoRecord(::aaoRecord *record) :
      Open62541OutputRecord(record) {
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

  UaVariant readRecordValue() {
    const Open62541RecordAddress &address = getRecordAddress();
    ::aaoRecord *record = getRecord();
    Open62541RecordAddress::DataType dataType = address.getDataType();
    // If no data type has been specified, we assume that the OPC UA variable
    // of the same type as the array.
    if (dataType == Open62541RecordAddress::DataType::unspecified) {
      switch (record->ftvl) {
      case menuFtypeCHAR:
        dataType = Open62541RecordAddress::DataType::sbyte;
        break;
      case menuFtypeUCHAR:
        dataType = Open62541RecordAddress::DataType::byte;
        break;
      case menuFtypeSHORT:
        dataType = Open62541RecordAddress::DataType::int16;
        break;
      case menuFtypeUSHORT:
        dataType = Open62541RecordAddress::DataType::uint16;
        break;
      case menuFtypeLONG:
        dataType = Open62541RecordAddress::DataType::int32;
        break;
      case menuFtypeULONG:
        dataType = Open62541RecordAddress::DataType::uint32;
        break;
      case menuFtypeFLOAT:
        dataType = Open62541RecordAddress::DataType::floatType;
        break;
      case menuFtypeDOUBLE:
        dataType = Open62541RecordAddress::DataType::doubleType;
        break;
      default:
        throw std::runtime_error("Unsupported FTVL.");
      }
    }
    UaVariant value;
    // Obviously, some conversions (e.g. to boolean or to unsigned types) are
    // going to be lossy. However, we use the same logic for deciding whether
    // to assume conversion is enabled (if not specified explicitly) that we
    // also use for the ai record. This way, there is some symmetry which should
    // be easier to understand for users.
    switch (dataType) {
    case Open62541RecordAddress::DataType::boolean:
      value = copyFromAaoRecordValue<UA_Boolean>(record,
          &UA_TYPES[UA_TYPES_BOOLEAN]);
      break;
    case Open62541RecordAddress::DataType::sbyte:
      value = copyFromAaoRecordValue<UA_SByte>(record,
          &UA_TYPES[UA_TYPES_SBYTE]);
      break;
    case Open62541RecordAddress::DataType::byte:
      value = copyFromAaoRecordValue<UA_Byte>(record, &UA_TYPES[UA_TYPES_BYTE]);
      break;
    case Open62541RecordAddress::DataType::uint16:
      value = copyFromAaoRecordValue<UA_UInt16>(record,
          &UA_TYPES[UA_TYPES_UINT16]);
      break;
    case Open62541RecordAddress::DataType::int16:
      value = copyFromAaoRecordValue<UA_Int16>(record,
          &UA_TYPES[UA_TYPES_INT16]);
      break;
    case Open62541RecordAddress::DataType::uint32:
      value = copyFromAaoRecordValue<UA_UInt32>(record,
          &UA_TYPES[UA_TYPES_UINT32]);
      break;
    case Open62541RecordAddress::DataType::int32:
      value = copyFromAaoRecordValue<UA_Int32>(record,
          &UA_TYPES[UA_TYPES_INT32]);
      break;
    case Open62541RecordAddress::DataType::uint64:
      value = copyFromAaoRecordValue<UA_UInt64>(record,
          &UA_TYPES[UA_TYPES_UINT64]);
      break;
    case Open62541RecordAddress::DataType::int64:
      value = copyFromAaoRecordValue<UA_Int64>(record,
          &UA_TYPES[UA_TYPES_INT64]);
      break;
    case Open62541RecordAddress::DataType::floatType:
      value = copyFromAaoRecordValue<UA_Float>(record,
          &UA_TYPES[UA_TYPES_FLOAT]);
      break;
    case Open62541RecordAddress::DataType::doubleType:
      value = copyFromAaoRecordValue<UA_Double>(record,
          &UA_TYPES[UA_TYPES_DOUBLE]);
      break;
    default:
      throw std::runtime_error(
          std::string("Unsupported data type: ")
              + Open62541RecordAddress::nameForDataType(dataType));
      break;
    }
    return value;
  }

  void writeRecordValue(const UaVariant &value) {
    ::aaoRecord *record = getRecord();
    if (!value) {
      recGblSetSevr(record, READ_ALARM, INVALID_ALARM);
      throw std::runtime_error("Read variant is empty.");
    }
    if (value.isScalar()) {
      throw std::runtime_error(
          "Read variant is a scalar, but an array is needed.");
    }
    std::size_t numberOfSourceElements = value.getArrayLength();
    std::size_t numberOfDestinationElements = record->nelm;
    if (numberOfSourceElements > numberOfDestinationElements) {
      errorExtendedPrintf(
          "%s Read %d elements but record can only store %d elements, discarding extra elements.",
          record->name, numberOfSourceElements, numberOfDestinationElements);
      numberOfSourceElements = numberOfDestinationElements;
    }
    // The aao record support only allocates buffer memory if the device support
    // does not. This means that memory allocation happens after this method is
    // called. For this reason we have to allocate memory here. We still check
    // whether bptr has already been initialized in case the behavior of the aao
    // record changes in the future.
    if (!record->bptr) {
      record->bptr = callocMustSucceed(record->nelm, dbValueSize(record->ftvl),
          "aao: buffer calloc failed");
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
      copyToAaoRecordValue(record, value.getData<UA_Boolean>(),
          numberOfSourceElements);
      break;
    case UA_TYPES_SBYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::sbyte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      copyToAaoRecordValue(record, value.getData<UA_SByte>(),
          numberOfSourceElements);
      break;
    case UA_TYPES_BYTE:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::byte) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      copyToAaoRecordValue(record, value.getData<UA_Byte>(),
          numberOfSourceElements);
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
      copyToAaoRecordValue(record, value.getData<UA_UInt16>(),
          numberOfSourceElements);
      break;
    case UA_TYPES_INT16:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int16) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      copyToAaoRecordValue(record, value.getData<UA_Int16>(),
          numberOfSourceElements);
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
      copyToAaoRecordValue(record, value.getData<UA_UInt32>(),
          numberOfSourceElements);
      break;
    case UA_TYPES_INT32:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int32) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      copyToAaoRecordValue(record, value.getData<UA_Int32>(),
          numberOfSourceElements);
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
      copyToAaoRecordValue(record, value.getData<UA_UInt64>(),
          numberOfSourceElements);
      break;
    case UA_TYPES_INT64:
      if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
          && address.getDataType() != Open62541RecordAddress::DataType::int64) {
        throw std::runtime_error(
            std::string("Expected data type ")
                + Open62541RecordAddress::nameForDataType(address.getDataType())
                + " but got " + value.getType().typeName);
      }
      copyToAaoRecordValue(record, value.getData<UA_Int64>(),
          numberOfSourceElements);
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
      copyToAaoRecordValue(record, value.getData<UA_Float>(),
          numberOfSourceElements);
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
      copyToAaoRecordValue(record, value.getData<UA_Double>(),
          numberOfSourceElements);
      break;
    default:
      recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
      throw std::runtime_error(
          std::string("Received unsupported variant type ")
              + value.getType().typeName + ".");
    }
  }

private:

  // We do not want to allow copy or move construction or assignment.
  Open62541AaoRecord(const Open62541AaoRecord &) = delete;
  Open62541AaoRecord(Open62541AaoRecord &&) = delete;
  Open62541AaoRecord &operator=(const Open62541AaoRecord &) = delete;
  Open62541AaoRecord &operator=(Open62541AaoRecord &&) = delete;

};

}
}

#endif // OPEN62541_EPICS_AAO_RECORD_H
