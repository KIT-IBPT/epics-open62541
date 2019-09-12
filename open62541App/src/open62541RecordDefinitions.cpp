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

#include <stdexcept>

#include <dbCommon.h>
#include <devSup.h>
#include <epicsExport.h>

#include "open62541Error.h"
#include "Open62541AaiRecord.h"
#include "Open62541AaoRecord.h"
#include "Open62541AiRecord.h"
#include "Open62541AoRecord.h"
#include "Open62541BiRecord.h"
#include "Open62541BoRecord.h"
#include "Open62541LonginRecord.h"
#include "Open62541LongoutRecord.h"
#include "Open62541LsiRecord.h"
#include "Open62541LsoRecord.h"
#include "Open62541MbbiDirectRecord.h"
#include "Open62541MbbiRecord.h"
#include "Open62541MbboDirectRecord.h"
#include "Open62541MbboRecord.h"
#include "Open62541StringinRecord.h"
#include "Open62541StringoutRecord.h"

using namespace open62541::epics;

namespace {

template<typename RecordDeviceSupportType>
long getInterruptInfo(int command, ::dbCommon *record, ::IOSCANPVT *iopvt) {
  if (!record) {
    errorExtendedPrintf(
        "Cannot get interrupt info: Pointer to record structure is null.");
    return -1;
  }
  try {
    RecordDeviceSupportType *deviceSupport =
        static_cast<RecordDeviceSupportType *>(record->dpvt);
    if (!deviceSupport) {
      throw std::runtime_error(
          "Pointer to device support data structure is null.");
    }
    deviceSupport->getInterruptInfo(command, iopvt);
  } catch (std::exception &e) {
    errorExtendedPrintf("%s Getting interrupt info failed: %s", record->name,
        e.what());
    return -1;
  } catch (...) {
    errorExtendedPrintf("%s Getting interrupt info failed: Unknown error.",
        record->name);
    return -1;
  }
  return 0;
}

template<typename RecordDeviceSupportType, typename RecordType>
long initRecord(void *recordVoid) {
  if (!recordVoid) {
    errorExtendedPrintf(
        "Record initialization failed: Pointer to record structure is null.");
    return -1;
  }
  dbCommon *record = static_cast<dbCommon *>(recordVoid);
  RecordDeviceSupportType *deviceSupport;
  try {
    deviceSupport = new RecordDeviceSupportType(
        static_cast<RecordType *>(recordVoid));
    record->dpvt = deviceSupport;
  } catch (std::exception &e) {
    record->dpvt = nullptr;
    errorExtendedPrintf("%s Record initialization failed: %s", record->name,
        e.what());
    return -1;
  } catch (...) {
    record->dpvt = nullptr;
    errorExtendedPrintf("%s Record initialization failed: Unknown error.",
        record->name);
    return -1;
  }
  // We want to be able to use the output record even if the value
  // initialization fails, so we print an error message but return a status
  // code indicating success.
  try {
    deviceSupport->initializeRecord();
  } catch (std::exception &e) {
    errorExtendedPrintf("%s Could not read initial value: %s", record->name,
        e.what());
  } catch (...) {
    errorExtendedPrintf("%s Could not read initial value: Unknown error.",
        record->name);
  }
  return 0;
}

template<>
long initRecord<Open62541AoRecord, ::aoRecord>(void *recordVoid) {
  if (!recordVoid) {
    errorExtendedPrintf(
        "Record initialization failed: Pointer to record structure is null.");
    return -1;
  }
  dbCommon *record = static_cast<dbCommon *>(recordVoid);
  Open62541AoRecord *deviceSupport;
  try {
    deviceSupport = new Open62541AoRecord(
        static_cast<::aoRecord *>(recordVoid));
    record->dpvt = deviceSupport;
  } catch (std::exception &e) {
    record->dpvt = nullptr;
    errorExtendedPrintf("%s Record initialization failed: %s", record->name,
        e.what());
    return -1;
  } catch (...) {
    record->dpvt = nullptr;
    errorExtendedPrintf("%s Record initialization failed: Unknown error.",
        record->name);
    return -1;
  }
  // We want to be able to use the output record even if the value
  // initialization fails, so we print an error message but return a status
  // code indicating success.
  try {
    return deviceSupport->initializeAoRecord();
  } catch (std::exception &e) {
    errorExtendedPrintf("%s Could not read initial value: %s", record->name,
        e.what());
  } catch (...) {
    errorExtendedPrintf("%s Could not read initial value: Unknown error.",
        record->name);
  }
  return 0;
}

template<typename RecordDeviceSupportType>
long processRecord(void *recordVoid) {
  if (!recordVoid) {
    errorExtendedPrintf(
        "Record processing failed: Pointer to record structure is null.");
    return -1;
  }
  dbCommon *record = static_cast<dbCommon *>(recordVoid);
  try {
    RecordDeviceSupportType *deviceSupport =
        static_cast<RecordDeviceSupportType *>(record->dpvt);
    if (!deviceSupport) {
      throw std::runtime_error(
          "Pointer to device support data structure is null.");
    }
    deviceSupport->processRecord();
  } catch (std::exception &e) {
    errorExtendedPrintf("%s Record processing failed: %s", record->name,
        e.what());
    return -1;
  } catch (...) {
    errorExtendedPrintf("%s Record processing failed: Unknown error.",
        record->name);
    return -1;
  }
  return 0;
}

template<>
long processRecord<Open62541AiRecord>(void *recordVoid) {
  if (!recordVoid) {
    errorExtendedPrintf(
        "Record processing failed: Pointer to record structure is null.");
    return -1;
  }
  dbCommon *record = static_cast<dbCommon *>(recordVoid);
  long returnValue;
  try {
    Open62541AiRecord *deviceSupport =
        static_cast<Open62541AiRecord *>(record->dpvt);
    if (!deviceSupport) {
      throw std::runtime_error(
          "Pointer to device support data structure is null.");
    }
    returnValue = deviceSupport->processAiRecord();
  } catch (std::exception &e) {
    errorExtendedPrintf("%s Record processing failed: %s", record->name,
        e.what());
    return -1;
  } catch (...) {
    errorExtendedPrintf("%s Record processing failed: Unknown error.",
        record->name);
    return -1;
  }
  return returnValue;
}

}

extern "C" {

/**
 * Type alias for the get_ioint_info functions. These functions have a slightly
 * different signature than the other functions, even though the definition in
 * the structures in the record header files might indicate something else.
 */
typedef long (*DEVSUPFUN_GET_IOINT_INFO)(int, ::dbCommon *, ::IOSCANPVT *);

/**
 * aai record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devAaiOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541AaiRecord, ::aaiRecord>,
  getInterruptInfo<Open62541AaiRecord>,
  processRecord<Open62541AaiRecord>
};
epicsExportAddress(dset, devAaiOpen62541);

/**
 * aao record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devAaoOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541AaoRecord, ::aaoRecord>,
  nullptr,
  processRecord<Open62541AaoRecord>
};
epicsExportAddress(dset, devAaoOpen62541);

/**
 * ai record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
  DEVSUPFUN special_linconv;
} devAiOpen62541 = {
  6,
  nullptr,
  nullptr,
  initRecord<Open62541AiRecord, ::aiRecord>,
  getInterruptInfo<Open62541AiRecord>,
  processRecord<Open62541AiRecord>,
  nullptr
};
epicsExportAddress(dset, devAiOpen62541);

/**
 * ao record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
  DEVSUPFUN special_linconv;
} devAoOpen62541 = {
  6,
  nullptr,
  nullptr,
  initRecord<Open62541AoRecord, ::aoRecord>,
  nullptr,
  processRecord<Open62541AoRecord>,
  nullptr
};
epicsExportAddress(dset, devAoOpen62541);

/**
 * bi record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devBiOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541BiRecord, ::biRecord>,
  getInterruptInfo<Open62541BiRecord>,
  processRecord<Open62541BiRecord>
};
epicsExportAddress(dset, devBiOpen62541);

/**
 * bo record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devBoOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541BoRecord, ::boRecord>,
  nullptr,
  processRecord<Open62541BoRecord>
};
epicsExportAddress(dset, devBoOpen62541);

/**
 * longin record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devLonginOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541LonginRecord, ::longinRecord>,
  getInterruptInfo<Open62541LonginRecord>,
  processRecord<Open62541LonginRecord>
};
epicsExportAddress(dset, devLonginOpen62541);

/**
 * longout record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devLongoutOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541LongoutRecord, ::longoutRecord>,
  nullptr,
  processRecord<Open62541LongoutRecord>
};
epicsExportAddress(dset, devLongoutOpen62541);

/**
 * lsi record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devLsiOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541LsiRecord, ::lsiRecord>,
  getInterruptInfo<Open62541LsiRecord>,
  processRecord<Open62541LsiRecord>
};
epicsExportAddress(dset, devLsiOpen62541);

/**
 * lso record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devLsoOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541LsoRecord, ::lsoRecord>,
  nullptr,
  processRecord<Open62541LsoRecord>
};
epicsExportAddress(dset, devLsoOpen62541);

/**
 * mbbi record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devMbbiOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541MbbiRecord, ::mbbiRecord>,
  getInterruptInfo<Open62541MbbiRecord>,
  processRecord<Open62541MbbiRecord>
};
epicsExportAddress(dset, devMbbiOpen62541);

/**
 * mbbo record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devMbboOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541MbboRecord, ::mbboRecord>,
  nullptr,
  processRecord<Open62541MbboRecord>
};
epicsExportAddress(dset, devMbboOpen62541);

/**
 * mbbiDirect record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devMbbiDirectOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541MbbiDirectRecord, ::mbbiDirectRecord>,
  getInterruptInfo<Open62541MbbiDirectRecord>,
  processRecord<Open62541MbbiDirectRecord>
};
epicsExportAddress(dset, devMbbiDirectOpen62541);

/**
 * mbboDirect record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devMbboDirectOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541MbboDirectRecord, ::mbboDirectRecord>,
  nullptr,
  processRecord<Open62541MbboDirectRecord>
};
epicsExportAddress(dset, devMbboDirectOpen62541);

/**
 * stringin record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN read;
} devStringinOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541StringinRecord, ::stringinRecord>,
  getInterruptInfo<Open62541StringinRecord>,
  processRecord<Open62541StringinRecord>
};
epicsExportAddress(dset, devStringinOpen62541);

/**
 * stringout record type.
 */
struct {
  long numberOfFunctionPointers;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN_GET_IOINT_INFO get_ioint_info;
  DEVSUPFUN write;
} devStringoutOpen62541 = {
  5,
  nullptr,
  nullptr,
  initRecord<Open62541StringoutRecord, ::stringoutRecord>,
  nullptr,
  processRecord<Open62541StringoutRecord>
};
epicsExportAddress(dset, devStringoutOpen62541);

} // extern "C"
