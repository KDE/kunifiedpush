/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector.h"
#include "connector_p.h"
#include "connector1adaptor.h"
#include "distributor1iface.h"
#include "logging.h"

#include <QDBusConnection>

using namespace KUnifiedPush;

ConnectorPrivate::ConnectorPrivate(Connector *qq)
    : QObject(qq)
    , q(qq)
{
    new Connector1Adaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/unifiedpush/Connector"), this);
}

void ConnectorPrivate::Message(const QString &token, const QString &message, const QString &messageIdentifier)
{
    qCDebug(Log) << token << message << messageIdentifier;
    Q_EMIT q->messageReceived(message);
}

void ConnectorPrivate::NewEndpoint(const QString &token, const QString &endpoint)
{
    // ### sigh...
    QString actuallyWorkingEndpoint(endpoint);
    actuallyWorkingEndpoint.replace(QLatin1String("/UP?"), QLatin1String("/message?"));
    qCDebug(Log) << token << actuallyWorkingEndpoint;
}

void ConnectorPrivate::Unregister(const QString &token)
{
    qCDebug(Log) << token;
}

void ConnectorPrivate::selectDistributor()
{
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    services.erase(std::remove_if(services.begin(), services.end(), [](const auto &s) { return !s.startsWith(QLatin1String("org.unifiedpush.Distributor.")); }), services.end());
    qCDebug(Log) << services;

    if (services.isEmpty()) {
        qCWarning(Log) << "No UnifiedPush distributor found.";
        return;
    }

    // TODO customizable selection if there is more than one

    m_distributor = new OrgUnifiedpushDistributor1Interface(services.at(0), QStringLiteral("/org/unifiedpush/Distributor"), QDBusConnection::sessionBus(), this);
    qCDebug(Log) << "Selected distributor" << services.at(0) << m_distributor->isValid();
}


Connector::Connector(QObject *parent)
    : QObject(parent)
    , d(new ConnectorPrivate(this))
{
    d->m_token = QStringLiteral("191059e1-10b9-4cfb-a878-a031de0fcd38"); // TODO
    d->selectDistributor();

    if (d->m_distributor && d->m_distributor->isValid()) {
        // TODO get service name default from QCoreApp
        qCDebug(Log) << "Registering";
        const auto reply = d->m_distributor->Register(QStringLiteral("org.kde.kunifiedpush.demo-notifier"), d->m_token/*, QStringLiteral("TODO")*/);
        qCDebug(Log) << reply;
    }

}

Connector::~Connector() = default;
