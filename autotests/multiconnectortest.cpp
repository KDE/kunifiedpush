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

class MultiConnectorTest : public QObject
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
        QSettings stateSettings1(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-org.kde.kunifiedpush.connectortest[identifier%2F1]"), QSettings::IniFormat);
        stateSettings1.clear();
        QSettings stateSettings2(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-org.kde.kunifiedpush.connectortest[identifier%2F2]"), QSettings::IniFormat);
        stateSettings2.clear();
    }

    void testMessageAreDistributedToExpectedConnector() {
        using namespace KUnifiedPush;
        std::unique_ptr<Connector> con1;
        std::unique_ptr<Connector> con2;
        std::unique_ptr<QSignalSpy> stateSpy1;
        std::unique_ptr<QSignalSpy> endpointSpy1;
        std::unique_ptr<QSignalSpy> messageSpy1;
        std::unique_ptr<QSignalSpy> stateSpy2;
        std::unique_ptr<QSignalSpy> endpointSpy2;
        std::unique_ptr<QSignalSpy> messageSpy2;

        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kunifiedpush.connectortest")));
        con1.reset(new Connector(QStringLiteral("org.kde.kunifiedpush.connectortest"), QStringLiteral("identifier/1")));
        QCOMPARE(con1->state(), KUnifiedPush::Connector::NoDistributor);
        stateSpy1.reset(new QSignalSpy(con1.get(), &Connector::stateChanged));
        endpointSpy1.reset(new QSignalSpy(con1.get(), &Connector::endpointChanged));
        messageSpy1.reset(new QSignalSpy(con1.get(), &Connector::messageReceived));

        con2.reset(new Connector(QStringLiteral("org.kde.kunifiedpush.connectortest"), QStringLiteral("identifier/2")));
        QCOMPARE(con2->state(), KUnifiedPush::Connector::NoDistributor);
        stateSpy2.reset(new QSignalSpy(con2.get(), &Connector::stateChanged));
        endpointSpy2.reset(new QSignalSpy(con2.get(), &Connector::endpointChanged));
        messageSpy2.reset(new QSignalSpy(con2.get(), &Connector::messageReceived));

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
        QCOMPARE(con1->state(), KUnifiedPush::Connector::Unregistered);

        // register a client
        con1->setVapidPublicKeyRequired(true);
        con1->registerClient(QStringLiteral("Push notification unit test 1."));
        QCOMPARE(con1->state(), KUnifiedPush::Connector::Unregistered);
        con1->setVapidPublicKey(u"<vapid-key-1>"_s);
        QCOMPARE(con1->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy1->wait());
        QCOMPARE(con1->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy1->size(), 1);
        QCOMPARE(con1->endpoint(), QLatin1String("https://localhost/push-endpoint"));
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Connected);

        // initial idle state with no client registered for second connector
        QCOMPARE(con2->state(), KUnifiedPush::Connector::Unregistered);

        // register a client
        con2->setVapidPublicKeyRequired(true);
        con2->registerClient(QStringLiteral("Push notification unit test 2."));
        QCOMPARE(con2->state(), KUnifiedPush::Connector::Unregistered);
        con2->setVapidPublicKey(u"<vapid-key-2>"_s);
        QCOMPARE(con2->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy2->wait());
        QCOMPARE(con2->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy2->size(), 1);
        QCOMPARE(con2->endpoint(), QLatin1String("https://localhost/push-endpoint"));
        QCOMPARE(distIface.status(), KUnifiedPush::DistributorStatus::Connected);


        // receiving a message on connector 1 is delivered to con1
        QSettings stateSettings1(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-org.kde.kunifiedpush.connectortest[identifier%2F1]"), QSettings::IniFormat);
        stateSettings1.beginGroup("Client");
        QString remoteId1 = stateSettings1.value("Token").toString() + QStringLiteral("-remote-id");
        ctrlIface.callWithArgumentList(QDBus::AutoDetect, "receiveMessage"_L1, { remoteId1, QByteArray("hello world 1"), u"<msg-id-1>"_s});
        QVERIFY(messageSpy1->wait());
        QCOMPARE(messageSpy1->at(0).at(0).toByteArray(), "hello world 1");

        // receiving a message on connector 1 is delivered to con2
        QSettings stateSettings2(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1String("/kunifiedpush-org.kde.kunifiedpush.connectortest[identifier%2F2]"), QSettings::IniFormat);
        stateSettings2.beginGroup("Client");
        QString remoteId2 = stateSettings2.value("Token").toString() + QStringLiteral("-remote-id");
        ctrlIface.callWithArgumentList(QDBus::AutoDetect, "receiveMessage"_L1, { remoteId2, QByteArray("hello world 2"), u"<msg-id-2>"_s});
        QVERIFY(messageSpy2->wait());
        QCOMPARE(messageSpy2->at(0).at(0).toByteArray(), "hello world 2");
    }
};

QTEST_GUILESS_MAIN(MultiConnectorTest)

#include "multiconnectortest.moc"
#include "../src/shared/moc_distributorstatus_p.cpp"
