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

There's also support for the following security capabilities, in case your application server
supports any of those:
- Message Encryption for Web Push ([RFC 8291](https://datatracker.ietf.org/doc/rfc8291/)).
- Voluntary Application Server Identification (VAPID) ([RFC 8292](https://datatracker.ietf.org/doc/rfc8292/)).

### Supported Distributors

For the basic functionality any standard compliant D-Bus distributor should work.
The graphical status and configuration user interface however only works with
the `kunifiedpush-distributor`, as that needs a lot more access to distributor
internal details than the standard interfaces provide.

## D-Bus Distributor

UnifiedPush only specifies the protocol between distributor and client as well as
between the server-side application and the push provider, but not the one between
the push provider and the distributor. Supporting different push providers therefore
requires provider specific code and configuration.

### Configuration

There is a configuration KCM, alternatively this can be configured via
the `~/.config/KDE/kunifiedpush-distributor.conf` configuration file.

```
[PushProvider]
Type=[Gotify|Autopush|NextPush|Ntfy]
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

#### Mozilla WebPush

https://github.com/mozilla-services/autopush-rs

Configuration:

```
[Autopush]
Url=https://push.services.mozilla.com
````

#### NextPush

https://github.com/UP-NextPush/server-app

Configuration:

```
[NextPush]
AppPassword=xxx
Url=https://my.server.org
Username=user
```

#### Ntfy

https://ntfy.sh/

Configuration:

```
[Ntfy]
Url=https://ntfy.sh
```

Ntfy can be self-hosted, but also provides a hosted instance at `https://ntfy.sh` that
works without requiring an account.

## Push Notification KCM

In combination with our own distributor this allows configuring the push provider to use
in System Settings, check the distributor status, and see all apps currently using push
notifications.
