# Shared Memory Client
The Shared Memory Client connects to a shared memory created by a Shared Memory Server.
A Shared Memory Server has two shared memory queues, one TX-queue and one RX-queue.

``` C++
#include <bus/interface/businterfacefactory.h>
// The broker is a smart pointer (unique_ptr)
auto client = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryClientType);
client->Name("BusTxRxMemory"); // Share memory name
client->Start();    
```
