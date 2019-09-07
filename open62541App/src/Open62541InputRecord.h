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

#ifndef OPEN62541_EPICS_INPUT_RECORD_H
#define OPEN62541_EPICS_INPUT_RECORD_H

#include <string>

#include <alarm.h>
#include <recGbl.h>

#include "Open62541Record.h"
#include "open62541Error.h"

namespace open62541 {
namespace epics {

/**
 * Base class for most device support classes belonging to EPICS input records.
 */
template<typename RecordType>
class Open62541InputRecord: public Open62541Record<RecordType> {

protected:

  /**
   * Creates an instance of the device support class for the specified record
   * instance.
   */
  Open62541InputRecord(RecordType *record) :
      Open62541Record<RecordType>(record, record->inp), readSuccessful(false) {
  }

  /**
   * Destructor.
   */
  virtual ~Open62541InputRecord() {
  }

  virtual void processPrepare();

  virtual void processComplete();

  /**
   * Validates the record address. In addition to the checks made by the base
   * class, this method also checks that the no_read_on_init flag is not set.
   * This flag is only allowed for output records.
   */
  virtual void validateRecordAddress() {
    Open62541Record<RecordType>::validateRecordAddress();
    const Open62541RecordAddress &address { this->getRecordAddress() };
    if (!address.isReadOnInit()) {
      throw std::invalid_argument(
          "The no_read_on_init flag is not supported for input records.");
    }
  }

private:

  struct CallbackImpl: ServerConnection::ReadCallback {
    CallbackImpl(Open62541InputRecord &record);
    void success(const UaNodeId &nodeId, const UaVariant &value);
    void failure(const UaNodeId &nodeId, UA_StatusCode statusCode);

    // In EPICS, records are never destroyed. Therefore, we can safely keep a
    // reference to the device support object.
    Open62541InputRecord &record;
  };

  // We do not want to allow copy or move construction or assignment.
  Open62541InputRecord(const Open62541InputRecord &) = delete;
  Open62541InputRecord(Open62541InputRecord &&) = delete;
  Open62541InputRecord &operator=(const Open62541InputRecord &) = delete;
  Open62541InputRecord &operator=(Open62541InputRecord &&) = delete;

  bool readSuccessful;
  UaVariant readValue;
  std::string readErrorMessage;

};

template<typename RecordType>
void Open62541InputRecord<RecordType>::processPrepare() {
  auto callback = std::make_shared<CallbackImpl>(*this);
  this->getServerConnection()->readAsync(this->getRecordAddress().getNodeId(),
      callback);
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::processComplete() {
  if (readSuccessful) {
    // Unset UDF flag, because this might have been the first time that
    // the record has been read.
    this->getRecord()->udf = 0;
    this->writeRecordValue(readValue);
  } else {
    recGblSetSevr(this->getRecord(), READ_ALARM, INVALID_ALARM);
    throw std::runtime_error(readErrorMessage);
  }
}

template<typename RecordType>
Open62541InputRecord<RecordType>::CallbackImpl::CallbackImpl(
    Open62541InputRecord &record) :
    record(record) {
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::CallbackImpl::success(
    const UaNodeId &nodeId, const UaVariant &value) {
  record.readSuccessful = true;
  record.readValue = value;
  record.scheduleProcessing();
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::CallbackImpl::failure(
    const UaNodeId &nodeId, UA_StatusCode statusCode) {
  record.readSuccessful = false;
  try {
    record.readErrorMessage = std::string("Error reading from node: ")
        + UA_StatusCode_name(statusCode);
  } catch (...) {
    // We want to schedule processing of the record even if we cannot assemble
    // the error message for some obscure reason.
  }
  record.scheduleProcessing();
}

}
}

#endif // OPEN62541_EPICS_INPUT_RECORD_H
