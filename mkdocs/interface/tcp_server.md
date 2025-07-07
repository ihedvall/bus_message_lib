# TCP/IP Server
Implement a TCP/IP server with no shared memory bus. 
It operates against one or more TCP/IP clients. 
A typical application is bidirectional communication with a physical driver.
This application shall call the Start() function.

``` C++
#include <bus/interface/businterfacefactory.h>
// The broker is a smart pointer (unique_ptr)
auto server = BusInterfaceFactory::CreateBroker(
    BrokerType::TcpServerType);
server->Name("TcpServer"); // Name for internal use only
server->Address("127.0.0.1");
server->Port(42612);
server->Start();    
```
