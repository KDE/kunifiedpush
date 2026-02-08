/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include "managementinterface.h"

#include "../src/distributor/distributor.h"
#include "../src/shared/unifiedpush-constants.h"

#include <QDBusConnection>
#include <QLoggingCategory>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

using namespace Qt::Literals;

class ConnectorTest : public QObject
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
        settings.setValue(QStringLiteral("PushProvider/Type"), QStringLiteral("Mock"));
        settings.sync();

        QLoggingCategory::setFilterRules(QStringLiteral("org.kde.kunifiedpush.*=true"));
    }

    void init()
    {
        QSettings stateSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-org.kde.kunifiedpush.connectortest"), QSettings::IniFormat);
        stateSettings.clear();
    }

    void testNoDistributor()
    {
        KUnifiedPush::Connector con(QStringLiteral("org.kde.kunifiedpush.connectortest"));
        QCOMPARE(con.endpoint(), QString());
        QCOMPARE(con.state(), KUnifiedPush::Connector::NoDistributor);
    }

    void testWithDistributor()
    {
        using namespace KUnifiedPush;
        std::unique_ptr<Connector> con;
        std::unique_ptr<QSignalSpy> stateSpy;
        std::unique_ptr<QSignalSpy> endpointSpy;

        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kunifiedpush.connectortest")));
        con.reset(new Connector(QStringLiteral("org.kde.kunifiedpush.connectortest")));
        QCOMPARE(con->state(), KUnifiedPush::Connector::NoDistributor);
        stateSpy.reset(new QSignalSpy(con.get(), &Connector::stateChanged));
        endpointSpy.reset(new QSignalSpy(con.get(), &Connector::endpointChanged));

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
        con->setVapidPublicKey(u"<vapid-key-1>"_s);
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy->size(), 1);
        QCOMPARE(con->endpoint(), QLatin1String("https://localhost/push-endpoint"));
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Connected);

        // connector restart does not register at the provider but uses existing state
        {
            con.reset();
            QTest::qWait(10); // propagate connector D-Bus tear-down
            con.reset(new Connector(QStringLiteral("org.kde.kunifiedpush.connectortest")));
            QCOMPARE(con->endpoint(), QLatin1String("https://localhost/push-endpoint"));
            QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
            stateSpy.reset(new QSignalSpy(con.get(), &Connector::stateChanged));
            endpointSpy.reset(new QSignalSpy(con.get(), &Connector::endpointChanged));
            QVERIFY(stateSpy->wait());
            QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        }

        // distributor goes away and comes back
        ctrlIface.call("quit"_L1);
        QVERIFY(dist.waitForFinished());
        QVERIFY(dbusUnregistrationSpy.wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::NoDistributor);
        dist.start(QCoreApplication::applicationDirPath() + "/mockdistributor"_L1);
        QVERIFY(dist.waitForStarted());
        QVERIFY(dbusRegistrationSpy.wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Connected);

        // changing VAPID key forces a re-registration
        endpointSpy->clear();
        con->setVapidPublicKey(u"<vapid-key-2>"_s);
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
        QCOMPARE(endpointSpy->size(), 1);
        QCOMPARE(endpointSpy->at(0).at(0).toString(), QString());
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy->size(), 2);
        QCOMPARE(endpointSpy->at(1).at(0).toString(), "https://localhost/push-endpoint"_L1);
        endpointSpy->clear();

        // receiving a message
        QSignalSpy msgSpy(con.get(), &Connector::messageReceived);
        ctrlIface.callWithArgumentList(QDBus::AutoDetect, "receiveMessage"_L1, { u"<client-remote-id>"_s, QByteArray("hello world"), QString()});
        QVERIFY(msgSpy.wait());
        QCOMPARE(msgSpy.at(0).at(0).toByteArray(), "hello world");

        // receive a message with acknowledgement
        msgSpy.clear();
        ctrlIface.callWithArgumentList(QDBus::AutoDetect, "receiveMessage"_L1, { u"<client-remote-id>"_s, QByteArray("acknowledged message"), u"<msg-id>"_s});
        QVERIFY(msgSpy.wait());
        QCOMPARE(msgSpy.at(0).at(0).toByteArray(), "acknowledged message");

        // reconfigure distributor to a different push provider
        distIface.setPushProvider(QStringLiteral("Broken"), {});
        QVERIFY(endpointSpy->wait());
        QCOMPARE(con->endpoint(), QString());
        QVERIFY(distStatusSpy.wait());
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::NoSetup);

        distIface.setPushProvider(QStringLiteral("Mock"), {});
        if (distIface.status() == KUnifiedPush::DistributorStatus::NoSetup) {
            QVERIFY(distStatusSpy.wait());
        }
        if (distIface.status() == KUnifiedPush::DistributorStatus::Idle) {
            QVERIFY(distStatusSpy.wait());
        }
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Connected);
        QVERIFY(endpointSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(con->endpoint(), QLatin1String("https://localhost/push-endpoint"));

        QVariantMap config;
        config.insert(QStringLiteral("setting"), true);
        distIface.setPushProvider(QStringLiteral("Mock"), config);
        QVERIFY(endpointSpy->wait());
        QCOMPARE(con->endpoint(), QString());
        QVERIFY(endpointSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(con->endpoint(), QLatin1String("https://localhost/push-endpoint"));
        QCOMPARE(endpointSpy->size(), 4);

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

QTEST_GUILESS_MAIN(ConnectorTest)

#include "connectortest.moc"
#include "../src/shared/moc_distributorstatus_p.cpp"
