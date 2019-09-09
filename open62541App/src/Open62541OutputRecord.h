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

#ifndef OPEN62541_EPICS_OUTPUT_RECORD_H
#define OPEN62541_EPICS_OUTPUT_RECORD_H

#include <alarm.h>
#include <recGbl.h>

#include "open62541Error.h"
#include "Open62541Record.h"
#include "UaException.h"

namespace open62541 {
namespace epics {

/**
 * Base class for most device support classes belonging to EPICS input records.
 */
template<typename RecordType>
class Open62541OutputRecord: public Open62541Record<RecordType> {

public:

  /**
   * Initializes the records value with the current value read from the OPC UA
   * server. If the record address specifies that no initialization is desired,
   * the initialization is skipped. This method is called right after creating
   * the instance of this class.
   */
  virtual void initializeRecord();

protected:

  /**
   * Creates an instance of the device support class for the specified record
   * instance.
   */
  Open62541OutputRecord(RecordType *record);

  /**
   * Destructor.
   */
  virtual ~Open62541OutputRecord() {
  }

  /**
   * Reads and returns the record's current value. The caller is responsible for
   * freeing the memory associated with the returned variant.
   */
  virtual UaVariant readRecordValue() = 0;

  /**
   * Generic implementation of readRecordValue. Child classes can call this
   * method from their implementation of readRecordValue(...) in order to
   * implement the read logic. This implementation assumes that there is only
   * a single value field and that there is an implicit conversion from the
   * field's type to all supported OPC UA types.
   *
   * If the record address does not specify an OPC UA data-type, the passed
   * default data-type is used.
   *
   * The caller is responsible for freeing the memory associated with the
   * returned variant.
   */
  template<typename ValueFieldType>
  UaVariant readRecordValueGeneric(const ValueFieldType &valueField,
      Open62541RecordAddress::DataType defaultDataType);

  virtual bool processPrepare();

  virtual void processComplete();

private:

  struct CallbackImpl: ServerConnection::WriteCallback {
    CallbackImpl(Open62541OutputRecord &record);
    void success(const UaNodeId &nodeId);
    void failure(const UaNodeId &nodeId, UA_StatusCode statusCode);

    // In EPICS, records are never destroyed. Therefore, we can safely keep a
    // reference to the device support object.
    Open62541OutputRecord &record;
  };

  // We do not want to allow copy or move construction or assignment.
  Open62541OutputRecord(const Open62541OutputRecord &) = delete;
  Open62541OutputRecord(Open62541OutputRecord &&) = delete;
  Open62541OutputRecord &operator=(const Open62541OutputRecord &) = delete;
  Open62541OutputRecord &operator=(Open62541OutputRecord &&) = delete;

  bool writeSuccessful;
  std::string writeErrorMessage;

};

template<typename RecordType>
Open62541OutputRecord<RecordType>::Open62541OutputRecord(RecordType *record) :
    Open62541Record<RecordType>(record, record->out), writeSuccessful(false) {
}

template<typename RecordType>
void Open62541OutputRecord<RecordType>::initializeRecord() {
  Open62541Record<RecordType>::initializeRecord();
  if (this->getRecordAddress().isReadOnInit()) {
    UaVariant value;
    try {
      value = this->getServerConnection()->read(
        this->getRecordAddress().getNodeId());
    } catch (const UaException &e) {
      errorExtendedPrintf("%s Could not initialize record value: %s",
          this->getRecord()->name, e.what());
      return;
    }
    this->writeRecordValue(value);
    // The record's value has been initialized, therefore it is not undefined
    // any longer.
    this->getRecord()->udf = 0;
    // We have to reset the alarm state explicitly, so that the record is not
    // marked as invalid. This is not optimal because the record will not be
    // placed in an alarm state if the value would usually trigger an alarm.
    // However, alarms on output records are uncommon so this should be fine.
    // We also update the time stamp so that it represents the current time.
    recGblGetTimeStamp(this->getRecord());
    recGblResetAlarms(this->getRecord());
  }
}

template<typename RecordType>
template<typename ValueFieldType>
UaVariant Open62541OutputRecord<RecordType>::readRecordValueGeneric(
    const ValueFieldType &valueField,
    Open62541RecordAddress::DataType defaultDataType) {
  Open62541RecordAddress::DataType dataType =
      this->getRecordAddress().getDataType();
  if (dataType == Open62541RecordAddress::DataType::unspecified) {
    dataType = defaultDataType;
  }
  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode status;
  switch (dataType) {
  case Open62541RecordAddress::DataType::boolean: {
    UA_Boolean valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    break;
  }
  case Open62541RecordAddress::DataType::sbyte: {
    UA_SByte valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_SBYTE]);
    break;
  }
  case Open62541RecordAddress::DataType::byte: {
    UA_Byte valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_BYTE]);
    break;
  }
  case Open62541RecordAddress::DataType::uint16: {
    UA_UInt16 valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_UINT16]);
    break;
  }
  case Open62541RecordAddress::DataType::int16: {
    UA_Int16 valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_INT16]);
    break;
  }
  case Open62541RecordAddress::DataType::uint32: {
    UA_UInt32 valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_UINT32]);
    break;
  }
  case Open62541RecordAddress::DataType::int32: {
    UA_Int32 valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_INT32]);
    break;
  }
  case Open62541RecordAddress::DataType::uint64: {
    UA_UInt64 valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_UINT64]);
    break;
  }
  case Open62541RecordAddress::DataType::int64: {
    UA_Int64 valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_INT64]);
    break;
  }
  case Open62541RecordAddress::DataType::floatType: {
    UA_Float valueItem = valueField;
    status = UA_Variant_setScalarCopy(&value, &valueItem,
        &UA_TYPES[UA_TYPES_FLOAT]);
    break;
  }
  case Open62541RecordAddress::DataType::doubleType: {
    UA_Double valueItem = valueField;
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
  return UaVariant(std::move(value));
}

template<typename RecordType>
bool Open62541OutputRecord<RecordType>::processPrepare() {
  UaVariant value = this->readRecordValue();
  auto callback = std::make_shared<CallbackImpl>(*this);
  this->getServerConnection()->writeAsync(
      this->getRecordAddress().getNodeId(), value, callback);
  return true;
}

template<typename RecordType>
void Open62541OutputRecord<RecordType>::processComplete() {
  if (!writeSuccessful) {
    recGblSetSevr(this->getRecord(), WRITE_ALARM, INVALID_ALARM);
    throw std::runtime_error(writeErrorMessage);
  }
}

template<typename RecordType>
Open62541OutputRecord<RecordType>::CallbackImpl::CallbackImpl(
    Open62541OutputRecord &record) :
    record(record) {
}

template<typename RecordType>
void Open62541OutputRecord<RecordType>::CallbackImpl::success(
    const UaNodeId &nodeId) {
  record.writeSuccessful = true;
  record.scheduleProcessing();
}

template<typename RecordType>
void Open62541OutputRecord<RecordType>::CallbackImpl::failure(
    const UaNodeId &nodeId, UA_StatusCode statusCode) {
  record.writeSuccessful = false;
  try {
    record.writeErrorMessage = std::string("Error write to node: ")
        + UA_StatusCode_name(statusCode);
  } catch (...) {
    // We want to schedule processing of the record even if we cannot assemble
    // the error message for some obscure reason.
  }
  record.scheduleProcessing();
}

}
}

#endif // OPEN62541_EPICS_OUTPUT_RECORD_H
