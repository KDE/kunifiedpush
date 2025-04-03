/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kunifiedpush_version.h"

#include "../shared/eckey_p.h"

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <openssl/ec.h>

#include <iostream>

using namespace Qt::Literals;
using namespace KUnifiedPush;

int main(int argc, char **argv)
{
    QCoreApplication::setApplicationName(u"VAPID Key Generator"_s);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(KUNIFIEDPUSH_VERSION_STRING));
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    openssl::evp_pkey_ptr key(EVP_EC_gen("prime256v1"));
    if (!key) {
        return {};
    }
    const auto keyPair = ECKey::store(key);

    std::cout << "public key: " << keyPair.publicKey.toBase64(QByteArray::Base64UrlEncoding).constData() << std::endl;
    std::cout << "private key: " << keyPair.privateKey.toBase64(QByteArray::Base64UrlEncoding).constData() << std::endl;

    return 0;
}
