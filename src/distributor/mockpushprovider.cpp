/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "mockpushprovider.h"
#include "client.h"
#include "logging.h"

using namespace KUnifiedPush;

MockPushProvider* MockPushProvider::s_instance = nullptr;

MockPushProvider::MockPushProvider(QObject *parent)
    : AbstractPushProvider(Id, parent)
{
    s_instance = this;

    qRegisterMetaType<KUnifiedPush::Client>();
    qRegisterMetaType<KUnifiedPush::AbstractPushProvider::Error>();
}

MockPushProvider::~MockPushProvider()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool MockPushProvider::loadSettings(const QSettings &settings)
{
    Q_UNUSED(settings);
    return true;
}

void MockPushProvider::connectToProvider(Urgency urgency)
{
    qCDebug(Log) << qToUnderlying(urgency);
    QMetaObject::invokeMethod(this, "connected", Qt::QueuedConnection);
}

void MockPushProvider::disconnectFromProvider()
{
    qCDebug(Log);
    QMetaObject::invokeMethod(this, "disconnected", Qt::QueuedConnection, Q_ARG(KUnifiedPush::AbstractPushProvider::Error, NoError));
}

void MockPushProvider::registerClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token << client.vapidKey;

    auto newClient = client;
    newClient.remoteId = QStringLiteral("<client-remote-id>");
    newClient.endpoint = QStringLiteral("https://localhost/push-endpoint");
    QMetaObject::invokeMethod(this, "clientRegistered", Qt::QueuedConnection, Q_ARG(KUnifiedPush::Client, newClient));
}

void MockPushProvider::unregisterClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token;
    QMetaObject::invokeMethod(this, "clientUnregistered", Qt::QueuedConnection, Q_ARG(KUnifiedPush::Client, client));
}

void MockPushProvider::acknowledgeMessage(const Client &client, const QString &messageIdentifier)
{
    qCDebug(Log) << client.serviceName <<messageIdentifier;
    QMetaObject::invokeMethod(this, "messageAcknowledged", Qt::QueuedConnection, Q_ARG(KUnifiedPush::Client, client), Q_ARG(QString, messageIdentifier));
}

void MockPushProvider::doChangeUrgency(Urgency urgency)
{
    qCDebug(Log) << qToUnderlying(urgency);
    AbstractPushProvider::doChangeUrgency(urgency);
}
