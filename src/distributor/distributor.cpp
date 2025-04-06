/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "distributor.h"
#include "distributor1adaptor.h"
#include "distributor2adaptor.h"
#include "managementadaptor.h"

#include "autopushprovider.h"
#include "client.h"
#include "gotifypushprovider.h"
#include "logging.h"
#include "message.h"
#include "mockpushprovider.h"
#include "nextpushprovider.h"
#include "ntfypushprovider.h"

#include "../shared/unifiedpush-constants.h"

#include <QDBusConnection>
#include <QSettings>

#include <QNetworkInformation>

using namespace KUnifiedPush;

Distributor::Distributor(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<KUnifiedPush::ClientInfo>();
    qDBusRegisterMetaType<QList<KUnifiedPush::ClientInfo>>();

    // setup network status tracking
    if (QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, &Distributor::processNextCommand);
    } else {
        qCWarning(Log) << "No network state information available!" << QNetworkInformation::availableBackends();
    }

    // register at D-Bus
    new Distributor1Adaptor(this);
    new Distributor2Adaptor(this);
    QDBusConnection::sessionBus().registerObject(UP_DISTRIBUTOR_PATH, this);

    new ManagementAdaptor(this);
    QDBusConnection::sessionBus().registerObject(KDE_DISTRIBUTOR_MANAGEMENT_PATH, this);

    // create and set up push provider
    if (!setupPushProvider()) {
        return;
    }

    // load previous clients
    // TODO what happens to existing clients if the above failed?
    QSettings settings;
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
    if (!m_clients.empty())
    {
        setStatus(DistributorStatus::NoNetwork);
        Command cmd;
        cmd.type = Command::Connect;
        m_commandQueue.push_back(std::move(cmd));
    } else {
        setStatus(DistributorStatus::Idle);
    }

    // purge uninstalled apps
    purgeUnavailableClients();

    processNextCommand();
}

Distributor::~Distributor() = default;

QString Distributor::Register(const QString& serviceName, const QString& token, const QString &description, QString& registrationResultReason)
{
    const auto it = std::ranges::find_if(m_clients, [&token](const auto &client) {
        return client.token == token;
    });
    if (it == m_clients.end()) {
        qCDebug(Log) << "Registering new client (v1)" << serviceName << token;
        connectOnDemand();

        Command cmd;
        cmd.type = Command::Register;
        cmd.client.token = token;
        cmd.client.serviceName = serviceName;
        cmd.client.description = description;
        cmd.client.version = Client::UnifiedPushVersion::v1;
        setDelayedReply(true);
        cmd.reply = message().createReply();
        m_commandQueue.push_back(std::move(cmd));

        processNextCommand();
        return {};
    }

    qCDebug(Log) << "Registering known client (v1)" << serviceName << token;
    (*it).activate();
    (*it).newEndpoint();
    registrationResultReason.clear();
    return UP_REGISTER_RESULT_SUCCESS;
}

QVariantMap Distributor::Register(const QVariantMap &args)
{
    const auto token = args.value(UP_ARG_TOKEN).toString();
    const auto it = std::ranges::find_if(m_clients, [&token](const auto &client) {
        return client.token == token;
    });

    if (it == m_clients.end()) {
        const auto serviceName = args.value(UP_ARG_SERVICE).toString();
        qCDebug(Log) << "Registering new client (v2)" << serviceName << token;
        connectOnDemand();

        Command cmd;
        cmd.type = Command::Register;
        cmd.client.token = token;
        cmd.client.serviceName = serviceName;
        cmd.client.description = args.value(UP_ARG_DESCRIPTION).toString();
        cmd.client.vapidKey = args.value(UP_ARG_VAPID).toString();
        cmd.client.version = Client::UnifiedPushVersion::v2;
        setDelayedReply(true);
        cmd.reply = message().createReply();
        m_commandQueue.push_back(std::move(cmd));

        processNextCommand();
        return {};
    }

    qCDebug(Log) << "Registering known client (v2)" << (*it).serviceName << token;
    (*it).activate();
    (*it).newEndpoint();

    QVariantMap res;
    res.insert(UP_ARG_SUCCESS, QString::fromLatin1(UP_REGISTER_RESULT_SUCCESS));
    return res;
}

void Distributor::connectOnDemand()
{
    // if this is the first client, connect to the push provider first
    // this can involve first-time device registration that is a prerequisite for registering clients
    if (m_clients.empty()) {
        Command cmd;
        cmd.type = Command::Connect;
        m_commandQueue.push_back(std::move(cmd));
    }
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

QVariantMap Distributor::Unregister(const QVariantMap &args)
{
    const auto token = args.value(UP_ARG_TOKEN).toString();
    Unregister(token);
    return {};
}

void Distributor::messageReceived(const Message &msg)
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
    (*it).message(this, msg.content, msg.messageId);
}

void Distributor::clientRegistered(const Client &client, AbstractPushProvider::Error error, const QString &errorMsg)
{
    qCDebug(Log) << client.token << client.remoteId << client.serviceName << error << errorMsg;
    switch (error) {
    case AbstractPushProvider::NoError:
    {
        // TODO check whether we got an endpoint, otherwise report an error
        m_clients.push_back(client);

        QSettings settings;
        client.store(settings);
        settings.setValue(QStringLiteral("Clients/Tokens"), clientTokens());
        Q_EMIT registeredClientsChanged();

        client.newEndpoint();

        if (m_currentCommand.reply.type() != QDBusMessage::InvalidMessage) {
            switch (client.version) {
                case Client::UnifiedPushVersion::v1:
                    m_currentCommand.reply << QString::fromLatin1(UP_REGISTER_RESULT_SUCCESS) << QString();
                    break;
                case Client::UnifiedPushVersion::v2:
                {
                    QVariantMap args;
                    args.insert(UP_ARG_SUCCESS, QString::fromLatin1(UP_REGISTER_RESULT_SUCCESS));
                    m_currentCommand.reply << args;
                    break;
                }
            }
            QDBusConnection::sessionBus().send(m_currentCommand.reply);
        }
        break;
    }
    case AbstractPushProvider::TransientNetworkError:
        // retry
        m_commandQueue.push_front(std::move(m_currentCommand));
        break;
    case AbstractPushProvider::ProviderRejected:
    {
        switch (client.version) {
            case Client::UnifiedPushVersion::v1:
                m_currentCommand.reply << QString::fromLatin1(UP_REGISTER_RESULT_FAILURE) << errorMsg;
                break;
            case Client::UnifiedPushVersion::v2:
            {
                QVariantMap args;
                args.insert(UP_ARG_SUCCESS, QString::fromLatin1(UP_REGISTER_RESULT_FAILURE));
                args.insert(UP_ARG_REASON, QString::fromLatin1(UP_REGISTER_FAILURE_INTERNAL_ERROR));
                // TODO when the client side ever actually uses errorMsg we could return this here in addition
                // in a custom argument
                m_currentCommand.reply << args;
                break;
            }
        }
        QDBusConnection::sessionBus().send(m_currentCommand.reply);
        break;
    }
    }

    m_currentCommand = {};
    processNextCommand();
}

void Distributor::clientUnregistered(const Client &client, AbstractPushProvider::Error error)
{
    qCDebug(Log) << client.token << client.remoteId << client.serviceName << error;
    switch (error) {
    case AbstractPushProvider::NoError:
        client.unregistered(m_currentCommand.type == Command::Unregister);
        [[fallthrough]];
    case AbstractPushProvider::ProviderRejected:
    {
        QSettings settings;
        settings.remove(client.token);
        const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&client](const auto &c) {
            return c.token == client.token;
        });
        if (it != m_clients.end()) {
            m_clients.erase(it);

            // if this was the last client, also disconnect from the push provider
            if (m_clients.empty()) {
                Command cmd;
                cmd.type = Command::Disconnect;
                m_commandQueue.push_back(std::move(cmd));
            }
        }
        settings.setValue(QStringLiteral("Clients/Tokens"), clientTokens());
        Q_EMIT registeredClientsChanged();
        break;
    }
    case AbstractPushProvider::TransientNetworkError:
        // retry
        m_commandQueue.push_front(std::move(m_currentCommand));
        break;
    }

    m_currentCommand = {};
    processNextCommand();
}

void Distributor::providerConnected()
{
    qCDebug(Log);
    setStatus(DistributorStatus::Connected);
    setErrorMessage({});
    m_currentCommand = {};
    processNextCommand();
}

void Distributor::providerDisconnected(AbstractPushProvider::Error error, const QString &errorMsg)
{
    qCDebug(Log) << error << errorMsg;
    if (m_currentCommand.type == Command::Disconnect) {
        m_currentCommand = {};
        setStatus(m_clients.empty() ? DistributorStatus::Idle : DistributorStatus::NoNetwork);
        setErrorMessage({});
    } else {
        setStatus(DistributorStatus::NoNetwork);
        setErrorMessage(errorMsg);
    }
    processNextCommand();
}

void Distributor::providerMessageAcknowledged(const Client &client, const QString &messageIdentifier)
{
    qCDebug(Log) << client.serviceName << messageIdentifier;
    if (m_currentCommand.type == Command::MessageAck && m_currentCommand.value == messageIdentifier) {
        m_currentCommand = {};
    }
    processNextCommand();
}

QStringList Distributor::clientTokens() const
{
    QStringList l;
    l.reserve(m_clients.size());
    std::transform(m_clients.begin(), m_clients.end(), std::back_inserter(l), [](const auto &client) { return client.token; });
    return l;
}

bool Distributor::setupPushProvider()
{
    // determine push provider
    const auto pushProviderName = pushProviderId();
    if (pushProviderName == GotifyPushProvider::Id) {
        m_pushProvider.reset(new GotifyPushProvider);
    } else if (pushProviderName == NextPushProvider::Id) {
        m_pushProvider.reset(new NextPushProvider);
    } else if (pushProviderName == NtfyPushProvider::Id) {
        m_pushProvider.reset(new NtfyPushProvider);
    } else if (pushProviderName == AutopushProvider::Id) {
        m_pushProvider = std::make_unique<AutopushProvider>();
    } else if (pushProviderName == MockPushProvider::Id) {
        m_pushProvider.reset(new MockPushProvider);
    } else {
        qCWarning(Log) << "Unknown push provider:" << pushProviderName;
        m_pushProvider.reset();
        setStatus(DistributorStatus::NoSetup);
        return false;
    }

    QSettings settings;
    settings.beginGroup(pushProviderName);
    if (!m_pushProvider->loadSettings(settings)) {
        qCWarning(Log) << "Invalid push provider settings!";
        setStatus(DistributorStatus::NoSetup);
        return false;
    }
    settings.endGroup();

    connect(m_pushProvider.get(), &AbstractPushProvider::messageReceived, this, &Distributor::messageReceived);
    connect(m_pushProvider.get(), &AbstractPushProvider::clientRegistered, this, &Distributor::clientRegistered);
    connect(m_pushProvider.get(), &AbstractPushProvider::clientUnregistered, this, &Distributor::clientUnregistered);
    connect(m_pushProvider.get(), &AbstractPushProvider::connected, this, &Distributor::providerConnected);
    connect(m_pushProvider.get(), &AbstractPushProvider::disconnected, this, &Distributor::providerDisconnected);
    connect(m_pushProvider.get(), &AbstractPushProvider::messageAcknowledged, this, &Distributor::providerMessageAcknowledged);
    return true;
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

    // in mock mode assume everything is activatable, as unit tests don't install
    // D-Bus service files
    if (m_pushProvider->metaObject() == &MockPushProvider::staticMetaObject) [[unlikely]] {
        return;
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
    if (hasCurrentCommand() || m_commandQueue.empty() || !isNetworkAvailable()) {
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
        case Command::ForceUnregister:
            m_pushProvider->unregisterClient(m_currentCommand.client);
            break;
        case Command::Connect:
            m_pushProvider->connectToProvider();
            break;
        case Command::Disconnect:
            m_pushProvider->disconnectFromProvider();
            break;
        case Command::ChangePushProvider:
        {
            QSettings settings;
            settings.setValue(QLatin1String("PushProvider/Type"), m_currentCommand.value);
            m_currentCommand = {};
            if (setupPushProvider()) {
                processNextCommand();
            }
            break;
        }
        case Command::MessageAck:
            m_pushProvider->acknowledgeMessage(m_currentCommand.client, m_currentCommand.value);
            break;
    }
}

int Distributor::status() const
{
    return m_status;
}

QString Distributor::errorMessage() const
{
    return m_errorMessage;
}

void Distributor::setStatus(DistributorStatus::Status status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    Q_EMIT statusChanged();
}

void Distributor::setErrorMessage(const QString &errMsg)
{
    if (m_errorMessage == errMsg) {
        return;
    }

    m_errorMessage = errMsg;
    Q_EMIT errorMessageChanged();
}

QString Distributor::pushProviderId() const
{
    QSettings settings;
    return settings.value(QStringLiteral("PushProvider/Type"), QString()).toString();
}

QVariantMap Distributor::pushProviderConfiguration(const QString &pushProviderId) const
{
    if (pushProviderId.isEmpty()) {
        return {};
    }

    QSettings settings;
    settings.beginGroup(pushProviderId);
    const auto keys = settings.allKeys();

    QVariantMap config;
    for (const auto &key : keys) {
        const auto v = settings.value(key);
        if (v.isValid()) {
            config.insert(key, settings.value(key));
        }
    }

    return config;
}

void Distributor::setPushProvider(const QString &pushProviderId, const QVariantMap &config)
{
    // store push provider config and check for changes
    bool configChanged = false;
    QSettings settings;
    settings.beginGroup(pushProviderId);
    for (auto it = config.begin(); it != config.end(); ++it) {
        const auto oldValue = settings.value(it.key());
        configChanged |= oldValue != it.value();
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
    if (!configChanged && pushProviderId == this->pushProviderId()) {
        return; // nothing changed
    }

    // if push provider or config changed: unregister all clients, create new push provider backend, re-register all clients
    if (m_status != DistributorStatus::NoSetup) {
        for (const auto &client : m_clients) {
            forceUnregisterClient(client.token);
        }
        if (m_status == DistributorStatus::Connected) {
            Command cmd;
            cmd.type = Command::Disconnect;
            m_commandQueue.push_back(std::move(cmd));
        }
        {
            Command cmd;
            cmd.type = Command::ChangePushProvider;
            cmd.value = pushProviderId;
            m_commandQueue.push_back(std::move(cmd));
        }

        // reconnect if there are clients
        if (!m_clients.empty()) {
            Command cmd;
            cmd.type = Command::Connect;
            m_commandQueue.push_back(std::move(cmd));
        }

        // re-register clients
        for (const auto &client : m_clients) {
            Command cmd;
            cmd.type = Command::Register;
            cmd.client = client;
            m_commandQueue.push_back(std::move(cmd));
        }
    } else {
        // recover from a previously failed attempt to change push providers

        // reconnect if there are clients
        if (!m_commandQueue.empty()) {
            Command cmd;
            cmd.type = Command::Connect;
            m_commandQueue.push_front(std::move(cmd));
        }

        Command cmd;
        cmd.type = Command::ChangePushProvider;
        cmd.value = pushProviderId;
        m_commandQueue.push_front(std::move(cmd));
    }

    processNextCommand();
}

QList<KUnifiedPush::ClientInfo> Distributor::registeredClients() const
{
    QList<KUnifiedPush::ClientInfo> result;
    result.reserve(m_clients.size());

    for (const auto &client : m_clients) {
        ClientInfo info;
        info.token = client.token;
        info.serviceName = client.serviceName;
        info.description = client.description;
        result.push_back(std::move(info));
    }

    return result;
}

void Distributor::forceUnregisterClient(const QString &token)
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
    cmd.type = Command::ForceUnregister;
    cmd.client = (*it);
    m_commandQueue.push_back(std::move(cmd));
    processNextCommand();
}

void Distributor::messageAcknowledged(const Client &client, const QString &messageIdentifier)
{
    if (messageIdentifier.isEmpty()) {
        return;
    }

    qCDebug(Log) << client.serviceName <<messageIdentifier;
    Command cmd;
    cmd.type = Command::MessageAck;
    cmd.client = client;
    cmd.value = messageIdentifier;
    m_commandQueue.push_back(std::move(cmd));
    processNextCommand();
}

bool Distributor::isNetworkAvailable() const
{
    // if in doubt assume we have network and try to connect
    if (QNetworkInformation::instance()) {
        const auto reachability = QNetworkInformation::instance()->reachability();
        return reachability == QNetworkInformation::Reachability::Online || reachability == QNetworkInformation::Reachability::Unknown;
    }
    return true;
}

#include "moc_distributor.cpp"
