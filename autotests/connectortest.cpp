/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include <QStandardPaths>
#include <QTest>

class ConnectorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("UNIFIEDPUSH_DISTRIBUTOR", "dummy");
    }

    void testUnknownServiceFailure()
    {
        KUnifiedPush::Connector con(QStringLiteral("org.kde.kunifiedpush.connectortest"));
        QCOMPARE(con.endpoint(), QString());
    }

    // not much testable beyond this without a mock distributor...
};

QTEST_GUILESS_MAIN(ConnectorTest)

#include "connectortest.moc"
