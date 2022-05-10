/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "distributor.h"
#include "distributor1adaptor.h"

#include "client.h"
#include "connector1iface.h"
#include "gotifypushprovider.h"
#include "logging.h"
#include "message.h"
#include "nextpushprovider.h"

#include "../shared/unifiedpush-constants.h"

#include <QDBusConnection>
#include <QSettings>

using namespace KUnifiedPush;

Distributor::Distributor(QObject *parent)
    : QObject(parent)
{
    // determine push provider
    QSettings settings;
    const auto pushProviderName = settings.value(QStringLiteral("PushProvider/Type"), QString()).toString();
    if (pushProviderName == QLatin1String("Gotify")) {
        m_pushProvider = new GotifyPushProvider(this);
    } else if (pushProviderName == QLatin1String("NextPush")) {
        m_pushProvider = new NextPushProvider(this);
    } else {
        qCWarning(Log) << "Unknown push provider:" << pushProviderName;
        return;
    }
    settings.beginGroup(pushProviderName);
    m_pushProvider->loadSettings(settings);
    settings.endGroup();
    connect(m_pushProvider, &AbstractPushProvider::messageReceived, this, &Distributor::messageReceived);
    connect(m_pushProvider, &AbstractPushProvider::clientRegistered, this, &Distributor::clientRegistered);
    connect(m_pushProvider, &AbstractPushProvider::clientUnregistered, this, &Distributor::clientUnregistered);

    // load previous clients
    const auto clientTokens = settings.value(QStringLiteral("Clients/Tokens"), QStringList()).toStringList();
    m_clients.reserve(clientTokens.size());
    for (const auto &token : clientTokens) {
        auto client = Client::load(token, settings);
        if (client.isValid()) {
            m_clients.push_back(std::move(client));
        }
    }
    qCDebug(Log) << m_clients.size() << "registered clients loaded";

    // connect to push provider if necessary
    // TODO
    m_pushProvider->connectToProvider();

    // purge uninstalled apps
    purgeUnavailableClients();

    // register at D-Bus
    new Distributor1Adaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String(UP_DISTRIBUTOR_PATH), this);
}

Distributor::~Distributor() = default;

QString Distributor::Register(const QString& serviceName, const QString& token, QString& registrationResultReason)
{
    qCDebug(Log) << serviceName << token;
    const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&token](const auto &client) {
        return client.token == token;
    });
    if (it == m_clients.end()) {
        qCDebug(Log) << "Registering new client";
        Command cmd;
        cmd.type = Command::Register;
        cmd.client.token = token;
        cmd.client.serviceName = serviceName;
        setDelayedReply(true);
        cmd.reply = message().createReply();
        m_commandQueue.push_back(std::move(cmd));
        processNextCommand();
        return {};
    }

    qCDebug(Log) << "Registering known client";
    (*it).activate();
    (*it).connector().NewEndpoint((*it).token, (*it).endpoint);
    registrationResultReason.clear();
    return QLatin1String(UP_REGISTER_RESULT_SUCCESS);
}

void Distributor::Unregister(const QString& token)
{
    qCDebug(Log) << token;
    const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&token](const auto &client) {
        return client.token == token;
    });
    if (it == m_clients.end()) {
        qCWarning(Log) << "Unregistration request for unknown client.";
        return;
    }

    Command cmd;
    cmd.type = Command::Unregister;
    cmd.client = (*it);
    m_commandQueue.push_back(std::move(cmd));
    processNextCommand();
}

void Distributor::messageReceived(const Message &msg) const
{
    qCDebug(Log) << msg.clientRemoteId << msg.content;
    const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&msg](const auto &client) {
        return client.remoteId == msg.clientRemoteId;
    });
    if (it == m_clients.end()) {
        qCWarning(Log) << "Received message for unknown client";
        return;
    }

    (*it).activate();
    (*it).connector().Message((*it).token, msg.content, {});
}

void Distributor::clientRegistered(const Client &client)
{
    qCDebug(Log) << client.token << client.remoteId << client.serviceName;
    // TODO check whether we got an endpoint, otherwise report an error
    m_clients.push_back(client);

    QSettings settings;
    client.store(settings);
    settings.setValue(QStringLiteral("Clients/Tokens"), clientTokens());

    client.connector().NewEndpoint(client.token, client.endpoint);

    m_currentCommand.reply << QString::fromLatin1(UP_REGISTER_RESULT_SUCCESS) << QString();
    QDBusConnection::sessionBus().send(m_currentCommand.reply);
    m_currentCommand = {};
    processNextCommand();
}

void Distributor::clientUnregistered(const Client &client)
{
    qCDebug(Log) << client.token << client.remoteId << client.serviceName;
    client.connector().Unregistered(QString());

    QSettings settings;
    settings.remove(client.token);
    const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&client](const auto &c) {
        return c.token == client.token;
    });
    if (it != m_clients.end()) {
        m_clients.erase(it);
    }
    settings.setValue(QStringLiteral("Clients/Tokens"), clientTokens());

    m_currentCommand = {};
    processNextCommand();
}

QStringList Distributor::clientTokens() const
{
    QStringList l;
    l.reserve(m_clients.size());
    std::transform(m_clients.begin(), m_clients.end(), std::back_inserter(l), [](const auto &client) { return client.token; });
    return l;
}

void Distributor::purgeUnavailableClients()
{
    QStringList activatableServiceNames = QDBusConnection::sessionBus().interface()->activatableServiceNames();
    std::sort(activatableServiceNames.begin(), activatableServiceNames.end());

    // collect clients to unregister first, so m_clients doesn't change underneath us
    QStringList tokensToUnregister;
    for (const auto &client : m_clients) {
        if (!std::binary_search(activatableServiceNames.begin(), activatableServiceNames.end(), client.serviceName)) {
            tokensToUnregister.push_back(client.token);
        }
    }

    for (const auto &token : tokensToUnregister) {
        Unregister(token);
    }
}

bool Distributor::hasCurrentCommand() const
{
    return m_currentCommand.type != Command::NoCommand;
}

void Distributor::processNextCommand()
{
    if (hasCurrentCommand() || m_commandQueue.empty()) {
        return;
    }

    m_currentCommand = m_commandQueue.front();
    m_commandQueue.pop_front();
    switch (m_currentCommand.type) {
        case Command::NoCommand:
            Q_ASSERT(false);
            processNextCommand();
            break;
        case Command::Register:
            m_pushProvider->registerClient(m_currentCommand.client);
            break;
        case Command::Unregister:
            m_pushProvider->unregisterClient(m_currentCommand.client);
            break;
    }
}
