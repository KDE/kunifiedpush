/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector.h"
#include "connector_p.h"
#include "logging.h"

#include "../shared/connectorutils_p.h"

#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

using namespace KUnifiedPush;

ConnectorPrivate::ConnectorPrivate(Connector *qq)
    : QObject(qq)
    , q(qq)
{
    init();
}

ConnectorPrivate::~ConnectorPrivate()
{
    deinit();
}

void ConnectorPrivate::Message(const QString &token, const QByteArray &message, const QString &messageIdentifier)
{
    qCDebug(Log) << token << message << messageIdentifier;
    if (token != m_token) {
        qCWarning(Log) << "Got message for a different token??";
        return;
    }
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
        m_endpoint.clear();
        Q_EMIT q->endpointChanged(m_endpoint);
        setState(Connector::Unregistered);
        storeState();
    }

    if (m_currentCommand == Command::Unregister) {
        m_currentCommand = Command::None;
    }
    processNextCommand();
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
    m_description = settings.value(QStringLiteral("Description"), QString()).toString();
}

void ConnectorPrivate::storeState() const
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Client"));
    settings.setValue(QStringLiteral("Token"), m_token);
    settings.setValue(QStringLiteral("Endpoint"), m_endpoint);
    settings.setValue(QStringLiteral("Description"), m_description);
}

void ConnectorPrivate::setDistributor(const QString &distServiceName)
{
    if (distServiceName.isEmpty()) {
        qCWarning(Log) << "No UnifiedPush distributor found.";
        setState(Connector::NoDistributor);
        return;
    }

    doSetDistributor(distServiceName);
    qCDebug(Log) << "Selected distributor" << distServiceName;
    setState(Connector::Unregistered);

    if (!m_token.isEmpty()) { // re-register if we have been registered before
        q->registerClient(m_description);
    }
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

void ConnectorPrivate::addCommand(ConnectorPrivate::Command cmd)
{
    // ignore new commands that are already in the queue or cancel each other out
    if (!m_commandQueue.empty()) {
        if (m_commandQueue.back() == cmd) {
            return;
        }
        if ((m_commandQueue.back() == Command::Register && cmd == Command::Unregister) || (m_commandQueue.back() == Command::Unregister && cmd == Command::Register)) {
            m_commandQueue.pop_back();
            return;
        }
    } else if (m_currentCommand == cmd) {
        return;
    }

    m_commandQueue.push_back(cmd);
    processNextCommand();
}

void ConnectorPrivate::processNextCommand()
{
    if (m_currentCommand != Command::None || !hasDistributor() || m_commandQueue.empty()) {
        return;
    }

    m_currentCommand = m_commandQueue.front();
    m_commandQueue.pop_front();

    switch (m_currentCommand) {
        case Command::None:
            break;
        case Command::Register:
        {
            if (m_state == Connector::Registered) {
                m_currentCommand = Command::None;
                break;
            }
            setState(Connector::Registering);
            if (m_token.isEmpty()) {
                m_token = QUuid::createUuid().toString();
            }
            qCDebug(Log) << "Registering";
            doRegister();
            break;
        }
        case Command::Unregister:
            if (m_state == Connector::Unregistered) {
                m_currentCommand = Command::None;
                break;
            }
            qCDebug(Log) << "Unregistering";
            doUnregister();
            break;
    }

    processNextCommand();
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
    d->setDistributor(ConnectorUtils::selectDistributor());
}

Connector::~Connector() = default;

QString Connector::endpoint() const
{
    return d->m_endpoint;
}

void Connector::registerClient(const QString &description)
{
    qCDebug(Log) << d->m_state;
    d->m_description = description;
    d->addCommand(ConnectorPrivate::Command::Register);
}

void Connector::unregisterClient()
{
    qCDebug(Log) << d->m_state;
    d->addCommand(ConnectorPrivate::Command::Unregister);
}

Connector::State Connector::state() const
{
    return d->m_state;
}

#include "moc_connector.cpp"

#include "moc_connector_p.cpp"
