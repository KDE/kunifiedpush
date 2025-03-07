/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_ABSTRACTPUSHPROVIDER_H
#define KUNIFIEDPUSH_ABSTRACTPUSHPROVIDER_H

#include <QObject>

class QNetworkAccessManager;
class QSettings;

namespace KUnifiedPush {

class Client;
class Message;

/** Base class for push provider protocol implementations.
 *  Needed to support different push providers as that part of the
 *  protocol is not part of the UnifiedPush spec.
 */
class AbstractPushProvider : public QObject
{
    Q_OBJECT
public:
    ~AbstractPushProvider();

    enum Error {
        NoError, ///< operation succeeded
        ProviderRejected, ///< communication worked, but the provider refused to complete the operation
        TransientNetworkError, ///< temporary network error, try again
    };
    Q_ENUM(Error)

    /** Load connection settings.
     *  @param settings can be read on the top level, the correct group is already selected.
     *  @returns @c true if the settings are valid, @c false otherwise.
     */
    virtual bool loadSettings(const QSettings &settings) = 0;

    /** Attempt to establish a connection to the push provider. */
    virtual void connectToProvider() = 0;

    /** Disconnect and existing connection to the push provider. */
    virtual void disconnectFromProvider() = 0;

    /** Register a new client with the provider. */
    virtual void registerClient(const Client &client) = 0;

    /** Unregister a client from the provider. */
    virtual void unregisterClient(const Client &client) = 0;

    /** Provider id used e.g. in settings. */
    [[nodiscard]] QLatin1StringView providerId() const;

Q_SIGNALS:
    /** Inform about a received push notification. */
    void messageReceived(const KUnifiedPush::Message &msg);

    /** Emitted after successful client registration. */
    void clientRegistered(const KUnifiedPush::Client &client, KUnifiedPush::AbstractPushProvider::Error error = NoError, const QString &errorMsg = {});

    /** Emitted after successful client unregistration. */
    void clientUnregistered(const KUnifiedPush::Client &client, KUnifiedPush::AbstractPushProvider::Error error = NoError);

    /** Emitted after the connection to the push provider has been established successfully. */
    void connected();

    /** Emitted after the connection to the push provider disconnected or failed to be established. */
    void disconnected(KUnifiedPush::AbstractPushProvider::Error error, const QString &errorMsg = {});

protected:
    explicit AbstractPushProvider(QLatin1StringView providerId, QObject *parent);
    QNetworkAccessManager *nam();

private:
    const QLatin1StringView m_providerId;
    QNetworkAccessManager *m_nam = nullptr;
};

}

#endif // KUNIFIEDPUSH_ABSTRACTPUSHPROVIDER_H
