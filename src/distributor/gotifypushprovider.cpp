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
#include <QNetworkAccessManager>
#include <QNetworkReply>
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

    QUrl url = m_url;
    auto path = url.path();
    path += QLatin1String("/application");
    url.setPath(path);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("X-Gotify-Key", m_clientToken.toUtf8());

    QJsonObject content;
    content.insert(QLatin1String("name"), client.serviceName);
    content.insert(QLatin1String("token"), client.token);

    auto reply = nam()->post(req, QJsonDocument(content).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, this, client]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(Log) << reply->errorString();
            return;
        }

        const auto replyObj = QJsonDocument::fromJson(reply->readAll()).object();
        qCDebug(Log) << QJsonDocument(replyObj).toJson(QJsonDocument::Compact);

        auto newClient = client;
        newClient.remoteId = QString::number(replyObj.value(QLatin1String("id")).toInt());

        QUrl endpointUrl = m_url;
        auto path = endpointUrl.path();
        path += QLatin1String("/message");
        endpointUrl.setPath(path);
        QUrlQuery query(endpointUrl);
        query.addQueryItem(QStringLiteral("token"), replyObj.value(QLatin1String("token")).toString());
        endpointUrl.setQuery(query);
        newClient.endpoint = endpointUrl.toString();

        Q_EMIT clientRegistered(newClient);
    });
}

void GotifyPushProvider::unregisterClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token << client.remoteId;
}