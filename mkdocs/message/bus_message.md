# Bus Message
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