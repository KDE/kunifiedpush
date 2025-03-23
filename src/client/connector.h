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
    Q_PROPERTY(bool vapidPublicKeyRequired READ vapidPublicKeyRequired WRITE setVapidPublicKeyRequired NOTIFY vapidPublicKeyRequiredChanged)
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
     *
     *  @see setVapidPublicKey
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
     *  The VAPID public key is persisted, ie. you don't need to store it separately in the application.
     *
     *  @note This should be either called before calling registerClient() or vapidPublicKeyRequired should
     *  be set to @c true.
     *
     *  @see RFC 8292
     *
     *  @since 25.08
     */
    void setVapidPublicKey(const QString &vapidPublicKey);

    /** Returns whether a VAPID public key is required before registering
     *  with the push provider.
     *  @see setVapidPublicKeyRequired
     *  @since 25.08
     */
    [[nodiscard]] bool vapidPublicKeyRequired() const;

    /** Sets whether a Voluntary Application Server Identification (VAPID) public key
     *  is required before registering with the push provider.
     *
     *  When this is set, calling registerClient() will wait for a VAPID key to be set
     *  via setVapidPublicKey(). This is useful when the VAPID key has first to be retrieved
     *  asynchronously from the application server.
     *
     *  @see setVapidPublicKey
     *
     *  @since 25.08
     */
    void setVapidPublicKeyRequired(bool vapidRequired);

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

    /** Emitted when the VAPID public key required property changed.
     *  @since 25.08
     */
    void vapidPublicKeyRequiredChanged();

private:
    ConnectorPrivate *const d;
};

}

#endif
