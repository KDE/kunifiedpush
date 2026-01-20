/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include "managementinterface.h"

#include "../src/distributor/distributor.h"
#include "../src/notifier/notifier.h"
#include "../src/notifier/vapid_p.h"
#include "../src/shared/unifiedpush-constants.h"

#include <QDBusConnection>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

using namespace Qt::Literals;
using namespace KUnifiedPush;

constexpr inline auto VAPID_PUBLIC_KEY = "BO2UGoiyXMe_aG1L772_jL3Pfyj22Qqchzj6F7D_pW9k2IIxCvkF54J-sxM3VzBaP_jpsRk1IqI3hZpQA9Fq6N0"_L1;
constexpr inline const char VAPID_PRIVATE_KEY[] = "BYvQ4EhxzDDyiJti5k5jF9r1e8EnqrdoNyopghXeI-4";

class NtfyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("UNIFIEDPUSH_DISTRIBUTOR", "mock");
        qRegisterMetaType<KUnifiedPush::Connector::State>();

        QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
        QCoreApplication::setOrganizationName(QStringLiteral("KDE"));

        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/KDE/mockdistributor.conf"_L1, QSettings::NativeFormat);
        settings.clear();
        settings.setValue("PushProvider/Type", u"Ntfy"_s);
        settings.setValue("Ntfy/Url", u"http://127.0.0.1:8001"_s);
        // settings.setValue("Ntfy/AuthMethod", u"None"_s);
        settings.setValue("Ntfy/AuthMethod", u"Basic"_s);
        // settings.setValue("Ntfy/AuthMethod", u"Bearer"_s);

        settings.setValue("Ntfy/Username", u"me"_s);
        settings.setValue("Ntfy/Secret", u"secret"_s);
        // settings.setValue("Ntfy/Secret", u"tk_6ekky8c4mg0i2v2hqm6vzhd5qsfwt"_s);

        settings.sync();

        QLoggingCategory::setFilterRules(QStringLiteral("org.kde.kunifiedpush.*=true"));
    }

    void init()
    {
        QSettings stateSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-org.kde.kunifiedpush.ntfytest"), QSettings::IniFormat);
        stateSettings.clear();
    }

    void testNtfy()
    {
        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kunifiedpush.ntfytest")));
        const auto con = std::make_unique<Connector>(QStringLiteral("org.kde.kunifiedpush.ntfytest"));
        QCOMPARE(con->state(), KUnifiedPush::Connector::NoDistributor);
        const auto stateSpy = std::make_unique<QSignalSpy>(con.get(), &Connector::stateChanged);
        const auto endpointSpy = std::make_unique<QSignalSpy>(con.get(), &Connector::endpointChanged);

        // start mock distributor and wait for it to become available on D-Bus
        QProcess dist;
        dist.setProcessChannelMode(QProcess::ForwardedChannels);
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(u"QT_LOGGING_RULES"_s, u"org.kde.kunifiedpush.*=true"_s);
        dist.setProcessEnvironment(env);
        dist.start(QCoreApplication::applicationDirPath() + "/mockdistributor"_L1);
        QVERIFY(dist.waitForStarted());
        QDBusServiceWatcher dbusWatcher("org.unifiedpush.Distributor.mock"_L1, QDBusConnection::sessionBus());
        QSignalSpy dbusRegistrationSpy(&dbusWatcher, &QDBusServiceWatcher::serviceRegistered);
        QSignalSpy dbusUnregistrationSpy(&dbusWatcher, &QDBusServiceWatcher::serviceUnregistered);
        QVERIFY(dbusRegistrationSpy.wait());

        // D-Bus interfaces to interact with the distributor
        org::kde::kunifiedpush::Management distIface("org.unifiedpush.Distributor.mock"_L1, KDE_DISTRIBUTOR_MANAGEMENT_PATH, QDBusConnection::sessionBus());
        QVERIFY(distIface.isValid());
        QSignalSpy distStatusSpy(&distIface, &org::kde::kunifiedpush::Management::statusChanged);
        QDBusInterface ctrlIface("org.unifiedpush.Distributor.mock"_L1, "/MockController"_L1, "org.kde.unifiedpush.MockController"_L1, QDBusConnection::sessionBus());
        QVERIFY(ctrlIface.isValid());

        // initial idle state with no client registered
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Idle);
        QCOMPARE(con->state(), KUnifiedPush::Connector::Unregistered);

        // register a client
        con->setVapidPublicKeyRequired(true);
        con->registerClient(QStringLiteral("Push notification unit test."));
        QCOMPARE(con->state(), KUnifiedPush::Connector::Unregistered);
        con->setVapidPublicKey(VAPID_PUBLIC_KEY);
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy->size(), 1);
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Connected);

        // receiving a message
        QSignalSpy msgSpy(con.get(), &Connector::messageReceived);

        QNetworkAccessManager nam;
        Notifier notifier;
        notifier.setEndpoint(QUrl(con->endpoint()));
        notifier.setVapidPrivateKey(VAPID_PRIVATE_KEY);
        notifier.setUserAgentPublicKey(con->contentEncryptionPublicKey());
        notifier.setAuthSecret(con->contentEncryptionAuthSecret());
        notifier.setContact(u"KUnifiedPush unit test"_s);
        notifier.submit("Hello world", &nam);

        QVERIFY(msgSpy.wait());
        QCOMPARE(msgSpy.at(0).at(0).toByteArray(), "Hello world");

        // connector unregisters
        endpointSpy->clear();
        con->unregisterClient();
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Unregistered);
        QCOMPARE(endpointSpy->size(), 1);
        QCOMPARE(con->endpoint(), QString());
        QVERIFY(distStatusSpy.wait());
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Idle);

        ctrlIface.call("quit"_L1);
        QVERIFY(dist.waitForFinished());
    }
};

QTEST_GUILESS_MAIN(NtfyTest)

#include "ntfytest.moc"
#include "../src/shared/moc_distributorstatus_p.cpp"
