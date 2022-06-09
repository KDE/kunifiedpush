/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "mockpushprovider.h"
#include "client.h"
#include "logging.h"

using namespace KUnifiedPush;

MockPushProvider::MockPushProvider(QObject *parent)
    : AbstractPushProvider(parent)
{
    qRegisterMetaType<KUnifiedPush::Client>();
}

MockPushProvider::~MockPushProvider() = default;

void MockPushProvider::loadSettings(const QSettings &settings)
{
    Q_UNUSED(settings);
}

void MockPushProvider::connectToProvider()
{
    qCDebug(Log);
}

void MockPushProvider::registerClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token;

    auto newClient = client;
    newClient.remoteId = QStringLiteral("<client-remote-id>");
    newClient.endpoint = QStringLiteral("https://localhost/push-endpoint");
    QMetaObject::invokeMethod(this, "clientRegistered", Qt::QueuedConnection, Q_ARG(KUnifiedPush::Client, newClient));
}

void MockPushProvider::unregisterClient(const Client &client)
{
    qCDebug(Log) << client.serviceName << client.token;
}
