/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include "../src/distributor/distributor.h"
#include "../src/distributor/message.h"
#include "../src/distributor/mockpushprovider.h"

#include <QDBusConnection>
#include <QLoggingCategory>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

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

        QSettings settings;
        settings.clear();
        settings.setValue(QStringLiteral("PushProvider/Type"), QStringLiteral("Mock"));

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

        Distributor dist;
        QCOMPARE(dist.status(), KUnifiedPush::DistributorStatus::Idle);
        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.unifiedpush.Distributor.mock")));

        QVERIFY(MockPushProvider::s_instance);
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Unregistered);

        con->registerClient(QStringLiteral("Push notification unit test."));
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy->wait());
        // FIXME: delayed D-Bus replies inside the same process seem not to be actually delayed, so state get messed up after this...
        // QCOMPARE(con.state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy->size(), 1);
        QCOMPARE(con->endpoint(), QLatin1String("https://localhost/push-endpoint"));
        QCOMPARE(dist.status(), KUnifiedPush::DistributorStatus::Connected);

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
        QVERIFY(QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.unifiedpush.Distributor.mock")));
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::NoDistributor);
        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.unifiedpush.Distributor.mock")));
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(dist.status(), KUnifiedPush::DistributorStatus::Connected);

        // receiving a message
        QSignalSpy msgSpy(con.get(), &Connector::messageReceived);
        KUnifiedPush::Message msg;
        msg.clientRemoteId = QStringLiteral("<client-remote-id>");
        msg.content = QStringLiteral("hello world");
        Q_EMIT MockPushProvider::s_instance->messageReceived(msg);
        QVERIFY(msgSpy.wait());
        QCOMPARE(msgSpy.at(0).at(0).toByteArray(), "hello world");

        // connector unregisters
        QCOMPARE(endpointSpy->size(), 0);
        con->unregisterClient();
        QVERIFY(stateSpy->wait());
        QCOMPARE(con->state(), KUnifiedPush::Connector::Unregistered);
        QCOMPARE(endpointSpy->size(), 1);
        QCOMPARE(con->endpoint(), QString());
        QCOMPARE(dist.status(), KUnifiedPush::DistributorStatus::Idle);
    }
};

QTEST_GUILESS_MAIN(ConnectorTest)

#include "connectortest.moc"
#include "../src/shared/moc_distributorstatus_p.cpp"
