/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTOR_H
#define KUNIFIEDPUSH_CONNECTOR_H

#include "kunifiedpush_export.h"

#include <QObject>

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
    Q_PROPERTY(QString vapidPublicKey READ vapidPublicKey WRITE setVapidPublicKey NOTIFY vapidPublicKeyChanged)
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
    [[nodiscard]] QString endpoint() const;

    /** Register this client.
     *  This is for subscribing to push notifications and is only needed the first
     *  time an application uses this, or after explicitly unregistering. The registration
     *  is persisted until explicitly changed.
     *  @param description A human-readable explanation what push notifications are used
     *  for by this application.
     */
    void registerClient(const QString &description);

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
    [[nodiscard]] State state() const;
    // TODO error message

    /** Returns the VAPID public key of the corresponding application.
     *  @see setVapidPublicKey
     *  @since 25.08
     */
    [[nodiscard]] QString vapidPublicKey() const;

    /** Sets the Voluntary Application Server Identification (VAPID) public key of the corresponding application.
     *
     *  This is a public key on the P-256 curve encoded in the uncompressed form and BASE64 URL encoded.
     *  This is used by the application server to identify itself to the push server, following RFC8292.
     *
     *  @note This must be called before calling registerClient()!
     *
     *  @see RFC 8292
     *
     *  @since 25.08
     */
    void setVapidPublicKey(const QString &vapidPublicKey);

Q_SIGNALS:
    /** Emitted for each newly received push message. */
    void messageReceived(const QByteArray &msg);

    /** Emitted when a new endpoint URL has been received. */
    void endpointChanged(const QString &endpoint);

    /** Emitted when the connector state changes. */
    void stateChanged(KUnifiedPush::Connector::State state);

    /** Emitted when the VAPID public key changed.
     *  @since 25.08
     */
    void vapidPublicKeyChanged();

private:
    ConnectorPrivate *const d;
};

}

#endif
