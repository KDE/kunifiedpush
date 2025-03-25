/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONTENCRYPTIONUTILS_P_H
#define KUNIFIEDPUSH_CONTENCRYPTIONUTILS_P_H

#include "opensslpp_p.h"

#include <QByteArray>

namespace KUnifiedPush
{

constexpr inline auto CE_SALT_SIZE = 16;
constexpr inline auto CE_RECORD_SIZE = 4096;
constexpr inline auto CE_AUTH_SECRET_SIZE = 16;
constexpr inline auto CE_AEAD_TAG_SIZE = 16;
constexpr inline uint8_t CE_MESSAGE_PADDING = 0x02;

/** Shared building blocks for RFC 8291 content encryption. */
class ContentEcryptionUtils {
public:
    /** @p size bytes of random data. */
    [[nodiscard]] static QByteArray random(qsizetype size);

    /** Derive shared ECDH secret. */
    [[nodiscard]] static QByteArray ecdhSharedSecret(const openssl::evp_pkey_ptr &key, const openssl::evp_pkey_ptr &peerKey);

    /** SHA-256 HMAC. */
    [[nodiscard]] static QByteArray hmacSha256(QByteArrayView key, QByteArrayView data);

    /** Derive content encryption key and nonce. */
    [[nodiscard]] static QByteArray cek(QByteArrayView prk);
    [[nodiscard]] static QByteArray nonce(QByteArrayView prk);
};

}

#endif

