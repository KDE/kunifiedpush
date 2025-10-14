/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selftest.h"

#include "../notifier/notifier.h"

#include <KUnifiedPush/Connector>

#include <KLocalizedString>

#include <QDBusConnection>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUuid>

constexpr inline const char VAPID_PUBLIC_KEY[] = "BCzlgilO4rGwV9yvrW8afgUJes4-wy4HuVRWH0BIt-5858aF21oSmB9agUz5eyvmxpAUruVyU7pBaQ9HvcWY0TY";
constexpr inline const char VAPID_PRIVATE_KEY[] = "dV5WqGE33-HmKyuvabQdE0vUrin-FuZYRbkspO9Vxco";

using namespace Qt::Literals;

SelfTest::SelfTest(QObject *parent)
    : QObject(parent)
{
    m_timer.setTimerType(Qt::VeryCoarseTimer);
    m_timer.setInterval(std::chrono::seconds(30));
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (m_state == WaitingForEndpoint || m_state == Submitting || m_state == WaitingForMessage) {
            setState(Error);
            setErrorMessage(i18n("The operation timed out."));
        }
    });
}

SelfTest::~SelfTest()
{
    if (m_connector) {
        m_connector->unregisterClient();
    }
}

void SelfTest::start()
{
    setErrorMessage({});

    m_connector = std::make_unique<KUnifiedPush::Connector>(QDBusConnection::sessionBus().baseService());
    m_connector->setVapidPublicKeyRequired(true);
    m_connector->setVapidPublicKey(QLatin1StringView(VAPID_PUBLIC_KEY));

    connect(m_connector.get(), &KUnifiedPush::Connector::endpointChanged, this, &SelfTest::endpointChanged);
    connect(m_connector.get(), &KUnifiedPush::Connector::messageReceived, this, &SelfTest::messageReceived);
    setState(WaitingForEndpoint);
    m_timer.start();

    m_connector->registerClient(i18n("Push notification self-test."));
}

void SelfTest::reset()
{
    setState(Idle);
    setErrorMessage({});
}

void SelfTest::endpointChanged()
{
    QUrl url(m_connector->endpoint());
    if (!url.isValid()) {
        setState(Error);
        setErrorMessage(i18n("Could not obtain push notification endpoint."));
        return;
    }

    m_notifier = std::make_unique<KUnifiedPush::Notifier>();
    m_notifier->setEndpoint(url);
    m_notifier->setUserAgentPublicKey(m_connector->contentEncryptionPublicKey());
    m_notifier->setAuthSecret(m_connector->contentEncryptionAuthSecret());
    m_notifier->setContact(u"https://invent.kde.org/libraries/kunifiedpush - KCM Self Test"_s);
    m_notifier->setVapidPublicKey(QByteArray::fromBase64(VAPID_PUBLIC_KEY, QByteArray::Base64UrlEncoding));
    m_notifier->setVapidPrivateKey(QByteArray::fromBase64(VAPID_PRIVATE_KEY, QByteArray::Base64UrlEncoding));
    m_notifier->setTtl(std::chrono::seconds(30));

    connect(m_notifier.get(), &KUnifiedPush::Notifier::finished, this, &SelfTest::submissionFinished);
    setState(Submitting);
    m_timer.start();

    m_msg = QUuid::createUuid().toByteArray();
    m_notifier->submit(m_msg, m_nam);
}

[[nodiscard]] static QString readErrorMessage(const QJsonObject &obj)
{
    for (const auto &key : { "message"_L1, "error"_L1 }) {
        if (const auto msg = obj.value(key).toString(); !msg.isEmpty()) {
            return msg;
        }
    }
    return {};
}

void SelfTest::submissionFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (const auto errMsg = readErrorMessage(obj); !errMsg.isEmpty()) {
            setErrorMessage(errMsg);
        } else {
            setErrorMessage(reply->errorString());
        }
        setState(Error); // ### careful, this invalidates reply!
        return;
    }

    setState(WaitingForMessage);
    m_timer.start();
}

void SelfTest::messageReceived(const QByteArray &msg)
{
    m_timer.stop();
    if (msg != m_msg) {
        setState(Error);
        setErrorMessage(i18n("Received notification does not have the expected content."));
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

    if (m_state == Success || m_state == Error || m_state == Idle) {
        m_notifier.reset();
        if (m_connector) {
            m_connector->unregisterClient();
            m_connector->removeState();
        }
        m_connector.reset();
        m_msg = {};
    }
}

void SelfTest::setErrorMessage(const QString &errMsg)
{
    if (m_errorMsg == errMsg) {
        return;
    }

    m_errorMsg = errMsg;
    Q_EMIT errorMessageChanged();
}

#include "moc_selftest.cpp"
