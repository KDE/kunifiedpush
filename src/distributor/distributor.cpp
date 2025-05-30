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

#include <Solid/Battery>
#include <Solid/Device>
#include <Solid/DeviceNotifier>

#include <QDBusConnection>
#include <QSettings>

#include <QNetworkInformation>

using namespace KUnifiedPush;

constexpr inline auto RETRY_BACKOFF_FACTOR = 1.5;
constexpr inline auto RETRY_INITIAL_MIN = 15;
constexpr inline auto RETRY_INITIAL_MAX = 40;

Distributor::Distributor(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<KUnifiedPush::ClientInfo>();
    qDBusRegisterMetaType<QList<KUnifiedPush::ClientInfo>>();

    // setup network status tracking
    if (QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, &Distributor::processNextCommand);
        connect(QNetworkInformation::instance(), &QNetworkInformation::isMeteredChanged, this, [this]{ setUrgency(determineUrgency()); });
    } else {
        qCWarning(Log) << "No network state information available!" << QNetworkInformation::availableBackends();
    }

    // setup battery monitoring
    for (const auto &batteryDevice : Solid::Device::listFromType(Solid::DeviceInterface::Battery)) {
        addBattery(batteryDevice);
    }
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, [this](const auto &id) {
        addBattery(Solid::Device(id));
    });

    m_urgency = determineUrgency();

    // exponential backoff retry timer
    m_retryTimer.setTimerType(Qt::VeryCoarseTimer);
    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(0);
    connect(&m_retryTimer, &QTimer::timeout, this, &Distributor::retryTimeout);

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
        // might still be in the queue and never completed registration
        const auto it = std::ranges::find_if(m_commandQueue, [token](const auto &cmd) { return cmd.type == Command::Register && cmd.client.token == token; });
        if (it != m_commandQueue.end()) {
            m_commandQueue.erase(it);
        } else {
            qCWarning(Log) << "Unregistration request for unknown client.";
        }
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
        const auto it = std::ranges::find_if(m_clients, [&client](const auto &c) { return c.token == client.token; });
        if (it == m_clients.end()) {
            m_clients.push_back(client);
        } else {
            (*it) = client;
        }

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
    {
        m_commandQueue.push_front(std::move(m_currentCommand));
        Command cmd;
        cmd.type = Command::Wait;
        m_commandQueue.push_front(std::move(cmd));
        break;
    }
    case AbstractPushProvider::ProviderRejected:
    case AbstractPushProvider::ActionRequired:
    {
        switch (client.version) {
            case Client::UnifiedPushVersion::v1:
                m_currentCommand.reply << QString::fromLatin1(UP_REGISTER_RESULT_FAILURE) << errorMsg;
                break;
            case Client::UnifiedPushVersion::v2:
            {
                QVariantMap args;
                args.insert(UP_ARG_SUCCESS, QString::fromLatin1(UP_REGISTER_RESULT_FAILURE));
                args.insert(UP_ARG_REASON, QString::fromLatin1(error == AbstractPushProvider::ActionRequired ? UP_REGISTER_FAILURE_ACTION_REQUIRED : UP_REGISTER_FAILURE_INTERNAL_ERROR));
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
        if (m_currentCommand.type != Command::SilentUnregister) {
            client.unregistered(m_currentCommand.type == Command::Unregister);
        }
        [[fallthrough]];
    case AbstractPushProvider::ProviderRejected:
    case AbstractPushProvider::ActionRequired:
        if (m_currentCommand.type != Command::SilentUnregister) {
            QSettings settings;
            settings.remove(client.token);
            const auto it = std::find_if(m_clients.begin(), m_clients.end(), [&client](const auto &c) {
                return c.token == client.token;
            });
            if (it != m_clients.end()) {
                m_clients.erase(it);

                // if this was the last client and nothing else needs to be done, also disconnect from the push provider
                if (m_clients.empty() && m_commandQueue.empty()) {
                    Command cmd;
                    cmd.type = Command::Disconnect;
                    m_commandQueue.push_back(std::move(cmd));
                }
            }
            settings.setValue(QStringLiteral("Clients/Tokens"), clientTokens());
            Q_EMIT registeredClientsChanged();
        } else {
            auto c = client;
            c.endpoint.clear();
            QSettings settings;
            c.store(settings);
            c.newEndpoint();
        }
        break;
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
    if (m_currentCommand.type == Command::Connect) {
        m_currentCommand = {};
        m_retryTimer.setInterval(0); // reset retry backoff timer

        // provider needs separate command to change urgency
        if (m_urgency != m_pushProvider->urgency()) {
            if (std::ranges::none_of(m_commandQueue, [](const auto &cmd) { return cmd.type == Command::ChangeUrgency; })) {
                Command cmd;
                cmd.type = Command::ChangeUrgency;
                m_commandQueue.push_back(std::move(cmd));
            }
        }
    }

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

        // defer what we are doing
        if (hasCurrentCommand() && m_currentCommand.type != Command::Connect) {
            m_commandQueue.push_front(std::move(m_currentCommand));
        }
        m_currentCommand = {};

        // attempt to reconnect when we have active clients and didn't ask for the disconnect
        if (!m_clients.empty()) {
            if (error == AbstractPushProvider::TransientNetworkError) {
                Command cmd;
                cmd.type = Command::Connect;
                m_commandQueue.push_front(std::move(cmd));

                cmd.type = Command::Wait;
                m_commandQueue.push_front(std::move(cmd));
            }
        }
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

void Distributor::providerUrgencyChanged()
{
    qCDebug(Log);
    if (m_currentCommand.type == Command::ChangeUrgency) {
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

bool Distributor::setupPushProvider(bool newSetup)
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
    if (newSetup) {
        m_pushProvider->resetSettings(settings);
    }
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
    connect(m_pushProvider.get(), &AbstractPushProvider::urgencyChanged, this, &Distributor::providerUrgencyChanged);
    setStatus(DistributorStatus::Idle);
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
    // Return to the event loop before processing the next command.
    // This allows calls into here from within processing a network operation
    // to fully complete first before we potentially call into that code again.
    QMetaObject::invokeMethod(this, &Distributor::doProcessNextCommand, Qt::QueuedConnection);
}

void Distributor::doProcessNextCommand()
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
            // abort registration if there is no push server configured
            if (!m_pushProvider || m_status == DistributorStatus::NoSetup) {
                clientRegistered(m_currentCommand.client, AbstractPushProvider::ActionRequired, {});
            } else {
                m_pushProvider->registerClient(m_currentCommand.client);
            }
            break;
        case Command::Unregister:
        case Command::ForceUnregister:
        case Command::SilentUnregister:
            m_pushProvider->unregisterClient(m_currentCommand.client);
            break;
        case Command::Connect:
            if (!m_pushProvider || m_status == DistributorStatus::NoSetup) {
                m_currentCommand = {};
                processNextCommand();
            } else {
                m_pushProvider->connectToProvider(m_urgency);
            }
            break;
        case Command::Disconnect:
            m_pushProvider->disconnectFromProvider();
            break;
        case Command::ChangePushProvider:
        {
            QSettings settings;
            settings.setValue(QLatin1String("PushProvider/Type"), m_currentCommand.value);
            m_currentCommand = {};
            if (setupPushProvider(true /* new setup */)) {
                processNextCommand();
            }
            break;
        }
        case Command::MessageAck:
            m_pushProvider->acknowledgeMessage(m_currentCommand.client, m_currentCommand.value);
            break;
        case Command::ChangeUrgency:
            m_pushProvider->changeUrgency(m_urgency);
            break;
        case Command::Wait:
            if (m_retryTimer.interval() == 0) {
                m_retryTimer.setInterval(std::chrono::seconds(QRandomGenerator::global()->bounded(RETRY_INITIAL_MIN, RETRY_INITIAL_MAX)));
            }
            qCDebug(Log) << "retry backoff time:" << m_retryTimer.interval();
            m_retryTimer.start();
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

    // stop retry timer if active
    if (m_currentCommand.type == Command::Wait) {
        qCDebug(Log) << "stopping retry timer";
        m_currentCommand = {};
        m_retryTimer.stop();
        m_retryTimer.setInterval(0);
    }

    // if push provider or config changed: unregister all clients, create new push provider backend, re-register all clients
    if (m_status != DistributorStatus::NoSetup) {
        for (const auto &client : m_clients) {
            Command cmd;
            cmd.type = Command::SilentUnregister;
            cmd.client = client;
            m_commandQueue.push_back(std::move(cmd));
        }
        // remaining registration commands are for not yet persisted clients, keep those
        std::vector<Command> pendingClients;
        for (auto it = m_commandQueue.begin(); it != m_commandQueue.end();) {
            if ((*it).type == Command::Register) {
                pendingClients.push_back(std::move(*it));
                it = m_commandQueue.erase(it);
            } else {
                ++it;
            }
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
        if (!m_clients.empty() || !pendingClients.empty()) {
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
        std::ranges::move(pendingClients, std::back_inserter(m_commandQueue));
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

Urgency Distributor::determineUrgency() const
{
    bool isNotDischarging = false;
    bool foundBattery = false;
    int maxChargePercent = 0;
    for (const auto &batteryDevice : Solid::Device::listFromType(Solid::DeviceInterface::Battery)) {
        const auto battery = batteryDevice.as<Solid::Battery>();
        if (battery->type() != Solid::Battery::PrimaryBattery || !battery->isPresent()) {
            continue;
        }
        if (battery->chargeState() != Solid::Battery::Discharging) {
            isNotDischarging = true;
        }
        maxChargePercent = std::max(battery->chargePercent(), maxChargePercent);
        foundBattery = true;
    }
    const auto isDischarging = foundBattery && !isNotDischarging;
    qCDebug(Log) << isDischarging << foundBattery << isNotDischarging << maxChargePercent;

    if (maxChargePercent > 0 && maxChargePercent <= 15) {
        return Urgency::High;
    }

    const auto isMetered = QNetworkInformation::instance() ? QNetworkInformation::instance()->isMetered() : false;
    qCDebug(Log) << isMetered;
    if (isDischarging && isMetered) {
        return Urgency::Normal;
    }

    if (isDischarging || isMetered) {
        return Urgency::Low;
    }

    return Urgency::VeryLow;
}

void Distributor::setUrgency(Urgency urgency)
{
    if (m_urgency == urgency) {
        return;
    }

    qCDebug(Log) << qToUnderlying(urgency);
    m_urgency = urgency;
    if (std::ranges::any_of(m_commandQueue, [](const auto &cmd) { return cmd.type == Command::ChangeUrgency; })) {
        return;
    }
    Command cmd;
    cmd.type = Command::ChangeUrgency;
    m_commandQueue.push_back(std::move(cmd));
}

void Distributor::addBattery(const Solid::Device &batteryDevice)
{
    const auto battery = batteryDevice.as<Solid::Battery>();
    if (!battery || battery->type() != Solid::Battery::PrimaryBattery) {
        return;
    }

    const auto updateUrgency = [this]() { setUrgency(determineUrgency()); };
    connect(battery, &Solid::Battery::presentStateChanged, this, updateUrgency);
    connect(battery, &Solid::Battery::chargeStateChanged, this, updateUrgency);
    connect(battery, &Solid::Battery::chargePercentChanged, this, updateUrgency);
}

void Distributor::retryTimeout()
{
    if (m_currentCommand.type == Command::Wait) {
        m_currentCommand = {};
        m_retryTimer.setInterval(m_retryTimer.interval() * RETRY_BACKOFF_FACTOR);
    }
    processNextCommand();
}

#include "moc_distributor.cpp"
