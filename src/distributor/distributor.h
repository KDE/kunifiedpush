/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_DISTRIBUTOR_H
#define KUNIFIEDPUSH_DISTRIBUTOR_H

#include <QObject>

#include <vector>

namespace KUnifiedPush {

class AbstractPushProvider;
class Client;
class Message;

/** UnifiedPush distributor service. */
class Distributor : public QObject
{
    Q_OBJECT
public:
    explicit Distributor(QObject *parent = nullptr);
    ~Distributor();

    QString Register(const QString &serviceName, const QString &token, QString &registrationResultReason);
    void Unregister(const QString &token);

private:
    void messageReceived(const Message &msg) const;
    void clientRegistered(const Client &client);

    QStringList clientTokens() const;

    AbstractPushProvider *m_pushProvider = nullptr;
    std::vector<Client> m_clients;
};

}

#endif // KUNIFIEDPUSH_DISTRIBUTOR_H
