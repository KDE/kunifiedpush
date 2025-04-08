/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_AUTOPUSHPROVIDER_H
#define KUNIFIEDPUSH_AUTOPUSHPROVIDER_H

#include "abstractpushprovider.h"
#include "client.h"

#include <QUrl>
#include <QTimer>

class QWebSocket;

namespace KUnifiedPush {

/** Push provider protocol implementation for Mozilla Autopush.
 *  @see https://github.com/mozilla-services/autopush-rs
 */
class AutopushProvider : public AbstractPushProvider
{
    Q_OBJECT
public:
    explicit AutopushProvider(QObject *parent = nullptr);

    bool loadSettings(const QSettings &settings) override;
    void connectToProvider(Urgency urgency) override;
    void disconnectFromProvider() override;
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;
    void acknowledgeMessage(const Client &client, const QString &messageIdentifier) override;

    static constexpr inline auto Id = QLatin1StringView("Autopush");

protected:
    void doChangeUrgency(Urgency urgency) override;

private:
    void sendMessage(const QJsonObject &msg);
    void wsMessageReceived(const QString &msg);
    void storeState();

    QString m_uaid;
    QUrl m_url;
    QWebSocket *m_socket = nullptr;
    Client m_currentClient;
    QString m_currentMessageId;
    QTimer m_pingTimer;
};

}

#endif // KUNIFIEDPUSH_AUTOPUSHPROVIDER_H
