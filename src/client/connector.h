/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTOR_H
#define KUNIFIEDPUSH_CONNECTOR_H

#include "kunifiedpush_export.h"

#include <QObject>

#include <memory>

/** Client-side integration with UnifiedPush. */
namespace KUnifiedPush {

class ConnectorPrivate;

/** Client connector to UnifiedPush.
 *  Registers with a local UnifedPush distributor if present
 *  and obtains an endpoint that can be used for push notifications.
 */
class KUNIFIEDPUSH_EXPORT Connector : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString endpoint READ endpoint NOTIFY endpointChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
public:
    /** Create a new connector instance.
     *  @param serviceName The application identifier, same as used for registration
     *  on D-Bus and for D-Bus activation.
     */
    explicit Connector(const QString &serviceName, QObject *parent = nullptr);
    ~Connector();

    /** HTTP endpoint to trigger the push notification.
     *  This needs to be communicated to the corresponding server-side application.
     *  @see endpointChanged
     */
    QString endpoint() const;

    /** Register this client.
     *  This is for subscribing to push notifications and is only needed the first
     *  time an application uses this, or after explicitly unregistering. The registration
     *  is persisted until explicitly changed.
     */
    void registerClient();

    /** Unregister this client.
     *  This is for permanently unsubscribing, do not use on regular application shutdown.
     */
    void unregisterClient();

    /** Connector state. */
    enum State {
        Unregistered, ///< Connector is not yet registered, or explicitly unregistered.
        Registering, ///< Connector is registering with the push provider.
        Registered, ///< Connector is registered and thus operational.
        NoDistributor, ///< Connector cannot find a UnifiedPush distributor to register at.
        Error, ///< Any other error condition.
    };
    Q_ENUM(State)
    /** State of the connector. */
    State state() const;
    // TODO error message

Q_SIGNALS:
    /** Emitted for each newly received push message. */
    // TODO this is supposed to be a byte array, but gotify deviates from the spec...
    void messageReceived(const QString &msg);

    /** Emitted when a new endpoint URL has been received. */
    void endpointChanged(const QString &endpoint);

    /** Emitted when the connector state changes. */
    void stateChanged(State state);

private:
    ConnectorPrivate *d;
};

}

#endif
