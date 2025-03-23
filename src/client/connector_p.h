/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTOR_P_H
#define KUNIFIEDPUSH_CONNECTOR_P_H

#include "connector.h"

#ifndef Q_OS_ANDROID
#include <QDBusServiceWatcher>
#else
#include <QJniObject>
#endif
#include <QObject>

#include <deque>
#include <variant>

class OrgUnifiedpushDistributor1Interface;
class OrgUnifiedpushDistributor2Interface;

class QDBusPendingCall;

namespace KUnifiedPush {
class ConnectorPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ConnectorPrivate(Connector *qq);
    ~ConnectorPrivate();

    // platform-specific implementations
    void init();
    void deinit();
    void doSetDistributor(const QString &distServiceName);
    [[nodiscard]] bool hasDistributor() const;
    void doRegister();
    void doUnregister();

    // UnifiedPush D-Bus interface v1
    void Message(const QString &token, const QByteArray &message, const QString &messageIdentifier);
    void NewEndpoint(const QString &token, const QString &endpoint);
    void Unregistered(const QString &token);

    // UnifiedPush D-Bus interface v2
    [[nodiscard]] QVariantMap Message(const QVariantMap &args);
    [[nodiscard]] QVariantMap NewEndpoint(const QVariantMap &args);
    [[nodiscard]] QVariantMap Unregistered(const QVariantMap &args);

    [[nodiscard]] QString stateFile() const;
    void loadState();
    void storeState() const;
    void selectDistributor();
    void setDistributor(const QString &distServiceName);

    void setState(Connector::State state);

    enum class Command { None, Register, Unregister };
    void addCommand(Command cmd);
    [[nodiscard]] bool isNextCommandReady() const;
    void processNextCommand();

    Connector *q = nullptr;
    QString m_serviceName;
    QString m_token;
    QString m_endpoint;
    QString m_description;
    QString m_vapidPublicKey;
    bool m_vapidRequired = false;
    Connector::State m_state = Connector::Unregistered;

    Command m_currentCommand = Command::None;
    std::deque<Command> m_commandQueue;

#ifndef Q_OS_ANDROID
    void handleRegisterResponse(const QDBusPendingCall &reply);

    std::variant<OrgUnifiedpushDistributor1Interface*, OrgUnifiedpushDistributor2Interface*> m_distributor;
    QDBusServiceWatcher m_serviceWatcher;
#else
    QJniObject m_distributor;
    static std::vector<ConnectorPrivate*> s_instances;
#endif
};
}

#endif
