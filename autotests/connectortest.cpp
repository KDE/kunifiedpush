/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include "../src/distributor/distributor.h"

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

        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kunifiedpush.connectortest")));
        Connector con(QStringLiteral("org.kde.kunifiedpush.connectortest"));
        QCOMPARE(con.state(), KUnifiedPush::Connector::NoDistributor);
        QSignalSpy stateSpy(&con, &Connector::stateChanged);
        QSignalSpy endpointSpy(&con, &Connector::endpointChanged);

        QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.unifiedpush.Distributor.mock")));
        Distributor dist;
        QVERIFY(stateSpy.wait());
        QCOMPARE(con.state(), KUnifiedPush::Connector::Unregistered);

        con.registerClient();
        QCOMPARE(con.state(), KUnifiedPush::Connector::Registering);
        QVERIFY(stateSpy.wait());
        // FIXME: delayed D-Bus replies inside the same process seem not to be actually delayed, so state get messed up after this...
        // QCOMPARE(con.state(), KUnifiedPush::Connector::Registered);
        QCOMPARE(endpointSpy.size(), 1);
        QVERIFY(!con.endpoint().isEmpty());

        QVERIFY(QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.unifiedpush.Distributor.mock")));
        QVERIFY(stateSpy.wait());
        QCOMPARE(con.state(), KUnifiedPush::Connector::NoDistributor);
    }
};

QTEST_GUILESS_MAIN(ConnectorTest)

#include "connectortest.moc"
