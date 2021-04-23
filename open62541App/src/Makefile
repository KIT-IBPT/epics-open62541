TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += open62541

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install open62541.dbd into <top>/dbd
DBDCAT += open62541.dbd

open62541_DBD += common.dbd
ifdef BASE_3_16
open62541_DBD += int64.dbd
open62541_DBD += longString.dbd
endif
# The lsi and lso records are also available in 3.15 starting with 3.15.0.2.
# We cannot easily check for a certain patch level, so enable these records for
# all versions starting with 3.15.1.
ifeq (BASE_3_15, YES)
ifneq (EPICS_MODIFICATION, 0)
open62541_DBD += longString.dbd
endif
endif

# specify all source files to be compiled and added to the library
open62541_SRCS += open62541.c
open62541_SRCS += open62541Error.cpp
open62541_SRCS += open62541DumpServerCertificates.cpp
open62541_SRCS += open62541RecordDefinitions.cpp
open62541_SRCS += open62541Registrar.cpp
open62541_SRCS += Open62541RecordAddress.cpp
open62541_SRCS += ServerConnection.cpp
open62541_SRCS += ServerConnectionRegistry.cpp
open62541_SRCS += UaNodeId.cpp
open62541_SRCS += UaVariant.cpp

open62541_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

