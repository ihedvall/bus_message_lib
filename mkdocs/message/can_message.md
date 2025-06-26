# CAN Message
All message classes have a ToRaw() and a FromRaw() function that serialize and deserialize the message.
The messages are pushed or popped from a message queue. The message queues are list of smart pointers. 
To create a message, use the following code.

```C++ 
#include <bus/candataframe.h>

auto msg = std::make_shared<CanDataFrame>
// Set the properties and message data (payload).

publisher->Push(msg);
```
Each CAN message, have a Time Stamp and a Bus Channel property. 
The Timestamp is always referencing nanoseconds since 1970-01-01 UTC (64-bit unsigned integer). 
The bus channel is 1–255 and defines the source of the message.

!!! warning

    Each CAN message has a unique ID which in someway is a little bit unique.
    The CAN Message ID is the CAN ID with the 31:th bit indicating that the ID is extended.
    It is a common mistake to blend the CAN and Message ID within the code. 


## CAN Data Frame
The standard Data Frame message is the normal message on a functional CAN or CAN-FD bus.
The CAN message can hold up to eight bytes while the CAN-FD message can hold up to 64 bytes of payload data.
<table>
<caption>CAN Data Frame Message Layout</caption>
<tr><th>Byte (:Bits)</th><th>Description</th><th>Size</th></tr>
<tr><td>0-17</td><td>Message Header</td><td>18 bytes</td></tr>
<tr><td>18-21</td><td>Message ID (CAN Id + IDE)</td><td>uint32_t</td></tr>
<tr><td>22</td><td>DLC</td><td>uint8_t</td></tr>
<tr><td>23</td><td>Data Length</td><td>uint8_t</td></tr>
<tr><td>24-27</td><td>CRC</td><td>uint32_t</td></tr>
<tr><td>28:0</td><td>Direction (Rx=0, Tx=1)</td><td>1-bit</td></tr>
<tr><td>28:1</td><td>SRR</td><td>1-bit</td></tr>
<tr><td>28:2</td><td>EDL</td><td>1-bit</td></tr>
<tr><td>28:3</td><td>BRS</td><td>1-bit</td></tr>
<tr><td>28:4</td><td>ESI</td><td>1-bit</td></tr>
<tr><td>28:5</td><td>RTR</td><td>1-bit</td></tr>
<tr><td>28:6</td><td>R0</td><td>1-bit</td></tr>
<tr><td>28:7</td><td>R1</td><td>1-bit</td></tr>
<tr><td>29:0</td><td>Wake Up</td><td>1-bit</td></tr>
<tr><td>29:1</td><td>Single Wire</td><td>1-bit</td></tr>
<tr><td>30-33</td><td>Frame Duration (ns)/td><td>uint32_t</td></tr>
<tr><td>34-xx</td><td>Data Bytes</td><td>Data Length bytes</td></tr>
</table>

### Message ID
The Message ID is a 32-bit value that includes an 11/27-bits CAN ID and 
an Extended Address flag at the highest bit.
The Extended flag defines if it is a 27-bits or an 11-bits address.
Yes, this is confusing, but this is industry standard.

### Data Length Code (DLC)
The data length code (DLC) is somewhat an artifact. 
The number of bytes and DLC are identical for CAN buses, but
CAN-FD uses different code for more than 8-bytes payload bytes.

!!! hint
    The class sets the DLC and Data Length properties automatically when the
    payload data is set.

### Checksum (CRC)
The checksum is rarely used and is optional to use.

### Direction
The Direction flag defines if the source (channel) was sending 
this message or receiving it. 
This is actually an enumerated value where 0 is Rx and 1 is Tx.

### Substitute Remote Request (SRR)
You need to ask a CAN expert about this flag.

### Extended Data Length (EDL)
The CAN-FD buses sets this flag to define that this is a CAN-FD frame.

### Bit Rate Switch (BRS)
Used by the CAN-FD bus.

### Error State Indicator (ESI)
Used by the CAN-FD bus.

### Remote Frame (RTR)
Remote Frame flag defines that this is a remote frame.
There is actually a special Remote Frame message defined. 
Remote Frame doesn't include any payload data.

### Reserved Bit 0 and 1
Undefined was these bits are used for.

### Wake Up
Flag that indicates that this was a wake-up message.

### Single Wire Detected
Some hardware can detect if only one pin was used to detect the message.

### Frame Duration
Frame Duration of the message in nanoseconds. 

## CAN Remote Frame
The Remote Frame is almost identical with the Data Frame but with the RTR flag set.
There is no payload data sent with this message.

## CAN Error Frame
The Error Frame is a normal Data Frame with error information.

## CAN Overload Frame
Seldom used message and is used mainly as an indication.
