/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_GOTIFYPUSHPROVIDER_H
#define KUNIFIEDPUSH_GOTIFYPUSHPROVIDER_H

#include "abstractpushprovider.h"

#include <QUrl>

class QWebSocket;

namespace KUnifiedPush {

/** Push provider protocol implementation for Gotify. */
class GotifyPushProvider : public AbstractPushProvider
{
    Q_OBJECT
public:
    explicit GotifyPushProvider(QObject *parent = nullptr);

    bool loadSettings(const QSettings &settings) override;
    void connectToProvider() override;
    void disconnectFromProvider() override;
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;

private:
    void wsMessageReceived(const QString &msg);

    QString m_clientToken;
    QUrl m_url;
    QWebSocket *m_socket;
};

}

#endif // KUNIFIEDPUSH_GOTIFYPUSHPROVIDER_H
