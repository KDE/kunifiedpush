/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contentencryptionutils_p.h"

#include <QDebug>

#include <openssl/err.h>
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

openssl::evp_pkey_ptr ContentEcryptionUtils::ecKey(QByteArrayView publicKey, QByteArrayView privateKey)
{
    openssl::bn_ptr privBn;

    openssl::ossl_param_bld_ptr param_bld(OSSL_PARAM_BLD_new());
    OSSL_PARAM_BLD_push_utf8_string(param_bld.get(), "group", "prime256v1", 0);
    if (!privateKey.isEmpty()) {
        privBn.reset(BN_bin2bn(reinterpret_cast<const uint8_t*>(privateKey.constData()), (int)privateKey.size(), nullptr));
        OSSL_PARAM_BLD_push_BN(param_bld.get(), "priv", privBn.get());
    }
    OSSL_PARAM_BLD_push_octet_string(param_bld.get(), "pub", reinterpret_cast<const uint8_t*>(publicKey.constData()), publicKey.size());

    openssl::ossl_param_ptr params(OSSL_PARAM_BLD_to_param(param_bld.get()));

    openssl::evp_pkey_ctx_ptr ctx(EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr));
    EVP_PKEY_fromdata_init(ctx.get());
    EVP_PKEY *pkey = nullptr;
    if (const auto res = EVP_PKEY_fromdata(ctx.get(), &pkey, EVP_PKEY_KEYPAIR, params.get()); res <= 0) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }

    return openssl::evp_pkey_ptr(pkey);
}
