/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "autopushprovider.h"
#include "client.h"
#include "logging.h"
#include "message.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QUrlQuery>
#include <QWebSocket>

using namespace Qt::Literals;
using namespace KUnifiedPush;

AutopushProvider::AutopushProvider(QObject *parent)
    : AbstractPushProvider(Id, parent)
{
    qCDebug(Log);
}

bool AutopushProvider::loadSettings(const QSettings &settings)
{
    m_url = QUrl(settings.value("Url", QString()).toString());

    QSettings internal;
    internal.beginGroup(providerId() + "-internal"_L1);
    m_uaid = internal.value("UAID", QString()).toString();

    qCDebug(Log) << m_url << m_uaid;
    return m_url.isValid();
}

void AutopushProvider::connectToProvider()
{
    qCDebug(Log);
    m_socket = new QWebSocket();
    m_socket->setParent(this);
    connect(m_socket, &QWebSocket::stateChanged, this, [this](auto state) {
        qCDebug(Log) << m_socket->state();
        if (state == QAbstractSocket::ConnectedState) {
            QJsonObject msg {{
                { "messageType"_L1, "hello"_L1 },
            }};
            if (!m_uaid.isEmpty()) {
                msg.insert("uaid"_L1, m_uaid);
            }
            sendMessage(msg);
        } else if (state == QAbstractSocket::UnconnectedState) {
            Q_EMIT disconnected(TransientNetworkError, m_socket->errorString());
            m_socket->deleteLater();
        }
    });
    connect(m_socket, &QWebSocket::textMessageReceived, this, &AutopushProvider::wsMessageReceived);

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

    m_socket->open(wsUrl);
}

void AutopushProvider::disconnectFromProvider()
{
    m_socket->close();
    m_socket = nullptr;
}

void AutopushProvider::sendMessage(const QJsonObject &msg)
{
    m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact)));
}

void AutopushProvider::wsMessageReceived(const QString &msg)
{
    qCDebug(Log) << msg;
    const auto msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    const auto msgType = msgObj.value("messageType"_L1).toString();
    const auto status = msgObj.value("status"_L1).toInt();

    if (msgType == "hello"_L1 && status == 200) {
        m_uaid = msgObj.value("uaid"_L1).toString();
        storeState();
        Q_EMIT connected();
        return;
    }

    if (msgType == "register"_L1) {
        const auto channelId = msgObj.value("channelID"_L1).toString();
        if (m_currentClient.remoteId != channelId) {
            qCCritical(Log) << "Got registration for a different client!" << channelId << m_currentClient.remoteId;
            return;
        }
        m_currentClient.endpoint = msgObj.value("pushEndpoint"_L1).toString();
        Q_EMIT clientRegistered(m_currentClient, status == 200 ? NoError : ProviderRejected);
        m_currentClient = {};
        return;
    }

    if (msgType == "notification"_L1) {
        Message m;
        m.clientRemoteId = msgObj.value("channelID"_L1).toString();
        m.content = QByteArray::fromBase64(msgObj.value("data"_L1).toString().toLatin1(), QByteArray::Base64UrlEncoding);
        m.messageId = msgObj.value("version"_L1).toString();
        Q_EMIT messageReceived(m);
        return;
    }

    if (msgType == "unregister"_L1) {
        const auto channelId = msgObj.value("channelID"_L1).toString();
        if (m_currentClient.remoteId != channelId) {
            qCCritical(Log) << "Got unregistration for a different client!" << channelId << m_currentClient.remoteId;
            return;
        }
        Q_EMIT clientUnregistered(m_currentClient, status == 200 ? NoError : ProviderRejected);
        return;
    }
}

void AutopushProvider::registerClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token;

    m_currentClient = client;
    m_currentClient.remoteId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject msg{{
        { "messageType"_L1, "register"_L1 },
        { "channelID"_L1, m_currentClient.remoteId },
    }};
    if (!client.vapidKey.isEmpty()) {
        msg.insert("key"_L1, client.vapidKey);
    }

    qCDebug(Log) << msg;
    sendMessage(msg);
}

void AutopushProvider::unregisterClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token << client.remoteId;

    m_currentClient = client;
    QJsonObject msg{{
        { "messageType"_L1, "unregister"_L1 },
        { "channelID"_L1, client.remoteId },
    }};
    sendMessage(msg);
}

void AutopushProvider::acknowledgeMessage(const Client &client, const QString &messageIdentifier)
{
    qCDebug(Log) << client.serviceName << client.remoteId << messageIdentifier;

    QJsonObject msg{{
        { "messageType"_L1, "ack"_L1 },
        { "updates"_L1, QJsonArray{{
            QJsonObject{{
                { "channelID"_L1, client.remoteId },
                { "version"_L1, messageIdentifier }
            }}
        }}},
    }};
    sendMessage(msg);
    Q_EMIT messageAcknowledged(client, messageIdentifier);
}

void AutopushProvider::storeState()
{
    QSettings settings;
    settings.beginGroup(providerId() + "-internal"_L1);
    settings.setValue("UAID", m_uaid);
}

#include "moc_autopushprovider.cpp"
