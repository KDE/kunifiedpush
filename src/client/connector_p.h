/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTOR_P_H
#define KUNIFIEDPUSH_CONNECTOR_P_H

#include "connector.h"

#include <QDBusServiceWatcher>
#include <QObject>

#include <deque>

class OrgUnifiedpushDistributor1Interface;

namespace KUnifiedPush {
class ConnectorPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ConnectorPrivate(Connector *qq);
    ~ConnectorPrivate();

    void Message(const QString &token, const QString &message, const QString &messageIdentifier);
    void NewEndpoint(const QString &token, const QString &endpoint);
    void Unregistered(const QString &token);

    QString stateFile() const;
    void loadState();
    void storeState() const;
    bool hasDistributor() const;
    void selectDistributor();
    void setDistributor(const QString &distServiceName);

    void setState(Connector::State state);

    enum class Command { None, Register, Unregister };
    void addCommand(Command cmd);
    void processNextCommand();

    Connector *q = nullptr;
    QString m_serviceName;
    QString m_token;
    QString m_endpoint;
    QString m_description;
    OrgUnifiedpushDistributor1Interface *m_distributor = nullptr;
    Connector::State m_state = Connector::Unregistered;

    Command m_currentCommand = Command::None;
    std::deque<Command> m_commandQueue;

    QDBusServiceWatcher m_serviceWatcher;
};
}

#endif
