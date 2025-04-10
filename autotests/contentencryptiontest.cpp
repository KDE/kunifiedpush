/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/client/contentencryption_p.h"

#include <QTest>

using namespace KUnifiedPush;

class ContentEncryptionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDecrypt()
    {
        // see RFC 8291 §5 and Appendix A
        ContentEncryption enc(
            QByteArray::fromBase64("BCVxsr7N_eNgVRqvHtD0zTZsEc6-VV-JvLexhqUzORcx aOzi6-AYWXvTBHm4bjyPjs7Vd8pZGH6SRpkNtoIAiw4", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("q1dXpw3UpT5VOmu_cf_v6ih07Aems3njxI-JWgLcM94", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("BTBZMqHH6r4Tts7J_aSIgg", QByteArray::Base64UrlEncoding)
        );
        QVERIFY(enc.hasKeys());

        const auto msg = enc.decrypt(QByteArray::fromBase64("DGv6ra1nlYgDCS1FRnbzlwAAEABBBP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A_yl95bQpu6cVPTpK4Mqgkf1CXztLVBSt2Ks3oZwbuwXPXLWyouBWLVWGNWQexSgSxsj_Qulcy4a-fN", QByteArray::Base64UrlEncoding));
        QCOMPARE(msg, "When I grow up, I want to be a watermelon");
    }

    void testEmpty()
    {
        ContentEncryption enc;
        QVERIFY(!enc.hasKeys());
        QVERIFY(enc.decrypt(QByteArray::fromBase64("DGv6ra1nlYgDCS1FRnbzlwAAEABBBP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A_yl95bQpu6cVPTpK4Mqgkf1CXztLVBSt2Ks3oZwbuwXPXLWyouBWLVWGNWQexSgSxsj_Qulcy4a-fN", QByteArray::Base64UrlEncoding)).isEmpty());
    }

    void testInvalidMessage()
    {
        ContentEncryption enc(
            QByteArray::fromBase64("BCVxsr7N_eNgVRqvHtD0zTZsEc6-VV-JvLexhqUzORcx aOzi6-AYWXvTBHm4bjyPjs7Vd8pZGH6SRpkNtoIAiw4", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("q1dXpw3UpT5VOmu_cf_v6ih07Aems3njxI-JWgLcM94", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("BTBZMqHH6r4Tts7J_aSIgg", QByteArray::Base64UrlEncoding)
        );
        QVERIFY(enc.hasKeys());

        QVERIFY(enc.decrypt(QByteArray()).isEmpty());
        QVERIFY(enc.decrypt("When I grow up, I want to be a watermelon").isEmpty());
        const auto msg = enc.decrypt(QByteArray::fromBase64("DGv6ra1nlYgDCS1FRnbzlwAAEABBBP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A_yl95bQpu6cVPTpK4Mqgkf1CXztLVBSt2Ks3oZwbuwXPXLWyouBWLVWGNWQexSgSxsj_Qulcy4a-fM", QByteArray::Base64UrlEncoding));
        QCOMPARE(msg, QByteArray());
    }

    void testInvalidKey()
    {
        ContentEncryption enc(
            QByteArray::fromBase64("BCVxsr7N_eNgVRqvHtD0zTZsEc6-VV-JvLexhqUzORcx aOzi6-AYWXvTBHm4bjyPjs7Vd8pZGH6SRpkNtoIAiw5", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("q1dXpw3UpT5VOmu_cf_v6ih07Aems3njxI-JWgLcM94", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("BTBZMqHH6r4Tts7J_aSIgg", QByteArray::Base64UrlEncoding)
        );
        QVERIFY(enc.hasKeys());

        QVERIFY(enc.decrypt(QByteArray()).isEmpty());
        const auto msg = enc.decrypt(QByteArray::fromBase64("DGv6ra1nlYgDCS1FRnbzlwAAEABBBP4z9KsN6nGRTbVYI_c7VJSPQTBtkgcy27mlmlMoZIIgDll6e3vCYLocInmYWAmS6TlzAC8wEqKK6PBru3jl7A_yl95bQpu6cVPTpK4Mqgkf1CXztLVBSt2Ks3oZwbuwXPXLWyouBWLVWGNWQexSgSxsj_Qulcy4a-fM", QByteArray::Base64UrlEncoding));
        QCOMPARE(msg, QByteArray());
    }

    void testKeyGen()
    {
        const auto enc = ContentEncryption::generateKeys();
        QCOMPARE(enc.authSecret().size(), 16);
        QCOMPARE(enc.publicKey().size(), 65);
        QCOMPARE(enc.publicKey()[0], 0x04);
        // should be 32 but leading zeros are not included
        QVERIFY(enc.privateKey().size() <= 32);
        QVERIFY(enc.privateKey().size() > 0);
        QVERIFY(enc.hasKeys());
    }
};

QTEST_GUILESS_MAIN(ContentEncryptionTest)

#include "contentencryptiontest.moc"
