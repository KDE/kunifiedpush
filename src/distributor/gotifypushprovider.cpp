/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gotifypushprovider.h"
#include "client.h"
#include "logging.h"
#include "message.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QUrlQuery>
#include <QWebSocket>

using namespace KUnifiedPush;

GotifyPushProvider::GotifyPushProvider(QObject *parent)
    : AbstractPushProvider(parent)
{
    qCDebug(Log);
}

void GotifyPushProvider::loadSettings(const QSettings &settings)
{
    m_clientToken = settings.value(QStringLiteral("ClientToken"), QString()).toString();
    m_url = QUrl(settings.value(QStringLiteral("Url"), QString()).toString());
    qCDebug(Log) << m_clientToken << m_url;
}

void GotifyPushProvider::connectToProvider()
{
    qCDebug(Log);
    m_socket = new QWebSocket();
    m_socket->setParent(this);
    connect(m_socket, &QWebSocket::stateChanged, this, [this]() {
        qCDebug(Log) << m_socket->state();
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, &GotifyPushProvider::wsMessageReceived);

    auto wsUrl = m_url;
    if (wsUrl.scheme() == QLatin1String("https")) {
        wsUrl.setScheme(QStringLiteral("wss"));
    } else if (wsUrl.scheme() == QLatin1String("http")) {
        wsUrl.setScheme(QStringLiteral("ws"));
    } else {
        qCWarning(Log) << "Unknown URL scheme:" << m_url;
        return;
    }

    auto path = wsUrl.path();
    path += QLatin1String("/stream");
    wsUrl.setPath(path);

    QUrlQuery query(wsUrl);
    query.addQueryItem(QStringLiteral("token"), m_clientToken);
    wsUrl.setQuery(query);

    m_socket->open(wsUrl);
}

void GotifyPushProvider::wsMessageReceived(const QString &msg)
{
    qCDebug(Log) << msg;
    const auto msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    Message m;
    m.clientRemoteId = QString::number(msgObj.value(QLatin1String("appid")).toInt());
    m.content = msgObj.value(QLatin1String("message")).toString();
    Q_EMIT messageReceived(m);
}

void GotifyPushProvider::registerClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token;
}

void GotifyPushProvider::unregisterClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token << client.remoteId;
}
