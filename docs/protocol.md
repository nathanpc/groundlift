# GroundLift Protocol

This project uses the [OBEX 1.5](https://archive.org/details/obex-15) protocol
as the basis for the communication of data between endpoints. This protocol was
chosen due to its proven track record in peer-to-peer file sharing and the fact
that it's extremely stable and hasn't changed in years. It's also very easy to
implement and uses very little resources, which is important if we want
GroundLift to be ported to as many platforms as possible.

## Examples of Transactions

The easiest way to document and understand the protocol after reading the OBEX
specification is to look at some examples of how we implement it and a full
transaction between 2 peers is conducted, so here are a couple of examples:

### Sending a File


