/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/notifier/vapid_p.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::Literals;
using namespace KUnifiedPush;

class VapidTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEncrypt()
    {
        Vapid vapid(
            QByteArray::fromBase64("BCVxsr7N_eNgVRqvHtD0zTZsEc6-VV-JvLexhqUzORcxaOzi6-AYWXvTBHm4bjyPjs7Vd8pZGH6SRpkNtoIAiw4", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("q1dXpw3UpT5VOmu_cf_v6ih07Aems3njxI-JWgLcM94", QByteArray::Base64UrlEncoding)
        );
        vapid.setContact(u"mailto:kunifiedpush-unit-test@kde.org"_s);
        const auto auth = vapid.authorization(QUrl(u"https://unifiedpush.kde.org/some/random/identifier"_s));
        QVERIFY(!auth.isEmpty());
        QVERIFY(auth.startsWith("vapid "));
        QVERIFY(auth.contains("t="));
        QVERIFY(auth.contains("k="));

        auto idx = auth.indexOf("t=");
        auto t = QByteArrayView(auth).mid(idx + 2);
        idx = t.indexOf(',');
        if (idx > 0) {
            t = t.left(idx);
        }
        QVERIFY(!t.isEmpty());
        const auto jwt = t.toByteArray().split('.');
        QCOMPARE(jwt.size(), 3);
        const auto header = QJsonDocument::fromJson(QByteArray::fromBase64(jwt[0], QByteArray::Base64UrlEncoding)).object();
        QCOMPARE(header.size(), 2);
        const auto body = QJsonDocument::fromJson(QByteArray::fromBase64(jwt[1], QByteArray::Base64UrlEncoding)).object();
        QCOMPARE(body["sub"_L1], "mailto:kunifiedpush-unit-test@kde.org"_L1);
        QCOMPARE(body["aud"_L1], "https://unifiedpush.kde.org"_L1);
        QVERIFY(body["exp"_L1].isDouble());
    }
};

QTEST_GUILESS_MAIN(VapidTest)

#include "vapidtest.moc"
