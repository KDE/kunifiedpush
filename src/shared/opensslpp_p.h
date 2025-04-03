/*
 * SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KUNIFIEDPUSH_OPENSSLPP_P_H
#define KUNIFIEDPUSH_OPENSSLPP_P_H

#include <functional>
#include <memory>

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>

namespace openssl {

    namespace detail {
        template <typename T, void(*F)(T*)>
        struct deleter {
            void operator()(T *ptr) { std::invoke(F, ptr); }
        };
    }

    using bn_ptr = std::unique_ptr<BIGNUM, detail::deleter<BIGNUM, &BN_free>>;
    using ecdsa_sig_ptr = std::unique_ptr<ECDSA_SIG, detail::deleter<ECDSA_SIG, &ECDSA_SIG_free>>;
    using evp_cipher_ctx_ptr = std::unique_ptr<EVP_CIPHER_CTX, detail::deleter<EVP_CIPHER_CTX, &EVP_CIPHER_CTX_free>>;
    using evp_pkey_ptr = std::unique_ptr<EVP_PKEY, detail::deleter<EVP_PKEY, &EVP_PKEY_free>>;
    using evp_md_ctx_ptr = std::unique_ptr<EVP_MD_CTX, detail::deleter<EVP_MD_CTX, &EVP_MD_CTX_free>>;
    using evp_pkey_ctx_ptr = std::unique_ptr<EVP_PKEY_CTX, detail::deleter<EVP_PKEY_CTX, &EVP_PKEY_CTX_free>>;
    using ossl_param_ptr = std::unique_ptr<OSSL_PARAM, detail::deleter<OSSL_PARAM, &OSSL_PARAM_free>>;
    using ossl_param_bld_ptr = std::unique_ptr<OSSL_PARAM_BLD, detail::deleter<OSSL_PARAM_BLD, &OSSL_PARAM_BLD_free>>;
}

#endif
