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

/** Shared building blocks for RFC 8291 content encryption. */
class ContentEcryptionUtils {
public:
    /** @p size bytes of random data. */
    [[nodiscard]] static QByteArray random(qsizetype size);

    /** Load an EC key (pair) from raw binary data. */
    [[nodiscard]] static openssl::evp_pkey_ptr ecKey(QByteArrayView publicKey, QByteArrayView privateKey = {});
};

}

#endif

