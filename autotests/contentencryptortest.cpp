/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/client/contentencryption_p.h"
#include "../src/notifier/contentencryptor_p.h"

#include <QTest>

using namespace KUnifiedPush;

class ContentEncryptorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEncrypt()
    {
        // see RFC 8291 §5 and Appendix A
        ContentEncryption decryptor(
            QByteArray::fromBase64("BCVxsr7N_eNgVRqvHtD0zTZsEc6-VV-JvLexhqUzORcxaOzi6-AYWXvTBHm4bjyPjs7Vd8pZGH6SRpkNtoIAiw4", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("q1dXpw3UpT5VOmu_cf_v6ih07Aems3njxI-JWgLcM94", QByteArray::Base64UrlEncoding),
            QByteArray::fromBase64("BTBZMqHH6r4Tts7J_aSIgg", QByteArray::Base64UrlEncoding)
        );
        QVERIFY(decryptor.hasKeys());

        ContentEncryptor encryptor(decryptor.publicKey(), decryptor.authSecret());
        const auto encrypted = encryptor.encrypt("When I grow up, I want to be a watermelon");
        QVERIFY(encrypted.size() > 21 + 65);
        const auto msg = decryptor.decrypt(encrypted);
        QCOMPARE(msg, "When I grow up, I want to be a watermelon");
    }
};

QTEST_GUILESS_MAIN(ContentEncryptorTest)

#include "contentencryptortest.moc"
