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

#ifndef OPEN62541_EPICS_LONGIN_RECORD_H
#define OPEN62541_EPICS_LONGIN_RECORD_H

#include <longinRecord.h>

#include "Open62541InputRecord.h"

namespace open62541 {
namespace epics {

/**
 * Device support class for the longin record.
 */
class Open62541LonginRecord: public Open62541InputRecord<::longinRecord> {

public:

  /**
   * Creates an instance of the device support for the longin record.
   */
  Open62541LonginRecord(::longinRecord *record) :
    Open62541InputRecord<::longinRecord>(record) {
    // We call this method here instead of the base constructor because it can
    // be (and is) overridden in child classes.
    validateRecordAddress();
  }

protected:

  virtual void writeRecordValue(const UaVariant &value) {
    writeRecordValueGeneric(value, getRecord()->val);
  }

private:

  // We do not want to allow copy or move construction or assignment.
  Open62541LonginRecord(const Open62541LonginRecord &) = delete;
  Open62541LonginRecord(Open62541LonginRecord &&) = delete;
  Open62541LonginRecord &operator=(const Open62541LonginRecord &) = delete;
  Open62541LonginRecord &operator=(Open62541LonginRecord &&) = delete;

};

}
}

#endif // OPEN62541_EPICS_LONGIN_RECORD_H
