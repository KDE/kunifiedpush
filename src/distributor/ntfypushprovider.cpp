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

using namespace Qt::Literals;
using namespace KUnifiedPush;

NtfyPushProvider::NtfyPushProvider(QObject *parent)
    : AbstractPushProvider(Id, parent)
{
    connect(&m_sseStream, &ServerSentEventsStream::messageReceived, this, [this](const SSEMessage &sse) {
        qCDebug(Log) << sse.event << sse.data;
        if (sse.event.isEmpty()) {
            QJsonObject msgObj = QJsonDocument::fromJson(sse.data).object();
            Message msg;
            msg.clientRemoteId = msgObj.value("topic"_L1).toString();
            msg.content = msgObj.value("message"_L1).toString().toUtf8();
            if (msgObj.value("encoding"_L1).toString() == "base64"_L1) {
                msg.content = QByteArray::fromBase64(msg.content);
            }

            const auto msgId = msgObj.value("id"_L1).toString();

            // encrypted messages come in as attachments apparently, so resolve those here
            if (const auto attachment = msgObj.value("attachment"_L1).toObject(); !attachment.isEmpty()) {
                const auto attachmentUrl = attachment.value("url"_L1).toString();
                if (!attachmentUrl.isEmpty()) {
                    auto reply = nam()->get(QNetworkRequest(QUrl(attachmentUrl)));
                    connect(reply, &QNetworkReply::finished, this, [this, reply, msgId, msg]() {
                        reply->deleteLater();
                        if (reply->error() != QNetworkReply::NoError) {
                            qCWarning(Log) << reply->errorString() << reply->readAll();
                            return;
                        }
                        Message m(msg);
                        m.content = reply->readAll();
                        m_lastMessageId = msgId;
                        Q_EMIT messageReceived(m);
                        storeState();
                    });
                    return;
                }
            }
            m_lastMessageId = msgId;
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
    internal.beginGroup(providerId() + "-internal"_L1);
    m_topics = internal.value(QStringLiteral("Topics"), QStringList()).toStringList();
    m_lastMessageId = internal.value(QStringLiteral("LastMessageId"), QString()).toString();

    return m_url.isValid();
}

void NtfyPushProvider::connectToProvider(Urgency urgency)
{
    doConnectToProvider(urgency);
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
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    path += topic;
    endpoint.setPath(path);
    newClient.endpoint = endpoint.toString();

    m_topics.push_back(topic);
    storeState();
    doConnectToProvider(urgency());
    Q_EMIT clientRegistered(newClient);
}

void NtfyPushProvider::unregisterClient(const Client &client)
{
    m_topics.removeAll(client.remoteId);
    storeState();
    doConnectToProvider(urgency());
    Q_EMIT clientUnregistered(client);
}

#if 0
// TODO see doConnectToProvider
void NtfyPushProvider::doChangeUrgency(Urgency urgency)
{
    doConnectToProvider(urgency);
}
#endif

void NtfyPushProvider::doConnectToProvider(Urgency urgency)
{
    if (m_sseReply) {
        m_sseReply->abort();
    }
    m_sseStream.clear();

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
    // TODO urgency filter
    // ntfy's "priority" comes close to this, but first would need RFC 8030 compliant urgency support for incoming messages
    // before we can add a corresponding filter here, otherwise Web Push message will not get correct urgency levels assigned
    // and we'd miss high priority ones
    url.setQuery(query);
    qCDebug(Log) << url << qToUnderlying(urgency);

    auto reply = nam()->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return; // we triggered this ourselves
        }

        qCDebug(Log) << reply->error() << reply->errorString() << m_sseStream.buffer();
        const auto obj = QJsonDocument::fromJson(m_sseStream.buffer()).object();
        if (const auto errMsg = obj.value("error"_L1).toString(); !errMsg.isEmpty()) {
            Q_EMIT disconnected(ProviderRejected, errMsg);
            return;
        }

        Q_EMIT disconnected(TransientNetworkError, reply->errorString());
    });

    m_sseReply = reply;
    m_sseStream.read(reply);

    // TODO urgency filter
    // setUrgency(urgency);
    // Q_EMIT urgencyChanged();
}

void NtfyPushProvider::storeState()
{
    QSettings settings;
    settings.beginGroup(providerId() + "-internal"_L1);
    settings.setValue(QStringLiteral("Topics"), m_topics);
    settings.setValue(QStringLiteral("LastMessageId"), m_lastMessageId);
}

#include "moc_ntfypushprovider.cpp"
