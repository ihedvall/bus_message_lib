# Message Header
All messages have the same 18-byte header. 
The byte order is always little endian (Intel Byte Order). 

<table>
    <caption>Bus Message Header</caption>
    <tr><th>Byte</th><th>Description</th><th>Size</th></tr>
    <tr><td>0-1</td><td>Type of Message (enumerate)</td><td>uint16_t</td></tr>
    <tr><td>2-3</td><td>Version Number</td><td>uint16_t</td></tr>
    <tr><td>4-7</td><td>Length of the message</td><td>uint32_t</td></tr>
    <tr><td>8-15</td><td>Timestamp ns since 1970</td><td>uint64_t</td></tr>
    <tr><td>16-17</td><td>Bus Channel</td><td>uint16_t</td></tr>
</table>

## Type of Message
Unique identifier of the message. 
The message types are defined in the '_ibusmessage.h_' header file.

## Version Number
Simple version number that is incremented each time the message layout is changed.

## Length of Message
The total length of the message includes this 18-byte header.
A message must thus be larger or equal to 18 bytes.

## Timestamp
The timestamp is always using the UTC time zone.
The timestamp value is the number of nanoseconds since 1970-01-01 midnight.

## Bus Channel
This identifies the CAN device channel. 
Note that many manufacture display the first channel (0) as 'Channel 1'. 
The prefered way is to start the channel number at 1.
