# KUnifiedPush

[UnifiedPush](https://unifiedpush.org) client library and distributor daemon.

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
- `kunifiedpush-distributor`
- `gotify-dbus-rust`

## D-Bus Distributor

UnifiedPush only specifies the protocol between distributor and client as well as
between the server-side application and the push provider, but not the one between
the push provider and the distributor. Supporting different push providers therefore
requires provider specific code and configuration.

### Configuration

There is no configuration KCM for the push provider yet, this can only be done via
the `~/.config/KDE/kunifiedpush-distributor.conf` configuration file.

```
[PushProvider]
Type=[Gotify|NextPush]
```

### Supported push providers

#### Gotify

https://gotify.net/

Configuration:

```
[Gotify]
ClientToken=xxx
Url=https://my.server.org
```

#### NextPush

https://github.com/UP-NextPush/server-app

Configuration:

```
[NextPush]
AppPassword=xxx
Url=https://my.server.org
Username=user
```
