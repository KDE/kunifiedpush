/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "client.h"
#include "connector1iface.h"
#include "logging.h"

#include "../shared/unifiedpush-constants.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QSettings>

using namespace KUnifiedPush;

Client Client::load(const QString &token, QSettings &settings)
{
    settings.beginGroup(token);
    Client client;
    client.token = token;
    client.remoteId = settings.value(QStringLiteral("RemoteId"), QString()).toString();
    client.serviceName = settings.value(QStringLiteral("ServiceName"), QString()).toString();
    client.endpoint = settings.value(QStringLiteral("Endpoint"), QString()).toString();
    settings.endGroup();
    return client;
}

void Client::store(QSettings& settings) const
{
    settings.beginGroup(token);
    settings.setValue(QStringLiteral("RemoteId"), remoteId);
    settings.setValue(QStringLiteral("ServiceName"), serviceName);
    settings.setValue(QStringLiteral("Endpoint"), endpoint);
    settings.endGroup();
}

void Client::activate() const
{
    qCDebug(Log) << "activating" << serviceName;
    QDBusConnection::sessionBus().interface()->startService(serviceName);
}

OrgUnifiedpushConnector1Interface Client::connector() const
{
    return OrgUnifiedpushConnector1Interface(serviceName, QLatin1String(UP_CONNECTOR_PATH), QDBusConnection::sessionBus());
}
