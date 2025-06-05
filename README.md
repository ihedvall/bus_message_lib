# Bus Message Interface Library

This library is intended to create a generic interface to various communication buses.
The purpose is to isolate the communication-driver-specific code from a GUI or service application.
The communication between an application and its bus driver uses shared memory or TCP/IP connections.

The following communication buses are supported:
- **CAN**.
- **LIN**.
- **FlexRay**.
- **MOST**.
- **ETH**.

The main library handles the serialization and bus message definition, while the sub-library
defines the shared memory and TCP/IP interfaces.