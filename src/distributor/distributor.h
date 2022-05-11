/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_DISTRIBUTOR_H
#define KUNIFIEDPUSH_DISTRIBUTOR_H

#include "abstractpushprovider.h"
#include "command.h"

#include <QDBusContext>
#include <QObject>

#include <deque>
#include <vector>

namespace KUnifiedPush {

class Client;
class Message;

/** UnifiedPush distributor service. */
class Distributor : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit Distributor(QObject *parent = nullptr);
    ~Distributor();

    QString Register(const QString &serviceName, const QString &token, QString &registrationResultReason);
    void Unregister(const QString &token);

private:
    void messageReceived(const Message &msg) const;
    void clientRegistered(const Client &client, AbstractPushProvider::Error error, const QString &errorMsg);
    void clientUnregistered(const Client &client, AbstractPushProvider::Error error);

    QStringList clientTokens() const;

    void purgeUnavailableClients();

    bool hasCurrentCommand() const;
    void processNextCommand();

    AbstractPushProvider *m_pushProvider = nullptr;
    std::vector<Client> m_clients;
    Command m_currentCommand;
    std::deque<Command> m_commandQueue;
};

}

#endif // KUNIFIEDPUSH_DISTRIBUTOR_H
