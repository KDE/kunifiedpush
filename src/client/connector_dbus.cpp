/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector_p.h"
#include "connector1adaptor.h"
#include "connector2adaptor.h"
#include "distributor1iface.h"
#include "distributor2iface.h"
#include "introspectiface.h"
#include "logging.h"

#include "../shared/unifiedpush-constants.h"
#include "../shared/connectorutils_p.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>

using namespace Qt::Literals;
using namespace KUnifiedPush;

void ConnectorPrivate::init()
{
    new Connector1Adaptor(this);
    new Connector2Adaptor(this);
    const auto res = QDBusConnection::sessionBus().registerObject(UP_CONNECTOR_PATH, this);
    if (!res) {
        qCWarning(Log) << "Failed to register connector D-Bus adapter!" << UP_CONNECTOR_PATH;
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
        const auto distributorServiceName = std::visit([](auto iface) { return iface->service(); }, m_distributor);
        if (distributorServiceName == serviceName) {
            std::visit([](auto iface) { return delete iface; }, m_distributor);
            m_distributor = {};
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
    // QDBusInterface::isValid does not detect whether the interface actually exists on the remote side,
    // so we have to manually introspect this
    OrgFreedesktopDBusIntrospectableInterface introspectIface(distServiceName, UP_DISTRIBUTOR_PATH, QDBusConnection::sessionBus());
    if (const QString reply = introspectIface.Introspect(); reply.contains(QLatin1StringView(OrgUnifiedpushDistributor2Interface::staticInterfaceName()))) {
        auto iface2 = std::make_unique<OrgUnifiedpushDistributor2Interface>(distServiceName, UP_DISTRIBUTOR_PATH, QDBusConnection::sessionBus(), this);
        if (iface2->isValid()) {
            m_distributor = iface2.release();
            qCDebug(Log) << "Using v2 distributor interface";
            return;
        }
    }
    auto iface1 = std::make_unique<OrgUnifiedpushDistributor1Interface>(distServiceName, UP_DISTRIBUTOR_PATH, QDBusConnection::sessionBus(), this);
    if (iface1->isValid()) {
        m_distributor = iface1.release();
        qCDebug(Log) << "Using v1 distributor interface";
        return;
    }
    qCWarning(Log) << "Invalid distributor D-Bus interface?" << distServiceName;
}

bool ConnectorPrivate::hasDistributor() const
{
    return std::visit([](auto iface) { return iface && iface->isValid(); }, m_distributor);
}

void ConnectorPrivate::doRegister()
{
    std::visit([this](auto iface) {
        using T = std::decay_t<decltype(iface)>;
        if constexpr (std::is_same_v<OrgUnifiedpushDistributor1Interface*, T>) {
            handleRegisterResponse(iface->Register(m_serviceName, m_token, m_description));
        }
        if constexpr (std::is_same_v<OrgUnifiedpushDistributor2Interface*, T>) {
            QVariantMap args;
            args.insert(UP_ARG_SERVICE, m_serviceName);
            args.insert(UP_ARG_TOKEN, m_token);
            args.insert(UP_ARG_DESCRIPTION, m_description);
            handleRegisterResponse(iface->Register(args));
        }
    }, m_distributor);
}

// Generally Qt can do that automatically, but not for the delayed replies
// we get for Register()...
[[nodiscard]] static QVariantMap unpackVariantMap(const QDBusArgument &arg)
{
    QVariantMap m;
    arg.beginMap();
    while (!arg.atEnd()) {
        arg.beginMapEntry();
        QString key;
        QVariant value;
        arg >> key;
        arg >> value;
        m.insert(key, value);
        arg.endMapEntry();
    }
    arg.endMap();
    return m;
}

void ConnectorPrivate::handleRegisterResponse(const QDBusPendingCall &reply)
{
    auto watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            qCWarning(Log) << watcher->error();
            setState(Connector::Error);
        } else {
            QString result, errorMsg;
            std::visit([&result, &errorMsg, watcher](auto iface) {
                using T = std::decay_t<decltype(iface)>;
                if constexpr (std::is_same_v<OrgUnifiedpushDistributor1Interface*, T>) {
                    result = watcher->reply().arguments().at(0).toString();
                    errorMsg = watcher->reply().arguments().at(1).toString();
                }
                if constexpr (std::is_same_v<OrgUnifiedpushDistributor2Interface*, T>) {
                    const auto args = unpackVariantMap(watcher->reply().arguments().at(0).value<QDBusArgument>());
                    result = args.value(UP_ARG_SUCCESS).toString();
                    errorMsg = args.value(UP_ARG_REASON).toString();
                }
            }, m_distributor);
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
    std::visit([this](auto iface) {
        using T = std::decay_t<decltype(iface)>;
        if constexpr (std::is_same_v<OrgUnifiedpushDistributor1Interface*, T>) {
            iface->Unregister(m_token);
        }
        if constexpr (std::is_same_v<OrgUnifiedpushDistributor2Interface*, T>) {
            QVariantMap args;
            args.insert(UP_ARG_TOKEN, m_token);
            iface->Unregister(args);
        }
    }, m_distributor);
}
