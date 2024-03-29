/*
 * Copyright 2017-2024 aquenos GmbH.
 * Copyright 2017-2024 Karlsruhe Institute of Technology.
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

#include <cmath>
#include <mutex>
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

public:

  /**
   * Called when a record is switched to or from "I/O Intr" mode. When enabling
   * the I/O Intr mode, command is 0. Otherwise, it is 1. This method initializes
   * the passed IOSCANPVT structure with the information from the device supports
   * IOSCANPVT structure.
   *
   * Effectively, this method enables or disables the use of a monitored item
   * (in contrast to the regular polling) for reading OPC UA node backing the
   * record.
   */
  virtual void getInterruptInfo(int command, ::IOSCANPVT *iopvt) {
    // A command value of 0 means enable I/O Intr mode, a value of 1 means
    // disable.
    // We have to remember whether monitoring is enabled. We have to update this
    // flag while holding a lock on the mutex because this flag is also accessed
    // by the monitor callback. Note that we do this before adding or removing
    // the monitored item. If we did it later, we might receive a callback with
    // the flag still being in the wrong state.
    {
      std::lock_guard<std::mutex> lock(monitoringMutex);
      monitoringEnabled = !command;
      // We reset the monitoringFirstEventReceived flag because events might
      // already have been received when the record has been in I/O Intr mode
      // previously, but we do not want these events to count when checking
      // whether an event has already been received for the current monitor.
      monitoringFirstEventReceived = false;
    }
    std::string const &subscriptionName =
      this->getRecordAddress().getSubscription();
    if (command == 0) {
      double samplingInterval = this->getRecordAddress().getSamplingInterval();
      if (std::isnan(samplingInterval)) {
        samplingInterval =
          this->getServerConnection()->getSubscriptionPublishingInterval(
            subscriptionName);
      }
      // We use a fixed queue size of one and set the discard-oldest flag. As we
      // do not use a queue for the record and notifications are delivered in
      // bursts, we would most likely discard any additional items delivered by
      // the server anyway.
      std::uint32_t queueSize = 1;
      bool discardOldest = true;
      this->getServerConnection()->addMonitoredItem(
        subscriptionName, this->getRecordAddress().getNodeId(),
        monitoredItemCallback, samplingInterval, queueSize, discardOldest);
    } else {
      this->getServerConnection()->removeMonitoredItem(
        subscriptionName, this->getRecordAddress().getNodeId(),
        monitoredItemCallback);
    }
    *iopvt = this->ioIntrModeScanPvt;
  }

protected:

  /**
   * Creates an instance of the device support class for the specified record
   * instance.
   */
  Open62541InputRecord(RecordType *record) :
      Open62541Record<RecordType>(record, record->inp),
      monitoredItemCallback(std::make_shared<MonitoredItemCallbackImpl>(*this)),
      monitoringEnabled(false), monitoringFirstEventReceived(false),
      readSuccessful(false) {
    ::scanIoInit(&this->ioIntrModeScanPvt);
  }

  /**
   * Destructor.
   */
  virtual ~Open62541InputRecord() {
  }

  virtual bool processPrepare();

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

  struct MonitoredItemCallbackImpl : ServerConnection::MonitoredItemCallback {
    MonitoredItemCallbackImpl(Open62541InputRecord &record);
    void success(const UaNodeId &nodeId, const UaVariant &value);
    void failure(const UaNodeId &nodeId, UA_StatusCode statusCode);

    // In EPICS, records are never destroyed. Therefore, we can safely keep a
    // reference to the device support object.
    Open62541InputRecord &record;
  };

  struct ReadCallbackImpl: ServerConnection::ReadCallback {
    ReadCallbackImpl(Open62541InputRecord &record);
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

  ::IOSCANPVT ioIntrModeScanPvt;
  std::shared_ptr<MonitoredItemCallbackImpl> monitoredItemCallback;
  bool monitoringEnabled;
  bool monitoringFirstEventReceived;
  std::mutex monitoringMutex;
  std::string readErrorMessage;
  bool readSuccessful;
  UaVariant readValue;

};

template<typename RecordType>
bool Open62541InputRecord<RecordType>::processPrepare() {
  // If monitoring is enabled and the first value has been received, this
  // function is most likeley called as the result of calling scanIoRequest. In
  // this case, we do not poll the value from the server and instead use the
  // last received value.
  // There is a small chance that this function is called more than once
  // without the value having changed in between. This should only happen when
  // the server is overloaded and there is backlog of processing requests. In
  // this case, we simply reuse the latest value, because any other approach
  // would be much more complicated (e.g. using a queue of received values).
  // We need to hold the monitoringMutext when checking the
  // monitoringFirstEventReceived flag because it might be concurrently
  // modified by the callback. We also have to hold it when calling
  // processComplete, because the actual value might also be modified by the
  // callback.
  // We can check the monitoringEnabled flag without holding the mutex because
  // this flag is only modified in getIoIntInfo and synchronization in EPICS
  // Base ensures that calls to that function and processRecord() are
  // serialized.
  if (monitoringEnabled) {
    std::lock_guard<std::mutex> lock(monitoringMutex);
    // If we have not received an event yet, we completely ignore the
    // processing request, keeping the last value and keeping the record in an
    // undefined state if it has not been processed yet.
    if (monitoringFirstEventReceived) {
      processComplete();
    }
    return false;
  }
  auto callback = std::make_shared<ReadCallbackImpl>(*this);
  this->getServerConnection()->readAsync(this->getRecordAddress().getNodeId(),
      callback);
  return true;
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
Open62541InputRecord<RecordType>::MonitoredItemCallbackImpl::MonitoredItemCallbackImpl(
    Open62541InputRecord &record) :
    record(record) {
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::MonitoredItemCallbackImpl::success(
    const UaNodeId &nodeId, const UaVariant &value) {
  // Notifications happen asynchronously, so we have to hold a lock on the mutex
  // in order to avoid a race condition when getInterruptInfo or process are
  // being called concurrently by a different thread.
  std::lock_guard<std::mutex> lock(record.monitoringMutex);
  // It could happen that we receive notifications even though the monitored
  // item has been removed. The reason for this is that removal of the monitored
  // item happens asynchronously. For this reason, we check whether monitoring
  // is still enabled for this record and discard notifications if it is not.
  if (!record.monitoringEnabled) {
    return;
  }
  record.monitoringFirstEventReceived = true;
  record.readSuccessful = true;
  record.readValue = value;
  // There is a small chance that scanIoRequest will fail because the queues are
  // already full (it will return zero in that case).
  // The most likely case when scanIoRequest will fail is when the IOC has not
  // been fully initialized yet. In this case, calling scheduleProcessing will
  // usually work. If this does not work either, we print an error message.
  if (!::scanIoRequest(record.ioIntrModeScanPvt)) {
    if (!record.scheduleProcessing()) {
      errorExtendedPrintf(
        "%s Could not schedule asynchronous processing of record. Monitored item notification is not going to be processed.",
        record.getRecord()->name);
    }
  }
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::MonitoredItemCallbackImpl::failure(
    const UaNodeId &nodeId, UA_StatusCode statusCode) {
  // Notifications happen asynchronously, so we have to hold a lock on the mutex
  // in order to avoid a race condition when getInterruptInfo or process are
  // being called concurrently by a different thread.
  std::lock_guard<std::mutex> lock(record.monitoringMutex);
  // It could happen that we receive notifications even though the monitored
  // item has been removed. The reason for this is that removal of the monitored
  // item happens asynchronously. For this reason, we check whether monitoring
  // is still enabled for this record and discard notifications if it is not.
  if (!record.monitoringEnabled) {
    return;
  }
  record.monitoringFirstEventReceived = true;
  record.readSuccessful = false;
  try {
    record.readErrorMessage = std::string("Error monitoring node: ")
        + UA_StatusCode_name(statusCode);
  } catch (...) {
    // We want to schedule processing of the record even if we cannot assemble
    // the error message for some obscure reason.
  }
  // There is a small chance that scanIoRequest will fail because the queues are
  // already full (it will return zero in that case).
  // The most likely case when scanIoRequest will fail is when the IOC has not
  // been fully initialized yet. In this case, calling scheduleProcessing will
  // usually work. If this does not work either, we print an error message.
  if (!::scanIoRequest(record.ioIntrModeScanPvt)) {
    if (!record.scheduleProcessing()) {
      errorExtendedPrintf(
        "%s Could not schedule asynchronous processing of record. Monitored item notification is not going to be processed.",
        record.getRecord()->name);
    }
  }
}

template<typename RecordType>
Open62541InputRecord<RecordType>::ReadCallbackImpl::ReadCallbackImpl(
    Open62541InputRecord &record) :
    record(record) {
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::ReadCallbackImpl::success(
    const UaNodeId &nodeId, const UaVariant &value) {
  record.readSuccessful = true;
  record.readValue = value;
  record.scheduleProcessing();
}

template<typename RecordType>
void Open62541InputRecord<RecordType>::ReadCallbackImpl::failure(
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
