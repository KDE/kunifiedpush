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
#include <QNetworkReply>
#include <QSettings>
#include <QUrlQuery>
#include <QWebSocket>

using namespace KUnifiedPush;

GotifyPushProvider::GotifyPushProvider(QObject *parent)
    : AbstractPushProvider(Id, parent)
{
    qCDebug(Log);
}

bool GotifyPushProvider::loadSettings(const QSettings &settings)
{
    m_clientToken = settings.value(QStringLiteral("ClientToken"), QString()).toString();
    m_url = QUrl(settings.value(QStringLiteral("Url"), QString()).toString());
    qCDebug(Log) << m_clientToken << m_url;
    return m_url.isValid() && !m_clientToken.isEmpty();
}

void GotifyPushProvider::connectToProvider()
{
    qCDebug(Log);
    m_socket = new QWebSocket();
    m_socket->setParent(this);
    connect(m_socket, &QWebSocket::stateChanged, this, [this](auto state) {
        qCDebug(Log) << m_socket->state();
        if (state == QAbstractSocket::ConnectedState) {
            Q_EMIT connected();
        } else if (state == QAbstractSocket::UnconnectedState) {
            Q_EMIT disconnected(TransientNetworkError, m_socket->errorString());
            m_socket->deleteLater();
        }
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, &GotifyPushProvider::wsMessageReceived);

    auto wsUrl = m_url;
    if (wsUrl.scheme() == QLatin1String("https")) {
        wsUrl.setScheme(QStringLiteral("wss"));
    } else if (wsUrl.scheme() == QLatin1String("http")) {
        wsUrl.setScheme(QStringLiteral("ws"));
    } else {
        qCWarning(Log) << "Unknown URL scheme:" << m_url;
        Q_EMIT disconnected(ProviderRejected);
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

void GotifyPushProvider::disconnectFromProvider()
{
    m_socket->close();
    m_socket = nullptr;
}

void GotifyPushProvider::wsMessageReceived(const QString &msg)
{
    qCDebug(Log) << msg;
    const auto msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    Message m;
    m.clientRemoteId = QString::number(msgObj.value(QLatin1String("appid")).toInt());
    m.content = msgObj.value(QLatin1String("message")).toString().toUtf8();
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
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT clientRegistered(client, TransientNetworkError, reply->errorString());
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

    QUrl url = m_url;
    auto path = url.path();
    path += QLatin1String("/application/") + client.remoteId;
    url.setPath(path);

    QNetworkRequest req(url);
    req.setRawHeader("X-Gotify-Key", m_clientToken.toUtf8());

    auto reply = nam()->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [reply, client, this]() {
        reply->deleteLater();
        qCDebug(Log) << reply->errorString() << reply->readAll(); // TODO
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT clientUnregistered(client, TransientNetworkError);
        } else {
            Q_EMIT clientUnregistered(client);
        }
    });
}

#include "moc_gotifypushprovider.cpp"
