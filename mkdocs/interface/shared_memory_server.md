# Shared Memory Server
A Shared Memory Server has two shared memory queues, one TX-queue and one RX-queue.
A broker has only one common queue. 
The server should work together with a Shared Memory Client.
Although it's possible to use more than one client in an application,
doing so may cause application problems.
A typical application is bidirectional communication with a physical driver.
This application shall call the Start() function 

``` C++
#include <bus/interface/businterfacefactory.h>
// The broker is a smart pointer (unique_ptr)
auto server = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryServerType);
server->Name("BusTxRxMemory"); // Share memory name
server->Start();    
```
