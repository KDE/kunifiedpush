/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector.h"
#include "connector_p.h"
#include "logging.h"

#include "../shared/connectorutils_p.h"
#include "../shared/unifiedpush-constants.h"

#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

using namespace KUnifiedPush;

ConnectorPrivate::ConnectorPrivate(Connector *qq)
    : QObject(qq)
    , q(qq)
{
    init();
}

ConnectorPrivate::~ConnectorPrivate()
{
    deinit();
}

void ConnectorPrivate::Message(const QString &token, const QByteArray &message, const QString &messageIdentifier)
{
    qCDebug(Log) << token << message << messageIdentifier << m_contentEnc.hasKeys();
    if (token != m_token) {
        qCWarning(Log) << "Got message for a different token??";
        return;
    }
    if (m_contentEnc.hasKeys()) {
        const auto decrypted = m_contentEnc.decrypt(message);
        qCDebug(Log) << token << decrypted;
        if (!decrypted.isEmpty()) {
            Q_EMIT q->messageReceived(decrypted);
            return;
        }
    }

    Q_EMIT q->messageReceived(message);
}

QVariantMap ConnectorPrivate::Message(const QVariantMap &args)
{
    const auto token = args.value(UP_ARG_TOKEN).toString();
    const auto message = args.value(UP_ARG_MESSAGE).toByteArray();
    const auto id = args.value(UP_ARG_MESSAGE_IDENTIFIER).toString();
    Message(token, message, id);

    QVariantMap r;
    if (!id.isEmpty()) {
        r.insert(UP_ARG_MESSAGE_IDENTIFIER, id);
    }
    return r;
}

void ConnectorPrivate::NewEndpoint(const QString &token, const QString &endpoint)
{
    qCDebug(Log) << token << endpoint;
    if (token != m_token) {
        qCWarning(Log) << "Got new endpoint for a different token??";
        return;
    }

    // ### Gotify workaround...
    QString actuallyWorkingEndpoint(endpoint);
    actuallyWorkingEndpoint.replace(QLatin1String("/UP?"), QLatin1String("/message?"));

    if (m_endpoint != actuallyWorkingEndpoint) {
        m_endpoint = actuallyWorkingEndpoint;
        Q_EMIT q->endpointChanged(m_endpoint);
    }
    storeState();
    setState(Connector::Registered);
}

QVariantMap ConnectorPrivate::NewEndpoint(const QVariantMap &args)
{
    const auto token = args.value(UP_ARG_TOKEN).toString();
    const auto endpoint = args.value(UP_ARG_ENDPOINT).toString();
    NewEndpoint(token, endpoint);
    return {};
}

void ConnectorPrivate::Unregistered(const QString &token)
{
    qCDebug(Log) << token;

    // confirmation of our unregistration request
    if (token.isEmpty() || (token == m_token && m_currentCommand == Command::Unregister)) {
        m_token.clear();
        m_endpoint.clear();
        Q_EMIT q->endpointChanged(m_endpoint);
        const auto res = QFile::remove(stateFile());
        qCDebug(Log) << "Removing" << stateFile() << res;
        setState(Connector::Unregistered);
    }

    // we got unregistered by the distributor
    else if (token == m_token) {
        m_endpoint.clear();
        Q_EMIT q->endpointChanged(m_endpoint);
        setState(Connector::Unregistered);
        storeState();
    }

    if (m_currentCommand == Command::Unregister) {
        m_currentCommand = Command::None;
    }
    processNextCommand();
}

QVariantMap ConnectorPrivate::Unregistered(const QVariantMap &args)
{
    const auto token = args.value(UP_ARG_TOKEN).toString();
    Unregistered(token);
    return {};
}

QString ConnectorPrivate::stateFile() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-") + m_serviceName;
}

void ConnectorPrivate::loadState()
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    settings.beginGroup("Client");
    m_token = settings.value("Token").toString();
    m_endpoint = settings.value("Endpoint").toString();
    m_description = settings.value("Description").toString();
    m_vapidRequired = settings.value("VapidRequired", false).toBool();
    m_vapidPublicKey = settings.value("VapidPublicKey").toString();

    const auto pubKey = settings.value("ContentEncryptionPublicKey").toByteArray();
    const auto privKey = settings.value("ContentEncryptionPrivateKey").toByteArray();
    const auto authSec = settings.value("contentEncryptionAuthSecret").toByteArray();
    if (!pubKey.isEmpty() && !privKey.isEmpty() && !authSec.isEmpty()) {
        m_contentEnc = ContentEncryption(pubKey, privKey, authSec);
    }
}

void ConnectorPrivate::storeState() const
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    settings.beginGroup("Client");
    settings.setValue("Token", m_token);
    settings.setValue("Endpoint", m_endpoint);
    settings.setValue("Description", m_description);
    settings.setValue("VapidRequired", m_vapidRequired);
    settings.setValue("VapidPublicKey", m_vapidPublicKey);

    if (m_contentEnc.hasKeys()) {
        settings.setValue("ContentEncryptionPublicKey", m_contentEnc.publicKey());
        settings.setValue("ContentEncryptionPrivateKey", m_contentEnc.privateKey());
        settings.setValue("contentEncryptionAuthSecret", m_contentEnc.authSecret());
    }
}

void ConnectorPrivate::setDistributor(const QString &distServiceName)
{
    if (distServiceName.isEmpty()) {
        qCWarning(Log) << "No UnifiedPush distributor found.";
        setState(Connector::NoDistributor);
        return;
    }

    doSetDistributor(distServiceName);
    qCDebug(Log) << "Selected distributor" << distServiceName;
    setState(Connector::Unregistered);

    if (!m_token.isEmpty()) { // re-register if we have been registered before
        q->registerClient(m_description);
    }
}

void ConnectorPrivate::registrationFailed(const QString &token, const QString &reason)
{
    qCDebug(Log) << token << reason;
    if (token != m_token) {
        return;
    }

    // TODO error code/error message
    setState(Connector::Error);
}

void ConnectorPrivate::setState(Connector::State state)
{
    qCDebug(Log) << state;
    if (m_state == state) {
        return;
    }

    m_state = state;
    Q_EMIT q->stateChanged(m_state);
}

void ConnectorPrivate::addCommand(ConnectorPrivate::Command cmd)
{
    // ignore new commands that are already in the queue or cancel each other out
    if (!m_commandQueue.empty()) {
        if (m_commandQueue.back() == cmd) {
            return;
        }
        if ((m_commandQueue.back() == Command::Register && cmd == Command::Unregister) || (m_commandQueue.back() == Command::Unregister && cmd == Command::Register)) {
            m_commandQueue.pop_back();
            return;
        }
    } else if (m_currentCommand == cmd) {
        return;
    }

    m_commandQueue.push_back(cmd);
    processNextCommand();
}

bool ConnectorPrivate::isNextCommandReady() const
{
    assert(!m_commandQueue.empty());

    if (m_commandQueue.front() == Command::Register) {
        return !m_vapidRequired || !m_vapidPublicKey.isEmpty();
    }
    return true;
}

void ConnectorPrivate::processNextCommand()
{
    if (m_currentCommand != Command::None || !hasDistributor() || m_commandQueue.empty() || !isNextCommandReady()) {
        return;
    }

    m_currentCommand = m_commandQueue.front();
    m_commandQueue.pop_front();

    switch (m_currentCommand) {
        case Command::None:
            break;
        case Command::Register:
        {
            if (m_state == Connector::Registered) {
                m_currentCommand = Command::None;
                break;
            }
            setState(Connector::Registering);
            if (m_token.isEmpty()) {
                m_token = QUuid::createUuid().toString();
            }
            qCDebug(Log) << "Registering";
            doRegister();
            break;
        }
        case Command::Unregister:
            if (m_state == Connector::Unregistered) {
                m_currentCommand = Command::None;
                break;
            }
            qCDebug(Log) << "Unregistering";
            doUnregister();
            break;
    }

    processNextCommand();
}

void ConnectorPrivate::ensureKeys()
{
    if (m_contentEnc.hasKeys()) {
        return;
    }
    m_contentEnc = ContentEncryption::generateKeys();
    storeState();
}


Connector::Connector(const QString &serviceName, QObject *parent)
    : QObject(parent)
    , d(new ConnectorPrivate(this))
{
    d->m_serviceName = serviceName;
    if (d->m_serviceName.isEmpty()) {
        qCWarning(Log) << "empty D-Bus service name!";
        return;
    }

    d->loadState();
    d->setDistributor(ConnectorUtils::selectDistributor());
}

Connector::~Connector() = default;

QString Connector::endpoint() const
{
    return d->m_endpoint;
}

void Connector::registerClient(const QString &description)
{
    qCDebug(Log) << d->m_state;
    d->m_description = description;
    d->addCommand(ConnectorPrivate::Command::Register);
}

void Connector::unregisterClient()
{
    qCDebug(Log) << d->m_state;
    d->addCommand(ConnectorPrivate::Command::Unregister);
}

Connector::State Connector::state() const
{
    return d->m_state;
}

QString Connector::vapidPublicKey() const
{
    return d->m_vapidPublicKey;
}

void Connector::setVapidPublicKey(const QString &vapidPublicKey)
{
    if (d->m_vapidPublicKey == vapidPublicKey) {
        return;
    }

    d->m_vapidPublicKey = vapidPublicKey;
    Q_EMIT vapidPublicKeyChanged();

    // if the VAPID key changed after we had previously registered we need to re-register
    if (!d->m_token.isEmpty()) {
        d->addCommand(ConnectorPrivate::Command::Unregister);
        d->addCommand(ConnectorPrivate::Command::None); // no-op as a barrier to prevent the other two command from being merged
        d->addCommand(ConnectorPrivate::Command::Register);
    } else {
        d->processNextCommand();
    }
}

bool Connector::vapidPublicKeyRequired() const
{
    return d->m_vapidRequired;
}

void Connector::setVapidPublicKeyRequired(bool vapidRequired)
{
    if (d->m_vapidRequired == vapidRequired) {
        return;
    }

    d->m_vapidRequired = vapidRequired;
    Q_EMIT vapidPublicKeyRequiredChanged();
    d->processNextCommand();
}

QByteArray Connector::contentEncryptionPublicKey() const
{
    d->ensureKeys();
    return d->m_contentEnc.publicKey();
}

QByteArray Connector::contentEncryptionAuthSecret() const
{
    d->ensureKeys();
    return d->m_contentEnc.authSecret();
}

void Connector::removeState()
{
    QFile::remove(d->stateFile());
}

#include "moc_connector.cpp"
#include "moc_connector_p.cpp"
