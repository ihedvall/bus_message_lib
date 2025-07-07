# TCP/IP Broker
A TCP/IP Broker is a combination of a Shared Memory Broker and a TCP/IP Server.
The broker implements a shared memory broker and connects a TCP/IP server to that memory.
This enables remote connections to the bus.
Use a TCP/IP Client to connect to the server.

``` C++
#include <bus/interface/businterfacefactory.h>
// The broker is a smart pointer (unique_ptr)
auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::TcpBrokerType);
broker->Name("BusMemory"); // Share memory name
broker->Address("0.0.0.0");
broker->Port(42611);
broker->Start();    
```
