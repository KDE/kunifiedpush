/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ntfypushprovider.h"
#include "client.h"
#include "logging.h"
#include "message.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QUrlQuery>
#include <QUuid>

using namespace KUnifiedPush;

NtfyPushProvider::NtfyPushProvider(QObject *parent)
    : AbstractPushProvider(Id, parent)
{
    connect(&m_sseStream, &ServerSentEventsStream::messageReceived, this, [this](const SSEMessage &sse) {
        qCDebug(Log) << sse.event << sse.data;
        if (sse.event.isEmpty()) {
            QJsonObject msgObj = QJsonDocument::fromJson(sse.data).object();
            Message msg;
            msg.clientRemoteId = msgObj.value(QLatin1String("topic")).toString();
            msg.content = msgObj.value(QLatin1String("message")).toString().toUtf8();
            if (msgObj.value(QLatin1String("encoding")).toString() == QLatin1String("base64")) {
                msg.content = QByteArray::fromBase64(msg.content);
            }
            m_lastMessageId = msgObj.value(QLatin1String("id")).toString();
            Q_EMIT messageReceived(msg);
            storeState();
        }
    });
}

NtfyPushProvider::~NtfyPushProvider() = default;

bool NtfyPushProvider::loadSettings(const QSettings &settings)
{
    m_url = settings.value(QStringLiteral("Url"), QUrl()).toUrl();

    QSettings internal;
    internal.beginGroup(QLatin1String(providerId()) + QLatin1String("-internal"));
    m_topics = internal.value(QStringLiteral("Topics"), QStringList()).toStringList();
    m_lastMessageId = internal.value(QStringLiteral("LastMessageId"), QString()).toString();

    return m_url.isValid();
}

void NtfyPushProvider::connectToProvider()
{
    doConnectToProvider();
    Q_EMIT connected();
}

void NtfyPushProvider::disconnectFromProvider()
{
    if (m_sseReply) {
        m_sseReply->abort();
    }
    Q_EMIT disconnected(NoError);
}

void NtfyPushProvider::registerClient(const Client &client)
{
    const QString topic = QLatin1String("upk") + QUuid::createUuid().toString(QUuid::Id128);
    auto newClient = client;
    newClient.remoteId = topic;

    QUrl endpoint = m_url;
    auto path = endpoint.path();
    path  += QLatin1Char('/') + topic;
    endpoint.setPath(path);
    newClient.endpoint = endpoint.toString();

    m_topics.push_back(topic);
    storeState();
    doConnectToProvider();
    Q_EMIT clientRegistered(newClient);
}

void NtfyPushProvider::unregisterClient(const Client &client)
{
    m_topics.removeAll(client.remoteId);
    storeState();
    doConnectToProvider();
    Q_EMIT clientUnregistered(client);
}

void NtfyPushProvider::doConnectToProvider()
{
    if (m_sseReply) {
        m_sseReply->abort();
    }

    if (m_topics.empty()) {
        return;
    }

    QUrl url = m_url;
    QString path = url.path();
    path += QLatin1Char('/') + m_topics.join(QLatin1Char(',')) + QLatin1String("/sse");
    url.setPath(path);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("up"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("since"), m_lastMessageId.isEmpty() ? QStringLiteral("all") : m_lastMessageId);
    url.setQuery(query);
    qCDebug(Log) << url;

    auto reply = nam()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return; // we triggered this ourselves
        }
        qCDebug(Log) << reply->error() << reply->errorString();
        Q_EMIT disconnected(TransientNetworkError, reply->errorString());
    });

    m_sseReply = reply;
    m_sseStream.read(reply);
}

void NtfyPushProvider::storeState()
{
    QSettings settings;
    settings.beginGroup(QLatin1String(providerId()) + QLatin1String("-internal"));
    settings.setValue(QStringLiteral("Topics"), m_topics);
    settings.setValue(QStringLiteral("LastMessageId"), m_lastMessageId);
}
