# Brokers and Servers
The library implements basic communication interfaces using shared memory or TCP/IP communication.
There are two basic application types, broker or server applications.

## Brokers
A broker application implements a single internal bus with one or more publishers 
and one or more subscribers. 
All subscribers receive all the published messages.
Brokers implement the internal bus with shared memory.
Broker applications are typically used for logger applications.

## Servers
A server application implements two internal buses, one TX and one RX bus.
In this context, there is one server and one client. 
The server can handle multiple clients, but this may cause some issues.
Server applications are typically used when connecting to physical drivers.

## Publisher and Subscriber Queues
Each broker/server can create publishers and subscribers.
Publisher sends messages while subscribers receive messages.
Both publishers and subscribers implement a temporary in-memory FIFO queue.

Publishers are created by calling the CreatePublisher() function.
Similar are the subscribers created by calling the CreateSubscriber() function.
Remember to call the Start() function.

``` C++
// The publisher is a smart pointer (shared_ptr)
auto publisher = broker->CreatePublisher();
publisher->Start(); 
```


