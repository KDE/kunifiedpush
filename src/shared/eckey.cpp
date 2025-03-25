/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "eckey_p.h"

#include <QDebug>

#include <openssl/err.h>

using namespace KUnifiedPush;

openssl::evp_pkey_ptr ECKey::load(QByteArrayView publicKey, QByteArrayView privateKey)
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

ECKeyPair ECKey::store(const openssl::evp_pkey_ptr &key, int selection)
{
    ECKeyPair pair;

    OSSL_PARAM *paramPtr = nullptr;
    EVP_PKEY_todata(key.get(), selection, &paramPtr);
    openssl::ossl_param_ptr params(paramPtr);
    for (;paramPtr->key; ++paramPtr) {
        if (paramPtr->data_type == OSSL_PARAM_OCTET_STRING && std::strcmp(paramPtr->key, "pub") == 0) {
            pair.publicKey.resize((qsizetype)paramPtr->data_size);
            std::size_t len = 0;
            auto data = reinterpret_cast<void*>(pair.publicKey.data());
            OSSL_PARAM_get_octet_string(paramPtr, &data, paramPtr->data_size, &len);
        }
        if (paramPtr->data_type == OSSL_PARAM_UNSIGNED_INTEGER && std::strcmp(paramPtr->key, "priv") == 0) {
            BIGNUM *valPtr = nullptr;
            OSSL_PARAM_get_BN(paramPtr, &valPtr);
            openssl::bn_ptr val(valPtr);

            pair.privateKey.resize(BN_num_bytes(valPtr));
            BN_bn2bin(valPtr, reinterpret_cast<uint8_t*>(pair.privateKey.data()));
        }
    }


    return pair;
}
