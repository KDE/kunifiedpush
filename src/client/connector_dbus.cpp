/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector_p.h"
#include "connector1adaptor.h"
#include "distributor1iface.h"
#include "logging.h"

#include "../shared/unifiedpush-constants.h"
#include "../shared/connectorutils_p.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>

using namespace KUnifiedPush;

void ConnectorPrivate::init()
{
    new Connector1Adaptor(this);
    auto res = QDBusConnection::sessionBus().registerObject(UP_CONNECTOR_PATH, this);
    if (!res) {
        qCWarning(Log) << "Failed to register v1 D-Bus adapter!" << UP_CONNECTOR_PATH;
        // TODO switch to error state?
    }

    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &serviceName) {
        qCDebug(Log) << "Distributor" << serviceName << "became available";
        if (!hasDistributor()) {
            setDistributor(ConnectorUtils::selectDistributor());
            processNextCommand();
        }
    });
    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        qCDebug(Log) << "Distributor" << serviceName << "is gone";
        if (m_distributor->service() == serviceName) {
            delete m_distributor;
            m_distributor = nullptr;
            m_currentCommand = Command::None;
            setState(Connector::NoDistributor);
        }
    });

    m_serviceWatcher.setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    m_serviceWatcher.addWatchedService(UP_DISTRIBUTOR_SERVICE_NAME_FILTER);
}

void ConnectorPrivate::deinit()
{
    QDBusConnection::sessionBus().unregisterObject(UP_CONNECTOR_PATH);
}

void ConnectorPrivate::doSetDistributor(const QString &distServiceName)
{
    m_distributor = new OrgUnifiedpushDistributor1Interface(distServiceName, UP_DISTRIBUTOR_PATH, QDBusConnection::sessionBus(), this);
    if (!m_distributor->isValid()) {
        qCWarning(Log) << "Invalid distributor D-Bus interface?" << distServiceName;
    }
}

bool ConnectorPrivate::hasDistributor() const
{
    return m_distributor && m_distributor->isValid();
}

void ConnectorPrivate::doRegister()
{
    const auto reply = m_distributor->Register(m_serviceName, m_token, m_description);
    auto watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            qCWarning(Log) << watcher->error();
            setState(Connector::Error);
        } else {
            const auto result = watcher->reply().arguments().at(0).toString();
            const auto errorMsg = watcher->reply().arguments().at(1).toString();
            qCDebug(Log) << result << errorMsg;
            if (result == UP_REGISTER_RESULT_SUCCESS) {
                setState(m_endpoint.isEmpty() ? Connector::Registering : Connector::Registered);
            } else {
                setState(Connector::Error);
            }
        }
        m_currentCommand = Command::None;
        processNextCommand();
    });
}

void ConnectorPrivate::doUnregister()
{
    m_distributor->Unregister(m_token);
}
