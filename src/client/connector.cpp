/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector.h"
#include "connector_p.h"
#include "connector1adaptor.h"
#include "distributor1iface.h"
#include "logging.h"

#include "../shared/unifiedpush-constants.h"

#include <QDBusConnection>
#include <QSettings>

using namespace KUnifiedPush;

ConnectorPrivate::ConnectorPrivate(Connector *qq)
    : QObject(qq)
    , q(qq)
{
    new Connector1Adaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String(UP_CONNECTOR_PATH), this);
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

    if (m_endpoint != actuallyWorkingEndpoint) {
        m_endpoint = actuallyWorkingEndpoint;
        Q_EMIT q->endpointChanged(m_endpoint);
    }
    storeState();
    setState(Connector::Registered);
}

void ConnectorPrivate::Unregistered(const QString &token)
{
    qCDebug(Log) << token;

    // confirmation of our unregistration request
    if (token.isEmpty()) {
        m_token.clear();
        m_endpoint.clear();
        Q_EMIT q->endpointChanged(m_endpoint);
        const auto res = QFile::remove(stateFile());
        qCDebug(Log) << "Removing" << stateFile() << res;
        setState(Connector::Unregistered);
    }

    // we got unregistered by the distributor
    else if (token == m_token) {
        setState(Connector::Unregistered);
        // TODO reregister?
    }
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
    services.erase(std::remove_if(services.begin(), services.end(), [](const auto &s) { return !s.startsWith(QLatin1String(UP_DISTRIBUTOR_SERVICE_NAME_PREFIX)); }), services.end());
    qCDebug(Log) << services;

    if (services.isEmpty()) {
        qCWarning(Log) << "No UnifiedPush distributor found.";
        setState(Connector::NoDistributor);
        return;
    }

    // check if one specific distributor was requested
    const auto requestedDist = QString::fromUtf8(qgetenv("UNIFIEDPUSH_DISTRIBUTOR"));
    if (!requestedDist.isEmpty()) {
        const QString distServiceName = QLatin1String(UP_DISTRIBUTOR_SERVICE_NAME_PREFIX) + requestedDist;
        if (!services.contains(distServiceName)) {
            qCWarning(Log) << "Requested UnifiedPush distributor is not available.";
            setState(Connector::NoDistributor);
        } else {
            setDistributor(distServiceName);
        }
        return;
    }

    // ... otherwise take a random one
    setDistributor(services.at(0));
}

void ConnectorPrivate::setDistributor(const QString &distServiceName)
{
    m_distributor = new OrgUnifiedpushDistributor1Interface(distServiceName, QLatin1String(UP_DISTRIBUTOR_PATH), QDBusConnection::sessionBus(), this);
    qCDebug(Log) << "Selected distributor" << distServiceName << m_distributor->isValid();
}

void ConnectorPrivate::setState(Connector::State state)
{
    qCDebug(Log) << state;
    if (m_state == state) {
        return;
    }

    m_state = state;
    Q_EMIT q->stateChanged(m_state);
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

    if (!d->m_token.isEmpty()) {
        registerClient();
    }
}

Connector::~Connector() = default;

QString Connector::endpoint() const
{
    return d->m_endpoint;
}

void Connector::registerClient()
{
    qCDebug(Log) << d->m_state;
    if (d->m_distributor && d->m_distributor->isValid() && d->m_state == Unregistered) {
        d->setState(Registering);
        if (d->m_token.isEmpty()) {
            d->m_token = QUuid::createUuid().toString();
        }
        qCDebug(Log) << "Registering";
        const auto reply = d->m_distributor->Register(d->m_serviceName, d->m_token/*, QStringLiteral("TODO")*/);
        const auto result = reply.argumentAt(0).toString();
        const auto errorMsg = reply.argumentAt(1).toString();
        qCDebug(Log) << result << errorMsg;
        if (result == QLatin1String("REGISTRATION_SUCCEEDED")) {
            d->setState(d->m_endpoint.isEmpty() ? Registering : Registered);
        } else {
            d->setState(Error);
        }
    }
}

void Connector::unregisterClient()
{
    qCDebug(Log) << d->m_state;
    if (d->m_distributor && d->m_distributor->isValid() && d->m_state == Registered) {
        d->m_distributor->Unregister(d->m_token);
    }
}

Connector::State Connector::state() const
{
    return d->m_state;
}
