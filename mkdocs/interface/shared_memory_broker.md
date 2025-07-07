# Shared Memory Broker
The shared memory broker object is used by both publishers and subscribers application.
One application shall be responsible for creating the shared memory. 
This application shall call the Start() function while the other applications shouldn't 
call the Start() function.

``` C++
#include <bus/interface/businterfacefactory.h>
// The broker is a smart pointer (unique_ptr)
auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryBrokerType);
broker->Name("BusMemory"); // Share memory name
broker->Start();    
```

