# GroundLift Protocol

This project uses the `glproto` protocol as the basis for the communication of
data between endpoints. This protocol was created because we needed a very easy
to implement, performant, and extremely portable protocol that uses very little
resources and can be ported to as many platforms as possible.

## Transport Layer

All of the messages exchanged between the client and the server will obligatory
use **UDP**. Since almost everything that the two devices must negotiate is very
simple information (type of transfer, discovery, size, name, etc.) we don't
need the overhead of a full TCP connection and the information sent will always
fit in a single UDP packet.

The only time when **TCP** will be used is during file transfers. More details
about this can be found in the [Sending Files](#sending-files) section.

The default network port used for general communication **MUST BE** port `1650`.
Only TCP file transfers **MAY** use other ports in order to handle multiple
transfers.

## Messages

Messages are going to be sent using **UDP** and will vary depending on the type
of data that they carry. They are not versioned and **MUST always contain the
same headers in the exact same order**. In case of future extensions, these will
be appended to the message and won't be parsed by older versions of the program,
ensuring backwards compatibility.

### Unique Peer Identifier

In order to easily identify a peer on the network without having to rely on
their IP address, each peer will store in its configuration a 64-bit unique
identifier, hereby called `GLUPI` (GroundLift Unique Peer Identifier), which
**MUST** be sent as the first header field on all messages.

With this unique identifier, systems that are capable of maintaining a cache of
the peers they encounter on a network can use this UID to match additional data
to a peer regardless of its IP address.

### Structure

Messages will always start with the characters `GL` (indicating GroundLift) and
**MUST** be followed by a  single `char` indicating the type of the message and
a `NUL` character. Following this preamble a `uint16` indicating the overall
size of the message (including the preamble) in bytes **MUST** be present.

Afterwards will come a list of headers fields. Each header **MUST** be prefixed
by a single `'|'` character. Every header type will have a fixed length defined
in the specification. Strings are the only exception and **CAN** have a variable
length, for more information about how they are implemented check the
[Strings](#strings) section.

Since headers are ordered by the specification and their order matters we don't
need any other additional information to tell them apart.

#### Multi-byte Values

All multi-byte values **MUST** always be in
[network byte order](https://www.rfc-editor.org/ien/ien137.txt) (big-endian)
when transferred.

#### Strings

All strings **MUST** start with a `uint8` denoting their length in bytes
(including the `NUL` terminator) and are in fact a merge between Pascal and C
style strings.

Due to their length being defined as a `uint8` it means that there will be a
hard limit of 253 characters (including the NUL terminator). This is an
arbitrary constraint that may help very limited systems (embedded, DOS, etc.)
deal with the filenames.

One important aspect is that all strings are by definition UTF-8 encoded, but
for platforms that don't support Unicode properly these **CAN** be silently
ignored and displayed as broken characters. You're not forced to ensure they are
properly dealt with on platforms that don't support UTF-8.

## Transaction Examples

This is a collection of examples that describe how transactions may occur in
detail. In this section we'll use the following naming conventions for the
parties involved in a transaction:

- `send-client` is the host that initiates a send operation and will never
  listen for any reply packets.
- `recv-server` is the host that is always listening for **UDP** (and only UDP)
  packets from an `send-client`.
- `send-server` is the host that initiated the send operation and will be
  listening for a **TCP** connection coming from the `recv-server`.

### Sending Files

When sending a file `send-client` will send a request message to the
`recv-server` with all the details of the file and a port number to accept an
incoming connection. If `recv-server` wants to accept the transaction it'll open
a **TCP** connection with `send-server` on the specified port.

The **TCP** data stream connection opening will be the only indication for
`send-server` that the transfer was accepted. If `recv-server` doesn't open the
connection within **15 seconds** (plenty of time for `recv-server` to accept the
transfer) `send-server` **MUST** assume that it was refused.

After `send-server` checks if the IP of `recv-server` and the connected port
matches an initiated transaction it'll stream the entire contents of the file
straight through the **TCP** socket connection (no length, header or any kind of
overhead must be added) and **MUST** close the connection once `EOF` is reached.
Since the length was already sent as part of the request message it's already
known by `recv-server`.

The `recv-server` **MUST** only accept the file up until the prearranged length
and terminate the connection if `send-server` sends any additional bytes past
the limit. If the connection is closed prematurely for any reason, the file must
be deemed incomplete/corrupted. The same must be true if additional bytes are
sent by `send-server` past the prearranged length.

The `recv-server` **MUST** wait for `send-server` to close the connection to
mark the file transfer as completed and should not close the connection when the
prearranged length is reached. If any of the parties closes the connection at
any point during the transfer before it being completed it **MUST** be
interpreted as a cancellation.

### Peer Discovery

Since this protocol is designed to be extremely portable and work on very
limited hardware, and even sometimes very limited software stacks, we've decided
to not use multicast for discovery messages and instead opted for the
universally available broadcast approach (at least for IPv4). This implies that
peer discovery request messages must be sent to the **network broadcast
address**.

#### Request

A `send-client` that wishes to present its user with a list of peer
`recv-server` may send a peer discovery request message to the **network
broadcast address** with the following structure:

| Data | Hex | Length | Description |
| :---: | :---: | :---: | :--- |
| `'GL'` | `47 4C` | 2 | GroundLift packet identifier |
| `'D'` | `44` | 1 | Discovery message type identifier |
| `NUL` | `00` | 1 | `NUL` separator |
| `32` | `00 20` | 2 | Length of the entire message |
| `'\|'` | `7C` | 1 | `GLUPI` header field start marker |
| ... | ... | 8 | GroundLift Unique Peer Identifier |
| `'\|'` | `7C` | 1 | `Device/OS Identifier` header field start marker |
| `'Win'` | `57 69 6E` | 3 | Device or OS identification characters |
| `NUL` | `00` | 1 | `NUL` separator |
| `'\|'` | `7C` | 1 | `Hostname` header field start marker |
| `9` | `00 09` | 2 | Length of the string with the `NUL` terminator |
| `"hostname"` | ... | 9 | Hostname string with a `NUL` terminator |

#### Response

Any `recv-server` that receives a peer discovery request message **MUST** reply
to the `send-client` that sent the request with a message that follows the exact
same structure as the request message.

### Sending URLs

A `send-client` will send a `U` type message with the `URL` encoded as a
`string` in the first header field. **No confirmation or `ACK` will be
expected** from the `recv-server` that receives this message.

#### Message Example

| Data | Hex | Length | Description |
| :---: | :---: | :---: | :--- |
| `'GL'` | `47 4C` | 2 | GroundLift packet identifier |
| `'U'` | `55` | 1 | URL message type identifier |
| `NUL` | `00` | 1 | `NUL` separator |
| `35` | `00 23` | 2 | Length of the entire message |
| `'\|'` | `7C` | 1 | `GLUPI` header field start marker |
| ... | ... | 8 | GroundLift Unique Peer Identifier |
| `'\|'` | `7C` | 1 | `URL` header field start marker |
| `17` | `00 11` | 2 | Length of the string with the `NUL` terminator |
| `"http://test.com/"` | ... | 17 | URL as a string with a `NUL` terminator |
