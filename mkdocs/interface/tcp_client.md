# TCP/IP Client
Implement a simple TCP/IP client that connects to a TCP/IP broker or a TCP/IP server.

A typical application is bidirectional communication with a physical driver.
This application shall call the Start() function.

``` C++
#include <bus/interface/businterfacefactory.h>
// The broker is a smart pointer (unique_ptr)
auto client = BusInterfaceFactory::CreateBroker(
    BrokerType::TcpClientType);
client->Name("TcpClient"); // Name for internal use only
client->Address("127.0.0.1");
client->Port(42612);
client->Start();    
```
