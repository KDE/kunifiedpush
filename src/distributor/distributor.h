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
#include "../shared/urgency_p.h"

#include <QDBusContext>
#include <QObject>
#include <QTimer>

#include <deque>
#include <memory>
#include <vector>

namespace Solid {
class Device;
}

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

    // UnifiedPush D-Bus interface v1
    [[nodiscard]] QString Register(const QString &serviceName, const QString &token, const QString &description, QString &registrationResultReason);
    void Unregister(const QString &token);

    // UnifiedPush D-Bus interface v2
    [[nodiscard]] QVariantMap Register(const QVariantMap &args);
    [[nodiscard]] QVariantMap Unregister(const QVariantMap &args);

    // KCM D-Bus interface
    [[nodiscard]] int status() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] QString pushProviderId() const;
    [[nodiscard]] QVariantMap pushProviderConfiguration(const QString &pushProviderId) const;
    void setPushProvider(const QString &pushProviderId, const QVariantMap &config);
    [[nodiscard]] QList<KUnifiedPush::ClientInfo> registeredClients() const;
    void forceUnregisterClient(const QString &token);

    // internal
    void messageAcknowledged(const Client &client, const QString &messageIdentifier);

Q_SIGNALS:
    void statusChanged();
    void errorMessageChanged();
    void pushProviderChanged();
    void registeredClientsChanged();

private:
    void messageReceived(const Message &msg);
    void connectOnDemand();
    void clientRegistered(const Client &client, AbstractPushProvider::Error error, const QString &errorMsg);
    void clientUnregistered(const Client &client, AbstractPushProvider::Error error);
    void providerConnected();
    void providerDisconnected(AbstractPushProvider::Error error, const QString &errorMsg);
    void providerMessageAcknowledged(const Client &client, const QString &messageIdentifier);
    void providerUrgencyChanged();
    void retryTimeout();

    [[nodiscard]] QStringList clientTokens() const;

    [[nodiscard]] bool setupPushProvider(bool newSetup = false);
    void purgeUnavailableClients();

    [[nodiscard]] bool hasCurrentCommand() const;
    void processNextCommand();
    void doProcessNextCommand();

    void setStatus(DistributorStatus::Status status);
    void setErrorMessage(const QString &errMsg);

    [[nodiscard]] bool isNetworkAvailable() const;

    // determine current urgency level based on network and batter state as per RFC 8030
    [[nodiscard]] Urgency determineUrgency() const;
    void setUrgency(Urgency urgency);
    void addBattery(const Solid::Device &batteryDevice);

    std::unique_ptr<AbstractPushProvider> m_pushProvider;
    std::vector<Client> m_clients;
    Command m_currentCommand;
    std::deque<Command> m_commandQueue;
    DistributorStatus::Status m_status = DistributorStatus::Unknown;
    Urgency m_urgency = AllUrgencies;
    QString m_errorMessage;
    QTimer m_retryTimer;
};

}

#endif // KUNIFIEDPUSH_DISTRIBUTOR_H
