/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "distributor.h"
#include "distributor1adaptor.h"

#include "client.h"
#include "connector1iface.h"
#include "gotifypushprovider.h"
#include "logging.h"
#include "message.h"

#include <QDBusConnection>
#include <QSettings>

using namespace KUnifiedPush;

Distributor::Distributor(QObject *parent)
    : QObject(parent)
{
    // determine push provider
    QSettings settings;
    const auto pushProviderName = settings.value(QStringLiteral("PushProvider/Type"), QStringLiteral("Gotify")).toString();
    if (pushProviderName == QLatin1String("Gotify")) {
        m_pushProvider = new GotifyPushProvider(this);
    } else {
        qCWarning(Log) << "Unknown push provider:" << pushProviderName;
        return;
    }
    settings.beginGroup(pushProviderName);
    m_pushProvider->loadSettings(settings);
    settings.endGroup();
    connect(m_pushProvider, &AbstractPushProvider::messageReceived, this, &Distributor::messageReceived);

    // load previous clients
    const auto clientTokens = settings.value(QStringLiteral("Clients/Tokens"), QStringList()).toStringList();
    m_clients.reserve(clientTokens.size());
    for (const auto &token : clientTokens) {
        settings.beginGroup(token);
        Client client;
        client.token = token;
        client.remoteId = settings.value(QStringLiteral("RemoteId"), QString()).toString();
        client.serviceName = settings.value(QStringLiteral("ServiceName"), QString()).toString();
        client.endpoint = settings.value(QStringLiteral("Endpoint"), QString()).toString();
        settings.endGroup();

        // TODO discard invalid
        m_clients.push_back(std::move(client));
    }
    qCDebug(Log) << m_clients.size() << "registered clients loaded";

    // connect to push provider if necessary
    // TODO
    m_pushProvider->connectToProvider();

    // purge uninstalled apps
    // TODO

    // register at D-Bus
    new Distributor1Adaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/unifiedpush/Distributor"), this);
}

Distributor::~Distributor() = default;

QString Distributor::Register(const QString& serviceName, const QString& token, QString& registrationResultReason)
{
    qCDebug(Log) << serviceName << token << "XXXXX";
    const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&token](const auto &client) {
        return client.token == token;
    });
    if (it == m_clients.end()) {
        qCDebug(Log) << "Registering new client";
        registrationResultReason = QStringLiteral("not implemented yet");
        return QStringLiteral("REGISTRATION_FAILED");
    }

    qCDebug(Log) << "Registering known client";
    QDBusConnection::sessionBus().interface()->startService((*it).serviceName);
    OrgUnifiedpushConnector1Interface iface((*it).serviceName, QStringLiteral("/org/unifiedpush/Connector"), QDBusConnection::sessionBus());
    qCDebug(Log) << (*it).serviceName << iface.isValid();
    iface.NewEndpoint((*it).token, (*it).endpoint);
    return QStringLiteral("REGISTRATION_SUCCEEDED");
}

void Distributor::Unregister(const QString& token)
{
    qCDebug(Log) << token;
}

void Distributor::messageReceived(const Message &msg) const
{
    qCDebug(Log) << msg.clientRemoteId << msg.content;
    const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&msg](const auto &client) {
        return client.remoteId == msg.clientRemoteId;
    });
    if (it == m_clients.end()) {
        qCWarning(Log) << "Received message for unknown client";
        return;
    }

    QDBusConnection::sessionBus().interface()->startService((*it).serviceName);
    OrgUnifiedpushConnector1Interface iface((*it).serviceName, QStringLiteral("/org/unifiedpush/Connector"), QDBusConnection::sessionBus());
    qCDebug(Log) << (*it).serviceName << iface.isValid();
    iface.Message((*it).token, msg.content, {});
}
