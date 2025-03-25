/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_ECKEY_P_H
#define KUNIFIEDPUSH_ECKEY_P_H

#include "opensslpp_p.h"

#include <QByteArray>

namespace KUnifiedPush
{

class ECKeyPair {
public:
    QByteArray publicKey;
    QByteArray privateKey;
};

/** Utility methods for dealing with EC keypairs. */
class ECKey {
public:
    /** Load an EC key (pair) from raw binary data. */
    [[nodiscard]] static openssl::evp_pkey_ptr load(QByteArrayView publicKey, QByteArrayView privateKey = {});

    /** Store an EC key pair to raw binary data. */
    [[nodiscard]] static ECKeyPair store(const openssl::evp_pkey_ptr &key, int selection = EVP_PKEY_KEYPAIR);
};

}

#endif
