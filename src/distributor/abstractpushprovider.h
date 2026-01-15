/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_ABSTRACTPUSHPROVIDER_H
#define KUNIFIEDPUSH_ABSTRACTPUSHPROVIDER_H

#include "../shared/urgency_p.h"

#include <QObject>

class QNetworkAccessManager;
class QSettings;

namespace KUnifiedPush {

class Client;
class Message;

/*!
 * \class KUnifiedPush::AbstractPushProvider
 * \inmodule KUnifiedPush
 * \brief Base class for push provider protocol implementations.
 *
 *  Needed to support different push providers as that part of the
 *  protocol is not part of the UnifiedPush spec.
 */
class AbstractPushProvider : public QObject
{
    Q_OBJECT
public:
    ~AbstractPushProvider();

    /*!
     * \enum KUnifiedPush::AbstractPushProvider::Error
     * \value NoError
     *        Operation succeeded
     * \value ProviderRejected
     *        Communication worked, but the provider refused to complete the operation
     * \value TransientNetworkError
     *        Temporary network error, try again
     * \value ActionRequired
     *        User interaction is required to resolve the problem, e.g. no push server set up
     */
    enum Error {
        NoError,
        ProviderRejected,
        TransientNetworkError,
        ActionRequired,
    };
    Q_ENUM(Error)

    /*! Load connection settings.
     *
     *  \a settings can be read on the top level, the correct group is already selected.
     */
    virtual bool loadSettings(const QSettings &settings) = 0;

    /*! Reset any internal state for a fresh setup connecting to
     *  a different push server instance.
     *
     *  The default implementation does nothing.
     */
    virtual void resetSettings(QSettings &settings);

    /*! Attempt to establish a connection to the push provider. */
    virtual void connectToProvider(Urgency urgency) = 0;

    /*! Disconnect and existing connection to the push provider. */
    virtual void disconnectFromProvider() = 0;

    /*! Register a new client with the provider. */
    virtual void registerClient(const Client &client) = 0;

    /*! Unregister a client from the provider. */
    virtual void unregisterClient(const Client &client) = 0;

    /*! Acknowledge a message.
     *
     *  The default implementation does nothing apart from indicating successful completion.
     */
    virtual void acknowledgeMessage(const Client &client, const QString &messageIdentifier);

    /*! Change urgency level as needed.
     *
     *  Reimplement doChangeUrgency if your provider does this as a separate command.
     */
    void changeUrgency(Urgency urgency);

    /*! Provider id used e.g. in settings. */
    [[nodiscard]] QLatin1StringView providerId() const;

    /*! The urgency level currently used by this provider.
     *
     *  This might not yet be the requested one if changing that is an asynchronous operation.
     */
    [[nodiscard]] Urgency urgency() const;

Q_SIGNALS:
    /*! Inform about a received push notification. */
    void messageReceived(const KUnifiedPush::Message &msg);

    /*! Emitted after successful client registration. */
    void clientRegistered(const KUnifiedPush::Client &client, KUnifiedPush::AbstractPushProvider::Error error = NoError, const QString &errorMsg = {});

    /*! Emitted after successful client unregistration. */
    void clientUnregistered(const KUnifiedPush::Client &client, KUnifiedPush::AbstractPushProvider::Error error = NoError);

    /*! Emitted after the connection to the push provider has been established successfully. */
    void connected();

    /*! Emitted after the connection to the push provider disconnected or failed to be established. */
    void disconnected(KUnifiedPush::AbstractPushProvider::Error error, const QString &errorMsg = {});

    /*! Emitted after a message reception has been acknowledge to the push server. */
    void messageAcknowledged(const KUnifiedPush::Client &client, const QString &messageIdentifier);

    /*! Emitted when the urgency level change request has been executed. */
    void urgencyChanged();

protected:
    /*! Re-implement if urgency leve changes are done as a separate command.
     *
     *  The default implementation assumes urgency levels aren't supported by this provider.
     */
    virtual void doChangeUrgency(Urgency urgency);
    /*!
     */
    inline void setUrgency(Urgency urgency) { m_currentUrgency = urgency; }

    explicit AbstractPushProvider(QLatin1StringView providerId, QObject *parent);
    QNetworkAccessManager *nam();

private:
    const QLatin1StringView m_providerId;
    QNetworkAccessManager *m_nam = nullptr;
    Urgency m_currentUrgency = AllUrgencies;
};

}

#endif // KUNIFIEDPUSH_ABSTRACTPUSHPROVIDER_H
