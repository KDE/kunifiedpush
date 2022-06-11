/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/shared/connectorutils.cpp"

#include <QTest>

namespace KUnifiedPush {

class ConnectorUtilsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSelectDistributor()
    {
        QCOMPARE(ConnectorUtils::selectDistributor({}), QString());
        QCOMPARE(ConnectorUtils::selectDistributor({ QStringLiteral("some.service") }), QString());
        QCOMPARE(ConnectorUtils::selectDistributor({ QStringLiteral("some.service"), QStringLiteral("org.unifiedpush.Distributor.mock") }), QLatin1String("org.unifiedpush.Distributor.mock"));
        QCOMPARE(ConnectorUtils::selectDistributor({ QStringLiteral("some.service"), QStringLiteral("org.unifiedpush.Distributor.mock2"), QStringLiteral("org.unifiedpush.Distributor.mock1") }), QLatin1String("org.unifiedpush.Distributor.mock1"));
        QCOMPARE(ConnectorUtils::selectDistributor({ QStringLiteral("some.service"), QStringLiteral("org.unifiedpush.Distributor.kde"), QStringLiteral("org.unifiedpush.Distributor.abc") }), QLatin1String("org.unifiedpush.Distributor.kde"));

        qputenv("UNIFIEDPUSH_DISTRIBUTOR", "abc");
        QCOMPARE(ConnectorUtils::selectDistributor({ QStringLiteral("some.service"), QStringLiteral("org.unifiedpush.Distributor.kde"), QStringLiteral("org.unifiedpush.Distributor.abc") }), QLatin1String("org.unifiedpush.Distributor.abc"));
    }
};

}

QTEST_GUILESS_MAIN(KUnifiedPush::ConnectorUtilsTest)

#include "connectorutilstest.moc"
