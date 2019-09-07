EPICS Device Support for Open62541 (OPC UA)
===========================================

This EPICS device support allows for integrating process variables provided by
OPC UA servers into an EPICS IOC. The implementation is backed by the Open62541
library (https://open62541.org/) which provided the low-level implementation of
the OPC UA protocol.

The Open62541 library is distributed with this device support in the form of the
single-file distribution. Consequently, the files `open62541.c` and
`open62541.h` are distributed under the terms of the Mozilla Public License
version 2.0, which can be found in the file [LICENSE-MPL].

Features
--------

* Connect to multiple OPC UA servers.
* Automatic reconnection handling.
* Read and write OPC UA process variables.

**Missing features:**

* Subscriptions (only polling is supported so far).
* Support for string type (only numeric data-types are supported).
* Bi-directional process variables (output records are initialized once on
  startup, but do not receive updates when the variable changes on the server).
* Support for GUID-based node IDs.

Installation
------------

In order to build this device support, one might have to create the file
`configure/RELEASE.local` and set `EPICS_BASE` to the correct path. Depending
on the used compiler, one might also have to change the compiler flags so that
the compiler expects C 11 and C++ 11 source code. This can be done by creating
a file called `configure/CONFIG_SITE.local` and setting `USR_CFLAGS` and
`USR_CXXFLAGS` in this file. An example can be found in
`configure/EXAMPLE_CONFIG_SITE.local`.

Usage
-----

In order to use this device support in an EPICS IOC, one has to add its install
path to the `configure/RELEASE` file of the IOC (e.g.
`OPEN62541 = /path/to/epics/modules/open62541`) and add a dependency on the
library and DBD file to the IOCs `Makefile` (e.g. `xxx_DBD += open62541.dbd` and
`xxx_LIBS += opent62541`).

In the IOC's st.cmd, one can connect to an OPC UA server using the following
command:

```
open62541ConnectionSetup("C0", "opc.tcp://opc-ua.example.com:4840", "username", "password");
```

If the OPC UA server does not require authentication, an empty string should be
passed for the username and password. In this example, `C0` is the identifier
for the connection. This identifier is used when referring to a connection from
the EPICS 

The device support works with the aai, aao, ai, ao, bi, bo, longin, longout,
mbbi, mbbo, mbbiDirect, and mbboDirect records. It is used by setting `DTYP` to
`open62541`.

At the moment, the device support only supports polling, so input records have
to be scanned in order to be updated.

The address in the `INP` or `OUT` field has the form

```
@<connection identifier> [(<options>)] <node ID> [<data type>] 
```

where `<connection identifier>` is the ID that was specified when creating the
connection (the ID must not contain whitespace or parentheses), `[(options)]` is
an optional options string,  `<node ID>` is the identifier of the node on the
server, and `[<data type>]` is the optional data type.

The options string is optional. If specified, it has to be enclosed in
parentheses. If more than one option is specified, the options are separated by
commas. At the moment, the following options are supported:

* `conversion_mode=<mode>`: Only supported for the ai and ao record. If
  specified, `<mode>` must be `convert` or `direct`. In `convert` mode, the
  device support writes to the record's `RVAL` field so that conversions apply.
  In `direct` mode, the device support writes to the records's `VAL` field so
  that conversions are bypassed. If the conversion mode is not specified, it
  depends on the OPC UA data-type. The Boolean, Byte, SByte, UInt16, Int16, and
  Int32 types default to `convert`, while the `UInt32`, `UInt64`, `Int64`,
  `Float` and `Double` types default to `direct`. 
* `no_read_on_init`: Only supported for output records. If specified, the
  record's value is *not* initialized by reading the current value from the
  server.

The node ID identifies the process variable on the server. There are two ways,
how a node ID can be specified: string and numeric. A string identifier has the
form `str:<namespace index>,<string ID>`, where `<namespace index>` is a number
identifying the namespace of the node ID and `<string ID>` is the node's name.
A numeric identifieer has the form `num:<namespace index>,<numeric ID>`, where
`<namespace index>` is a number identifying the namespace of the node ID and
`<numeric ID>` is the node's numeric ID. In most applications, string-based IDs
are used.

The data type specification is optional. If the data type is not specified, it
is guessed. For input records that works pretty well because the server sends
the value with the correct data-type and the client can then use this data type.
For output records, the data type is guessed based on the record type, which
will not always match the data type on the server. The mapping from record type
to default data type is as follows:

* ao: Double
* bo: Boolean
* longout: Int32
* mbbo and mbboDirect: UInt32

For the aao record, it depends on the type specified in the `FTVL` field:

* CHAR: Byte
* UCHAR: SByte
* SHORT: Int16
* USHORT: UInt16
* LONG: Int32
* ULONG: UInt32
* FLOAT: Float
* DOUBLE: Double

If the data type is specified explicitly, one of the following data-types can
be chosen for each record:

* Boolean
* Byte
* SByte
* UInt16
* Int16
* Uint32
* Int32
* UInt64
* Int64
* Float
* Double

**Examples for valid addresses:**

* `@C0 str:2,process.variable`
* `@C0 (no_read_on_init,convert=direct) str:2,other.process.variable Float`
* `@C0 num:2,353 Int16`

**Examples for records:**

```
record(ai, "$(P)$(R)ai") {
  field(DTYP, "open62541")
  field(INP,  "@$(CONN) str:2,process.variable")
  field(SCAN, "1 second")
}
```

```
record(ao, "$(P)$(R)ao") {
  field(DTYP, "open62541")
  field(OUT,  "@$(CONN) (conversion_mode=direct) str:2,other.process.variable uint32")
}
```

Copyright / License
-------------------

This EPICS device support is licensed under the terms of the
[GNU Lesser General Public License version 3](LICENSE-LGPL.md). It has been
developed by [aquenos GmbH](https://www.aquenos.com/) on behalf of the
[Karlsruhe Institute of Technology's Institute of Beam Physics and Technology](https://www.ibpt.kit.edu/).

The [Open62541 library](https://open62541.org/), which is shipped with this
device support (files `open62541.c` and `open62541.h`) is licensed under the
terms of the [Mozilla Public License version 2.0](LICENSE-MPL.txt). The
copyright for that library is with the original contributors.

This device support contains code originally developed for the s7nodave device
support. aquenos GmbH has given special permission to relicense these parts of
the s7nodave device support under the terms of the GNU LGPL version 3.