EPICS Device Support for Open62541 (OPC UA)
===========================================

This EPICS device support allows for integrating process variables provided by
OPC UA servers into an EPICS IOC. The implementation is backed by the Open62541
library (https://open62541.org/) which provided the low-level implementation of
the OPC UA protocol.

The Open62541 library is distributed with this device support in the form of the
single-file distribution. Consequently, the files `open62541.c` and
`open62541.h` are distributed under the terms of the [Mozilla Public License
version 2.0](LICENSE-MPL.txt).

Features
--------

* Connect to multiple OPC UA servers.
* Automatic reconnection handling.
* Read and write OPC UA process variables.
* Monitor OPC UA process variables for changes (subscriptions / monitored
  items).
* Support for signed and encrypted connections (when compiled with encryption
  support).

**Missing features:**

* Support for arrays of strings (only scalar strings are supported).
* Bi-directional process variables (output records are initialized once on
  startup, but do not receive updates when the variable changes on the server).
* Monitoring nodes at a sampling rate that is higher than the publishing
  interval (using queueing).
* Using server-provided time-stamps when monitoring nodes.

Installation
------------

In order to build this device support, one might have to create the file
`configure/RELEASE.local` and set `EPICS_BASE` to the correct path. Depending
on the used compiler, one might also have to change the compiler flags so that
the compiler expects C 11 and C++ 11 source code. This can be done by creating
a file called `configure/CONFIG_SITE.local` and setting `USR_CFLAGS` and
`USR_CXXFLAGS` in this file. An example can be found in
`configure/EXAMPLE_CONFIG_SITE.local`.

If you want to enable encryption support, you need to installe
[mbed TLS](https://tls.mbed.org/) and set `USE_MBEDTLS` to `YES` in
`configure/CONFIG_SITE.local`. If mbed TLS is not installed in one of the
standard locations where the compiler and linker will find it, you might also
have to specify `MBEDTLS_LIB` and `MBEDTLS_INCLUDE`.

Usage
-----

In order to use this device support in an EPICS IOC, one has to add its install
path to the `configure/RELEASE` file of the IOC (e.g.
`OPEN62541 = /path/to/epics/modules/open62541`) and add a dependency on the
library and DBD file to the IOCs `Makefile` (e.g. `xxx_DBD += open62541.dbd` and
`xxx_LIBS += open62541`).

### Connecting to a server

In the IOC's st.cmd, one can connect to an OPC UA server using the following
command:

```
open62541ConnectionSetup("C0", "opc.tcp://opc-ua.example.com:4840", "username", "password");
```

If the OPC UA server does not require authentication, an empty string should be
passed for the username and password. In this example, `C0` is the identifier
for the connection. This identifier is used when referring to a connection from
the EPICS 

The `open62541ConnectionSetup` command will establish a connection that is
neither signed nor encrypted. If you need such a connection, please refer to
the section called “Using encryption” later in this document.

### Configuring records

The device support works with the aai, aao, ai, ao, bi, bo, int64in, int64out,
longin, longout, lsi, lso, mbbi, mbbo, mbbiDirect, mbboDirect, stringin, and
stringout records. It is used by setting `DTYP` to `open62541`.

Input records can be polled or they can be monitored through a subscription.
When `SCAN` is set to `I/O Intr`, a monitored item for the specified node is
registered with a subscription, so that the server periodically send updates
(according to the configured sampling and publishing interval). Otherwise, the
server is polled for a new value each time the record is processed.

In general, using monitored items and subscriptions is more efficient than
polling (at least if many records share the same subscription), so it should be
the preferred way of monitoring nodes for changes.

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
* `sampling_interval`: Only supported for input records that are operated in
  `I/O Intr` mode. In this case, this option specifies the sampling interval
  for the respective OPC UA node in milliseconds (how often the OPC UA server
  will check the node's value for changes). Updates are still only received
  according to the publishing interval of the associated subscription and the
  server is free to choose a different sampling interval if it does not support
  the requested one. If not specified (or if `NaN` is specified), the sampling
  interval is set to be the same as the publishing interval of the associated
  subscription, which ususally makes sense.
* `subscription`: Only supported for input records that are operated in
  `I/O Intr` mode. In this case, this option specifies the name of the
  subscription that is used when registering the monitored item with the OPC UA
  server. The configuration options for subscriptions can be set through IOC
  shell commands. If the name of the subscription is not specified explicitly,
  the subscription with the name `default` is used.

The node ID identifies the process variable on the server. There are three ways
how a node ID can be specified: string, numeric, and GUID. In most applications,
string-based IDs are used.

A string identifier has the form `str:<namespace index>,<string ID>`, where
`<namespace index>` is a number identifying the namespace of the node ID and
`<string ID>` is the node's name.

A numeric identifier has the form `num:<namespace index>,<numeric ID>`, where
`<namespace index>` is a number identifying the namespace of the node ID and
`<numeric ID>` is the node's numeric ID.

A GUID identifier has the form `guid:<namespace index>,<GUID>`, where
`<namespace index>` is a number identifying the namespace of the node ID and
`<GUID>` is a valid GUID (e.g. `7877004d-bb37-41d2-9017-2ef483c49e8f`).

The data type specification is optional. If the data type is not specified, it
is guessed. For input records that works pretty well because the server sends
the value with the correct data-type and the client can then use this data type.
For output records, the data type is guessed based on the record type, which
will not always match the data type on the server. The mapping from record type
to default data type is as follows:

* ao: Double
* bo: Boolean
* int64out: Int64
* longout: Int32
* mbbo and mbboDirect: UInt32
* lso, stringout: String

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
* String
* ByteString

**Examples for valid addresses:**

* `@C0 str:2,process.variable`
* `@C0 (no_read_on_init,convert=direct) str:2,other.process.variable Float`
* `@C0 num:2,353 Int16`
* `@C0 (sampling_interval=500.0,subscription=mysub) str:4,some.process.variable`
* `@C0 guid:3,7877004d-bb37-41d2-9017-2ef483c49e8f`

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

record(ai, "$(P)$(R)aiMonitoring") {
  field(DTYP, "open62541")
  field(INP,  "@$(CONN) str:2,process.variable")
  field(SCAN, "I/O Intr")
}

### Configuring subscriptions

The options for a specific subscription can be set through three IOC shell commands:

```
open62541SetSubscriptionLifetimeCount("C0", "mysub", 10000);
open62541SetSubscriptionMaxKeepAliveCount("C0", "mysub", 10);
open62541SetSubscriptionPublishingInterval("C0", "mysub", 500.0);
```

The first two arguments to each of these commands are the identifier of the
connection and the identifier of the subscription that shall be configured.
Unlike connections, subscriptions do not have to be created explicitly. They are
created automatically when a record refers to them. A record that does not
explicitly specify a certain subscription automatically uses the subscription
with the name `default`.

The lifetime count is a non-negative integer number that specifies after how
many publishing intervals without client activity, the server may consider the
client inactive and cancel the subscription. The default value is 10000 and
there usually is no need to change this setting.

The max. keep alive count is a non-negative integer number that specifies how
often (in publishing intervals) the server sends an empty notification to the
client, even if there are no value changes to be published. This allows for the
client to detect that the connection to the server is still active. The default
value is 10. When specifying short publishing intervals, one might want to
increase this number so that the server does not have to send empty
notifications too often. When specifying a long publishing interval (e.g.
several seconds), one might want to reduce this number, so that there is some
server activity within a reasonable amount of time.

The publishing interval is a floating point number that specifies (in
millseconds) how often the server sends outstanding notifications. Notifications
are collected at the sampling interval specified for each monitored item, but
they are queued and only sent out together at each publishing interval. Please
note that the open62541 EPICS device support currently uses a queue size of one
for each monitored item. This means no “bursts” of updates are going to be
received, even if the sampling interval is less than the publishing interval.
For this reason, it typically does not make sense to specify a shorter sampling
interval than the publishing interval.

### Using encryption

If the open62541 device support has been compiled with encryption support
enabled (by adding `USE_MBEDTLS = YES` to `configure/CONFIG_SITE.local`), there
are two additional IOC shell commands.

The `open62541ConnectionSetupEncrypted` is used instead of the
`open62541ConnectionSetup` command and can be used to establish a signed or
encrypted connection to a server. It can be used like in the following example:

```
open62541ConnectionSetupEncrypted("C0", "opc.tcp://opc-ua.example.com:4840", "username", "password", "sign & encrypt", "$(TOP)/pki/client_cert.der", "$(TOP)/pki/client_key.der", "$(TOP)/pki/server_cert.der", "urn:unconfigured:application");
```

The meaning of the first four arguments is exactly the same as for the
`open62541ConnectionSetup` command. The following arguments are in order:

* The message security mode used for the connection. The default value (used if
  the string is empty) is `none`. This means that communication with the server
  will neither be signed nor encrypted. Typically, the only reason to use this
  mode (instead of simply using `open62541ConnectionSetup`) is a server
  requiring some cryptography for authentication, but still allowing unsigned
  and unencrypted communication. The other modes are `sign` (communication is
  signed so that it cannot be manipulated on the wire) and `sign & encrypt`
  (communication is also encrypted so that eavesdropping is not possible).
* The path to the file storing the client certificate. Specifying a client
  certificate is mandatory. The file must be in the DER format. An example of
  how to generate a client certificate is given later in this section.
* The path to the file storing the private key associated with the client
  certificate. The file must be in the DER format and the key must be available
  unencrypted (without requiring a passphrase). The private key is mandatory as
  well.
* The path to the file storing the server certificate. This is optional. If an
  empty string is specified, validation of the server certificate is disabled,
  so the client simply trusts any certificate presented by the server. If
  specified, the certificate file must be in the DER format as well. The server
  certificate can be retrieved using the `open62541DumpServerCertificates`
  command (described later in this section).
* The application URI. The default value (if an empty string is specified) is
  `urn:unconfigured:application`. Typically, servers check that the application
  URI presented by the client matches the one specified by the client
  certificate, so it might be necessary to specify this parameter if using a
  certificate that specifies a different URI than
  `urn:unconfigured:application`.

**Generating a client certificate:**

A self-signed client certificate can be generated using
[OpenSSL](https://www.openssl.org/). First, one has to create a certificate
request:

```
openssl req \
  -out client_cert.csr \
  -keyout client_key.pem \
  -newkey rsa:2048 \
  -nodes
```

Next, the certificate request has to be signed. There are some options that
cannot be specified on the command line directly, so we first have to create a
file called `exts.txt` with the following content:

```
subjectAltName=URI:urn:unconfigured:application
basicConstraints=critical,CA:FALSE
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
keyUsage=critical,digitalSignature,nonRepudiation,keyEncipherment,dataEncipherment,keyCertSign
extendedKeyUsage=critical,serverAuth,clientAuth
```

Now, the certificate can be signed:

```
openssl x509 \
  -in client_cert.csr \
  -out client_cert.der \
  -outform der \
  -signkey client_key.pem \
  -extfile exts.txt \
  -sha256 \
  -days 10950 \
  -req
```

Finally, the private key has to be converted from the PEM to the DER format:

```
openssl rsa \
  -in client_key.pem \
  -out client_key.der \
  -outform der
```

Typically, it is necessary to tell the OPC UA server to trust the certificate
presented by the client. Please refer to the documentation of the respective OPC
UA server for more information on how to do this.

If the server certificate is supposed to be validated by the client, it first
has to be saved to a file (in DER format). If you already have such a file, you
can simply use it. Otherwise, you can use the `open62541DumpServerCertificates`
IOC shell command in order to retrieve it.

**Retrieving the server certificate:**

The `open62541DumpServerCertificates` IOC shell command can be used to retrieve
all certificates presented by a server (typically, there will only be one):

```
open62541DumpServerCertificates("opc.tcp://opc-ua.example.com:4840", "$(TOP)/pki")
```

This command will dump the certificates presented by the server available at the
specified endpoint to the directory `$(TOP)/pki`. The directory needs to already
exist. Each certificate is dumped to a file with a filename in the form
`<SHA-256 hash>.der`. You can use OpenSSL to inspect the dumped certificate(s).
For example:

```
openssl x509 \
  -in ead5cd3be2697b77a2378dbc058474a3f0e45ff6f4f6845acdd88f692afad8d4.der \
  -inform der \
  -noout \
  -text
```

Updating the open62541 library
------------------------------

The open62541 library is bundled with this device support in order to make the
build process as easy as possible. Usually, there is no need to update the
bundled library, so this information is mainly intended for the maintainers of
the device support.

In order to build the single-file distribution that is bundled with this device
support, one has to run the following commands in the open62541 source tree:

```
cmake -DUA_ENABLE_AMALGAMATION=ON
make open62541-amalgamation-header open62541-amalgamation-source
```

This will create the two files `open62541.c` and `open62541.h` in the root of
the source distribution, which can then be copied to the open62541App/src of the
device support.

Copyright / License
-------------------

This EPICS device support is licensed under the terms of the
[GNU Lesser General Public License version 3](LICENSE-LGPL.md). It has been
developed by [aquenos GmbH](https://www.aquenos.com/) on behalf of the
[Karlsruhe Institute of Technology's Institute of Beam Physics and Technology](https://www.ibpt.kit.edu/).

The [open62541 library](https://open62541.org/), which is shipped with this
device support (files `open62541.c` and `open62541.h`) is licensed under the
terms of the [Mozilla Public License version 2.0](LICENSE-MPL.txt). The
copyright for that library is with the original contributors.

This device support contains code originally developed for the s7nodave device
support. aquenos GmbH has given special permission to relicense these parts of
the s7nodave device support under the terms of the GNU LGPL version 3.
