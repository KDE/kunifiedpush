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
#include <QSettings>

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
    // TODO
    Q_EMIT q->messageReceived(message);
}

void ConnectorPrivate::NewEndpoint(const QString &token, const QString &endpoint)
{
    qCDebug(Log) << token << endpoint;
    if (token != m_token) {
        qCWarning(Log) << "Got new endpoint for a different token??";
        return;
    }

    // ### Gotify workaround...
    QString actuallyWorkingEndpoint(endpoint);
    actuallyWorkingEndpoint.replace(QLatin1String("/UP?"), QLatin1String("/message?"));

    if (m_endpoint == actuallyWorkingEndpoint) {
        return;
    }
    m_endpoint = actuallyWorkingEndpoint;
    storeState();
    Q_EMIT q->endpointChanged(m_endpoint);
}

void ConnectorPrivate::Unregistered(const QString &token)
{
    // TODO token == m_token - we got unregistered by the distributor
    // TODO token.isEmpty() - confirmation of our unregistration request
    qCDebug(Log) << token;
}

QString ConnectorPrivate::stateFile() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-") + m_serviceName;
}

void ConnectorPrivate::loadState()
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Client"));
    m_token = settings.value(QStringLiteral("Token"), QString()).toString();
    m_endpoint = settings.value(QStringLiteral("Endpoint"), QString()).toString();
}

void ConnectorPrivate::storeState() const
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Client"));
    settings.setValue(QStringLiteral("Token"), m_token);
    settings.setValue(QStringLiteral("Endpoint"), m_endpoint);
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


Connector::Connector(const QString &serviceName, QObject *parent)
    : QObject(parent)
    , d(new ConnectorPrivate(this))
{
    d->m_serviceName = serviceName;
    if (d->m_serviceName.isEmpty()) {
        qCWarning(Log) << "empty D-Bus service name!";
        return;
    }

    d->loadState();
    d->selectDistributor();

    if (d->m_distributor && d->m_distributor->isValid()) {
        if (d->m_token.isEmpty()) {
            d->m_token = QUuid::createUuid().toString();
        }
        qCDebug(Log) << "Registering";
        const auto reply = d->m_distributor->Register(d->m_serviceName, d->m_token/*, QStringLiteral("TODO")*/);
        qCDebug(Log) << reply;
    }

}

Connector::~Connector() = default;

QString Connector::endpoint() const
{
    return d->m_endpoint;
}
