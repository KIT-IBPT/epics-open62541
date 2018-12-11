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
DBD += open62541.dbd

# specify all source files to be compiled and added to the library
open62541_SRCS += open62541.c
open62541_SRCS += open62541Error.cpp
open62541_SRCS += open62541RecordDefinitions.cpp
open62541_SRCS += open62541Registrar.cpp
open62541_SRCS += Open62541RecordAddress.cpp
open62541_SRCS += ServerConnection.cpp
open62541_SRCS += ServerConnectionRegistry.cpp

open62541_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

