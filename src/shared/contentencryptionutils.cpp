/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contentencryptionutils_p.h"

#include <QDebug>

#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

using namespace KUnifiedPush;

QByteArray ContentEcryptionUtils::random(qsizetype size)
{
    QByteArray b(size, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<uint8_t*>(b.data()), (int)b.size()) != 1) {
        return {};
    }
    return b;
}

QByteArray ContentEcryptionUtils::ecdhSharedSecret(const openssl::evp_pkey_ptr &key, const openssl::evp_pkey_ptr &peerKey)
{
    openssl::evp_pkey_ctx_ptr ctx(EVP_PKEY_CTX_new(key.get(), nullptr));
    EVP_PKEY_derive_init(ctx.get());
    EVP_PKEY_derive_set_peer(ctx.get(), peerKey.get());
    std::size_t secret_len = 0;
    EVP_PKEY_derive(ctx.get(), nullptr, &secret_len);
    QByteArray ecdh_secret((qsizetype)secret_len, Qt::Uninitialized);
    if (const auto res = EVP_PKEY_derive(ctx.get(), reinterpret_cast<uint8_t*>(ecdh_secret.data()), &secret_len); res <= 0) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }
    return ecdh_secret;
}

QByteArray ContentEcryptionUtils::hmacSha256(QByteArrayView key, QByteArrayView data)
{
    QByteArray result(32, Qt::Uninitialized);
    unsigned int resultSize = 0;
    HMAC(EVP_sha256(), reinterpret_cast<const uint8_t*>(key.constData()), (int)key.size(),
                       reinterpret_cast<const uint8_t*>(data.constData()), data.size(),
                       reinterpret_cast<uint8_t*>(result.data()), &resultSize);
    return result;
}

QByteArray ContentEcryptionUtils::cek(QByteArrayView prk)
{
    return ContentEcryptionUtils::hmacSha256(prk, QByteArrayView("Content-Encoding: aes128gcm\x00\x01", 29)).left(16);
}

QByteArray ContentEcryptionUtils::nonce(QByteArrayView prk)
{
    return ContentEcryptionUtils::hmacSha256(prk, QByteArrayView("Content-Encoding: nonce\x00\x01", 25)).left(12);
}
