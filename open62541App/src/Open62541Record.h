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

#ifndef OPEN62541_EPICS_RECORD_H
#define OPEN62541_EPICS_RECORD_H

#include <memory>
#include <stdexcept>

#include <callback.h>
#include <dbCommon.h>
#include <dbScan.h>

#include "Open62541RecordAddress.h"
#include "ServerConnectionRegistry.h"

namespace open62541 {
namespace epics {

/**
 * Base class for most EPICS record device support classes.
 */
template<typename RecordType>
class Open62541Record {

public:

  /**
   * Called once when the record is initialized. This method is called right
   * after an instance of this class has been created. It implements
   * initialization logic that should be run once, but which
   * (in case of failure) should not keep the record from being initialized.
   * All logic that is critical (so that the record cannot be used if it fails)
   * must be in the constructor.
   *
   * The default implementation does nothing.
   */
  virtual void initializeRecord() {
  }

  /**
   * Called each time the record is processed. Used for reading (input
   * records) or writing (output records) data from or to the hardware. The
   * default implementation of the processRecord method works asynchronously by
   * calling the {@link #processPrepare()} method and setting the PACT field to
   * one before returning. When it is called again later, PACT is reset to zero
   * and the {@link #processComplete} is called.
   */
  virtual void processRecord();

protected:

  /**
   * Constructor. The record is stored in an attribute of this class and
   * is used by all methods which want to access record fields.
   */
  Open62541Record(RecordType *record, const ::DBLINK &addressField);

  /**
   * Destructor.
   */
  virtual ~Open62541Record() {
  }

  /**
   * Returns the connection associated with this record.
   */
  inline std::shared_ptr<ServerConnection> getServerConnection() const {
    return connection;
  }

  /**
   * Returns the address associated with this record.
   */
  inline const Open62541RecordAddress &getRecordAddress() const {
    return address;
  }

  /**
   * Returns a pointer to the structure that holds the actual EPICS record.
   * Always returns a valid pointer.
   */
  inline RecordType *getRecord() const {
    return record;
  }

  /**
   * Called by the default implementation of the {@link #processRecord()}
   * method. This method should queue an asynchronous action that calls
   * {@link #scheduleProcessing()} when it finishes.
   */
  virtual void processPrepare() = 0;

  /**
   * Called by the default implementation of the {@link #processRecord()} method.
   * This method is called the second time the record is processed, after the
   * processing has been scheduled using {@link #scheduleProcessing()}. It
   * should update the record with the new value and / or error state.
   */
  virtual void processComplete() = 0;

  /**
   * Schedules processing of the record. This method should only be called from
   * an asynchronous callback that has been scheduled by the
   * {@link processPrepare()} method.
   */
  void scheduleProcessing();

  /**
   * Validates the record address. This method can be overridden by child
   * classes in order to modify the checks. The default implementation provided
   * by this base class rejects any record addresses that specify a conversion
   * mode (such a specification is only allowed for ai and ao records).
   */
  virtual void validateRecordAddress();

  /**
   * Updates the record's value with the specified value.
   */
  virtual void writeRecordValue(const UaVariant &value) = 0;

  /**
   * Generic implementation of writeRecordValue. Child classes can call this
   * method from their implementation of writeRecordValue(...) in order to
   * implement the write logic. This implementation assumes that there is only
   * a single value field and that there is an implicit conversion from all
   * supported (numeric) OPC UA types to the field's type.
   */
  template<typename ValueFieldType>
  void writeRecordValueGeneric(const UaVariant &value,
      ValueFieldType &valueField);

private:

  // We do not want to allow copy or move construction or assignment.
  Open62541Record(const Open62541Record &) = delete;
  Open62541Record(Open62541Record &&) = delete;
  Open62541Record &operator=(const Open62541Record &) = delete;
  Open62541Record &operator=(Open62541Record &&) = delete;

  /**
   * Address specified in the INP our OUT field of the record.
   */
  Open62541RecordAddress address;

  /**
   * Pointer to the server connection.
   */
  std::shared_ptr<ServerConnection> connection;

  /**
   * Record this device support has been instantiated for.
   */
  RecordType *record;

  /**
   * Callback needed to queue a request for processRecord to be run again.
   */
  ::CALLBACK processCallback;

  /**
   * Reads the record address from an address field.
   */
  static Open62541RecordAddress readRecordAddress(const ::DBLINK &addressField);

};

template<typename RecordType>
Open62541Record<RecordType>::Open62541Record(RecordType *record,
    const ::DBLINK &addressField) :
    address(readRecordAddress(addressField)), record(record) {
  this->connection =
      ServerConnectionRegistry::getInstance().getServerConnection(
          this->address.getConnectionId());
  if (!this->connection) {
    throw std::runtime_error(
        std::string("Could not find connection ")
            + this->address.getConnectionId() + ".");
  }
}

template<typename RecordType>
void Open62541Record<RecordType>::processRecord() {
  if (this->record->pact) {
    this->record->pact = false;
    processComplete();
  } else {
    processPrepare();
    this->record->pact = true;
  }
}

template<typename RecordType>
void Open62541Record<RecordType>::scheduleProcessing() {
  // Registering the callback establishes a happens-before relationship due to
  // an internal lock. Therefore, data written before registering the callback
  // is seen by the callback function.
  ::callbackRequestProcessCallback(&this->processCallback, priorityMedium,
      this->record);
}

template<typename RecordType>
void Open62541Record<RecordType>::validateRecordAddress() {
  const Open62541RecordAddress &address = getRecordAddress();
  if (address.getConversionMode()
      != Open62541RecordAddress::ConversionMode::automatic) {
    throw std::invalid_argument(
        "The conversion mode cannot be specified for this record type.");
  }
}

template<typename RecordType>
template<typename ValueFieldType>
void Open62541Record<RecordType>::writeRecordValueGeneric(
    const UaVariant &value, ValueFieldType &valueField) {
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
        && address.getDataType() != Open62541RecordAddress::DataType::boolean) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = (*value.getData<UA_Boolean>()) ? 1 : 0;
    break;
  case UA_TYPES_SBYTE:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::sbyte) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_SByte>();
    break;
  case UA_TYPES_BYTE:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::byte) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_Byte>();
    break;
  case UA_TYPES_UINT16:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::uint16) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_UInt16>();
    break;
  case UA_TYPES_INT16:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::int16) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_Int16>();
    break;
  case UA_TYPES_UINT32:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::uint32) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_UInt32>();
    break;
  case UA_TYPES_INT32:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::int32) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_Int32>();
    break;
  case UA_TYPES_UINT64:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::uint64) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_UInt64>();
    break;
  case UA_TYPES_INT64:
    if (address.getDataType() != Open62541RecordAddress::DataType::unspecified
        && address.getDataType() != Open62541RecordAddress::DataType::int64) {
      throw std::runtime_error(
          std::string("Expected data type ")
              + Open62541RecordAddress::nameForDataType(address.getDataType())
              + " but got " + value.getType().typeName);
    }
    valueField = *value.getData<UA_Int64>();
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
    valueField = *value.getData<UA_Float>();
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
    valueField = *value.getData<UA_Double>();
    break;
  default:
    recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
    throw std::runtime_error(
        std::string("Received unsupported variant type ") + value.getType().typeName
            + ".");
  }
}

template<typename RecordType>
Open62541RecordAddress Open62541Record<RecordType>::readRecordAddress(
    const ::DBLINK &addressField) {
  if (addressField.type != INST_IO) {
    throw std::runtime_error(
        "Invalid device address. Maybe mixed up INP/OUT or forgot '@'?");
  }
  return Open62541RecordAddress(
      addressField.value.instio.string == nullptr ?
          "" : addressField.value.instio.string);
}

}
}

#endif // OPEN62541_EPICS_RECORD_H
