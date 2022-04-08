# KUnifiedPush

[UnifiedPush](https://unifiedpush.org) client components.

## Client Library

### Getting Started

How to register for push notifications:
- Register on D-Bus with a service name
- Make your application D-Bus activatable
- Create an instance of KUnifiedPush::Connector.
- Communicate the endpoint URL provided by this to the corresponding server-side application.
- Connect to its `messageReceived` signal.

### Supported Distributors

In theory any standard compliant D-Bus distributor is supported, however
standard compliance seems hard...

The following distributors have been tested:
- `gotify-dbus-rust`

## D-Bus Distributor

TODO
