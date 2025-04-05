/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selftest.h"

#include "../notifier/notifier.h"

#include <KUnifiedPush/Connector>

#include <QDBusConnection>
#include <QNetworkReply>
#include <QUuid>

SelfTest::SelfTest(QObject *parent)
    : QObject(parent)
{
}

SelfTest::~SelfTest() = default;

void SelfTest::start()
{
    m_connector = std::make_unique<KUnifiedPush::Connector>(QDBusConnection::sessionBus().name());
    m_connector->setVapidPublicKeyRequired(true);

    connect(m_connector.get(), &KUnifiedPush::Connector::endpointChanged, this, &SelfTest::endpointChanged);
    connect(m_connector.get(), &KUnifiedPush::Connector::messageReceived, this, &SelfTest::messageReceived);
    setState(WaitingForEndpoint);
}

void SelfTest::endpointChanged()
{
    QUrl url(m_connector->endpoint());
    if (!url.isValid()) {
        setState(Error);
        // TODO error message
        return;
    }

    m_notifier = std::make_unique<KUnifiedPush::Notifier>();
    m_notifier->setEndpoint(url);
    m_notifier->setUserAgentPublicKey(m_connector->contentEncryptionPublicKey());
    m_notifier->setAuthSecret(m_connector->contentEncryptionAuthSecret());
    // m_notifier->setVapidPublicKey(); // TODO baked in VAPID keys
    // m_notifier->setVapidPrivateKey();
    m_notifier->setTtl(std::chrono::seconds(30)); // TODO add local timeout to error

    connect(m_notifier.get(), &KUnifiedPush::Notifier::finished, this, &SelfTest::submissionFinished);
    setState(Submitting);

    m_msg = QUuid::createUuid().toByteArray();
    m_notifier->submit(m_msg, m_nam);
}

void SelfTest::submissionFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        setState(Error);
        // TODO error message
        return;
    }

    setState(WaitingForMessage);
}

void SelfTest::messageReceived(const QByteArray &msg)
{
    if (msg != m_msg) {
        setState(Error);
        // TODO error message
        return;
    }

    setState(Success);
}

void SelfTest::setState(State state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    Q_EMIT stateChanged();

    // TODO teardown on Success/Error
}
