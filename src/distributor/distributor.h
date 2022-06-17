/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_DISTRIBUTOR_H
#define KUNIFIEDPUSH_DISTRIBUTOR_H

#include "abstractpushprovider.h"
#include "command.h"

#include "../shared/clientinfo_p.h"
#include "../shared/distributorstatus_p.h"

#include <QDBusContext>
#include <QObject>

#include <deque>
#include <memory>
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

    // UnifiedPush D-Bus interface
    QString Register(const QString &serviceName, const QString &token, QString &registrationResultReason);
    void Unregister(const QString &token);

    // KCM D-Bus interface
    int status() const;
    QString pushProviderId() const;
    QVariantMap pushProviderConfiguration(const QString &pushProviderId) const;
    void setPushProvider(const QString &pushProviderId, const QVariantMap &config);
    QList<KUnifiedPush::ClientInfo> registeredClients() const;
    void forceUnregisterClient(const QString &token);

Q_SIGNALS:
    void statusChanged();
    void pushProviderChanged();
    void registeredClientsChanged();

private:
    void messageReceived(const Message &msg) const;
    void clientRegistered(const Client &client, AbstractPushProvider::Error error, const QString &errorMsg);
    void clientUnregistered(const Client &client, AbstractPushProvider::Error error);
    void providerConnected();
    void providerDisconnected(AbstractPushProvider::Error error, const QString &errorMsg);

    QStringList clientTokens() const;

    bool setupPushProvider();
    void purgeUnavailableClients();

    bool hasCurrentCommand() const;
    void processNextCommand();

    void setStatus(DistributorStatus::Status status);

    std::unique_ptr<AbstractPushProvider> m_pushProvider;
    std::vector<Client> m_clients;
    Command m_currentCommand;
    std::deque<Command> m_commandQueue;
    DistributorStatus::Status m_status = DistributorStatus::Unknown;
};

}

#endif // KUNIFIEDPUSH_DISTRIBUTOR_H
