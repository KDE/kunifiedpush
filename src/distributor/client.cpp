/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "client.h"
#include "connector1iface.h"
#include "connector2iface.h"
#include "distributor.h"
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
    client.remoteId = settings.value("RemoteId", QString()).toString();
    client.serviceName = settings.value("ServiceName", QString()).toString();
    client.endpoint = settings.value("Endpoint", QString()).toString();
    client.description = settings.value("Description", QString()).toString();
    client.vapidKey = settings.value("VAPIDKey", QString()).toString();
    client.version = static_cast<UnifiedPushVersion>(settings.value("Version", 1).toInt());
    settings.endGroup();
    return client;
}

void Client::store(QSettings& settings) const
{
    settings.beginGroup(token);
    settings.setValue("RemoteId", remoteId);
    settings.setValue("ServiceName", serviceName);
    settings.setValue("Endpoint", endpoint);
    settings.setValue("Description", description);
    settings.setValue("VAPIDKey", vapidKey);
    settings.setValue("Version", qToUnderlying(version));
    settings.endGroup();
}

bool Client::isValid() const
{
    return !token.isEmpty() && !serviceName.isEmpty();
}

void Client::activate() const
{
    qCDebug(Log) << "activating" << serviceName;
    QDBusConnection::sessionBus().interface()->startService(serviceName);
}

void Client::message(Distributor *distributor, const QByteArray &message, const QString &messageIdentifier) const
{
    switch (version) {
        case UnifiedPushVersion::v1:
        {
            OrgUnifiedpushConnector1Interface iface(serviceName, UP_CONNECTOR_PATH, QDBusConnection::sessionBus());
            iface.Message(token, message, messageIdentifier);
            distributor->messageAcknowledged(*this, messageIdentifier);
            break;
        }
        case UnifiedPushVersion::v2:
        {
            OrgUnifiedpushConnector2Interface iface(serviceName, UP_CONNECTOR_PATH, QDBusConnection::sessionBus());
            QVariantMap args;
            args.insert(UP_ARG_TOKEN, token);
            args.insert(UP_ARG_MESSAGE, message);
            if (!messageIdentifier.isEmpty()) {
                args.insert(UP_ARG_MESSAGE_IDENTIFIER, messageIdentifier);
            }
            const auto reply = iface.Message(args);

            auto watcher = new QDBusPendingCallWatcher(reply, distributor);
            QObject::connect(watcher, &QDBusPendingCallWatcher::finished, distributor, [distributor, client = *this](auto *watcher) {
                QDBusPendingReply<QVariantMap> reply = *watcher;
                if (reply.isError()) {
                    qCWarning(Log) << reply.error();
                    // TODO explicit NACK? does any backend support that?
                } else {
                    const auto response = reply.argumentAt<0>();
                    const auto messageId = response.value(UP_ARG_MESSAGE_IDENTIFIER).toString();
                    distributor->messageAcknowledged(client, messageId);
                }
                watcher->deleteLater();
            });

            break;
        }
    }
}

void Client::newEndpoint() const
{
    switch (version) {
        case UnifiedPushVersion::v1:
        {
            OrgUnifiedpushConnector1Interface iface(serviceName, UP_CONNECTOR_PATH, QDBusConnection::sessionBus());
            iface.NewEndpoint(token, endpoint);
            break;
        }
        case UnifiedPushVersion::v2:
        {
            OrgUnifiedpushConnector2Interface iface(serviceName, UP_CONNECTOR_PATH, QDBusConnection::sessionBus());
            QVariantMap args;
            args.insert(UP_ARG_TOKEN, token);
            args.insert(UP_ARG_ENDPOINT, endpoint);
            iface.NewEndpoint(args);
            break;
        }
    }
}

void Client::unregistered(bool isConfirmation) const
{
    switch (version) {
        case UnifiedPushVersion::v1:
        {
            OrgUnifiedpushConnector1Interface iface(serviceName, UP_CONNECTOR_PATH, QDBusConnection::sessionBus());
            iface.Unregistered(isConfirmation ? QString() : token);
            break;
        }
        case UnifiedPushVersion::v2:
        {
            OrgUnifiedpushConnector2Interface iface(serviceName, UP_CONNECTOR_PATH, QDBusConnection::sessionBus());
            QVariantMap args;
            args.insert(UP_ARG_TOKEN, this->token); // v2 sends the token unconditionally
            iface.Unregistered(args);
            break;
        }
    }
}
