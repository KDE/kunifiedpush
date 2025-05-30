/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "nextpushprovider.h"
#include "client.h"
#include "logging.h"
#include "message.h"

#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::Literals;
using namespace KUnifiedPush;

NextPushProvider::NextPushProvider(QObject *parent)
    : AbstractPushProvider(Id, parent)
{
    connect(&m_sseStream, &ServerSentEventsStream::messageReceived, this, [this](const SSEMessage &sse) {
        qCDebug(Log) << sse.event << sse.data << sse.metaData;
        if (sse.event == "message") {
            QJsonObject msgObj = QJsonDocument::fromJson(sse.data).object();
            Message msg;
            msg.messageId = QString::fromUtf8(sse.metaData.value("id"));
            msg.clientRemoteId = msgObj.value("token"_L1).toString();
            msg.content = QByteArray::fromBase64(msgObj.value("message"_L1).toString().toUtf8());
            Q_EMIT messageReceived(msg);
        }
        if (sse.event == "start") {
            Q_EMIT connected();
            Q_EMIT urgencyChanged();
        }
    });
}

NextPushProvider::~NextPushProvider() = default;

bool NextPushProvider::loadSettings(const QSettings &settings)
{
    m_appPassword = settings.value(QStringLiteral("AppPassword"), QString()).toString();
    m_url = QUrl(settings.value(QStringLiteral("Url"), QString()).toString());
    m_userName = settings.value(QStringLiteral("Username"), QString()).toString();
    m_deviceId = settings.value(QStringLiteral("DeviceId"), QString()).toString();
    qCDebug(Log) << m_url << m_userName << m_deviceId;
    return m_url.isValid() && !m_appPassword.isEmpty() && !m_userName.isEmpty();
}

void NextPushProvider::resetSettings(QSettings &settings)
{
    settings.remove("DeviceId");
}

void NextPushProvider::connectToProvider(Urgency urgency)
{
    qCDebug(Log) << m_deviceId;

    if (m_deviceId.isEmpty()) {
        auto req = prepareRequest("device");
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        QJsonObject content;
        content.insert(QLatin1String("deviceName"), QHostInfo::localHostName());
        auto reply = nam()->put(req, QJsonDocument(content).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [reply, urgency, this]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(Log) << reply->errorString();
                Q_EMIT disconnected(ProviderRejected, reply->errorString());
                return;
            }

            const auto content = QJsonDocument::fromJson(reply->readAll()).object();
            qCDebug(Log) << QJsonDocument(content).toJson(QJsonDocument::Compact);
            // TODO check "success" field
            m_deviceId = content.value(QLatin1String("deviceId")).toString();

            QSettings settings;
            settings.setValue(QStringLiteral("NextPush/DeviceId"), m_deviceId);
            waitForMessage(urgency);
        });
    } else {
        waitForMessage(urgency);
    }
}

void NextPushProvider::disconnectFromProvider()
{
    if (m_sseReply) {
        m_sseReply->abort();
    }
    Q_EMIT disconnected(NoError);
}

void NextPushProvider::registerClient(const Client &client)
{
    qCDebug(Log) << client.serviceName;
    auto req = prepareRequest("app");
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject content;
    content.insert(QLatin1String("deviceId"), m_deviceId);
    content.insert(QLatin1String("appName"), client.serviceName);
    if (!client.vapidKey.isEmpty()) {
        content.insert("vapid"_L1, client.vapidKey);
    }
    auto reply = nam()->put(req, QJsonDocument(content).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, this, client]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT clientRegistered(client, TransientNetworkError, reply->errorString());
            return;
        }

        const auto content = QJsonDocument::fromJson(reply->readAll()).object();
        qCDebug(Log) << QJsonDocument(content).toJson(QJsonDocument::Compact);
        if (!content.value(QLatin1String("success")).toBool()) {
            Q_EMIT clientRegistered(client, ProviderRejected, QString()); // TODO do we get an error message in this case?
            return;
        }
        auto newClient = client;
        newClient.remoteId = content.value(QLatin1String("token")).toString();

        QUrl endpointUrl = m_url;
        auto path = endpointUrl.path();
        path += QLatin1String("/index.php/apps/uppush/push/") + newClient.remoteId;
        endpointUrl.setPath(path);
        newClient.endpoint = endpointUrl.toString();

        Q_EMIT clientRegistered(newClient);
    });
}

void NextPushProvider::unregisterClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.remoteId;
    auto req = prepareRequest("app", client.remoteId);
    auto reply = nam()->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [reply, this, client]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(Log) << reply->errorString();
            Q_EMIT clientUnregistered(client, TransientNetworkError);
        } else {
            qCDebug(Log) << "application deleted";
            Q_EMIT clientUnregistered(client);
        }
    });
}

void NextPushProvider::acknowledgeMessage(const Client &client, const QString &messageIdentifier)
{
    qCDebug(Log) << client.serviceName << messageIdentifier;
    auto req = prepareRequest("message", messageIdentifier);
    auto reply = nam()->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [reply, this, client, messageIdentifier]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(Log) << reply->errorString() << reply->readAll();
        };
        Q_EMIT messageAcknowledged(client, messageIdentifier);
    });
}

void NextPushProvider::doChangeUrgency(Urgency urgency)
{
    waitForMessage(urgency);
}

void NextPushProvider::waitForMessage(Urgency urgency)
{
    qCDebug(Log) << qToUnderlying(urgency);
    if (m_sseReply) {
        m_sseReply->abort();
    }

    // HACK For unknown reasons doing SSE requests on the same
    // QNAM instance as the other HTTP operations here will
    // prevent other requests from happening while an SSE connection is
    // active and will also prevent an SSE connection from being
    // re-established after a disconnect.
    // This doesn't match the behavior in the Ntfy implementation nor
    // the QNAM documentation. Using a new QNAM per SSE does avoid
    // those problems.
    auto nam = new QNetworkAccessManager(this);
    nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    nam->setStrictTransportSecurityEnabled(true);
    nam->enableStrictTransportSecurityStore(true, QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/org.kde.kunifiedpush/hsts/"_L1);

    auto req = prepareRequest("device", m_deviceId);
    req.setRawHeader("urgency", urgencyValue(urgency));
    auto reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, nam, this]() {
        reply->deleteLater();
        nam->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return; // we triggered this ourselves
        }

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(Log) << reply->errorString();
            Q_EMIT disconnected(TransientNetworkError, reply->errorString());
        } else {
            qCDebug(Log) << "GET finished";
            Q_EMIT disconnected(NoError);
        }
    });
    m_sseStream.read(reply);
    m_sseReply = reply;
    setUrgency(urgency);
}

QNetworkRequest NextPushProvider::prepareRequest(const char *restCmd, const QString &restArg) const
{
    QUrl url = m_url;
    auto path = url.path();
    path += QLatin1String("/index.php/apps/uppush/") + QLatin1String(restCmd) + QLatin1Char('/') + restArg;
    url.setPath(path);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Basic " + QByteArray(m_userName.toUtf8() + ':' + m_appPassword.toUtf8()).toBase64());
    return req;
}
