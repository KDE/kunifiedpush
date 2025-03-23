/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contentencryption_p.h"
#include "logging.h"
#include "opensslpp_p.h"

#include <QtEndian>

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

using namespace KUnifiedPush;

namespace KUnifiedPush {
class ContentEncryptionPrivate {
public:
    QByteArray m_publicKey;
    QByteArray m_privateKey;
    QByteArray m_authSecret;
};
}

[[nodiscard]] static openssl::evp_pkey_ptr ecKey(QByteArrayView publicKey, QByteArrayView privateKey = {})
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
        qCWarning(Log) << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }

    return openssl::evp_pkey_ptr(pkey);
}

[[nodiscard]] static QByteArray hmacSha256(QByteArrayView key, QByteArrayView data)
{
    QByteArray result(32, Qt::Uninitialized);
    unsigned int resultSize = 0;
    HMAC(EVP_sha256(), reinterpret_cast<const uint8_t*>(key.constData()), (int)key.size(),
                       reinterpret_cast<const uint8_t*>(data.constData()), data.size(),
                       reinterpret_cast<uint8_t*>(result.data()), &resultSize);
    return result;
}

constexpr inline auto AEAD_TAG_SIZE = 16;
constexpr inline auto AUTH_SECRET_SIZE = 16;

ContentEncryption::ContentEncryption() = default;
ContentEncryption::ContentEncryption(ContentEncryption &&) noexcept = default;

ContentEncryption::ContentEncryption(const QByteArray &publicKey, const QByteArray &privateKey, const QByteArray &authSecret)
    : d(std::make_unique<ContentEncryptionPrivate>())
{
    d->m_publicKey = publicKey;
    d->m_privateKey = privateKey;
    d->m_authSecret = authSecret;
}

ContentEncryption::~ContentEncryption() = default;

ContentEncryption& ContentEncryption::operator=(ContentEncryption&&) noexcept = default;

QByteArray ContentEncryption::publicKey() const
{
    return d ? d->m_publicKey : QByteArray();
}

QByteArray ContentEncryption::privateKey() const
{
    return d ? d->m_privateKey : QByteArray();
}

QByteArray ContentEncryption::authSecret() const
{
    return d ? d->m_authSecret : QByteArray();
}

bool ContentEncryption::hasKeys() const
{
    return d && d->m_publicKey.size() == 65 && d->m_privateKey.size() == 32 && d->m_authSecret.size() == AUTH_SECRET_SIZE;
}

QByteArray ContentEncryption::decrypt(const QByteArray &encrypted) const
{
    if (!hasKeys()) {
        return {};
    }

    if (encrypted.size() < 22) {
        qCWarning(Log) << "Encrypted message is too short!";
        return {};
    }

    // decode header according to RFC 8188 §2.1
    const auto salt = QByteArrayView(encrypted).left(16);
    //const auto rs = qFromBigEndian(*reinterpret_cast<const uint32_t*>(encrypted.constData() + 16));
    const auto idlen = *reinterpret_cast<const uint8_t*>(encrypted.constData() + 20);
    if (encrypted.size() < 22 + idlen + AEAD_TAG_SIZE) {
        qCWarning(Log) << "idlen exceeds encrypted message size!";
        return {};
    }
    const auto keyid = QByteArrayView(encrypted).mid(21, idlen); // sender public key in RFC 8291
    const auto encryptedContent = QByteArrayView(encrypted).mid(21 + idlen);

    // load user agent key pair
    const auto pkey = ecKey(d->m_publicKey, d->m_privateKey);
    const auto peerKey = ecKey(keyid);
    if (!pkey || !peerKey) {
        qCWarning(Log) << "Failed to load EC keys!";
        return {};
    }

    // derive ECDH shared secret
    openssl::evp_pkey_ctx_ptr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    EVP_PKEY_derive_init(ctx.get());
    EVP_PKEY_derive_set_peer(ctx.get(), peerKey.get());
    std::size_t secret_len = 0;
    EVP_PKEY_derive(ctx.get(), nullptr, &secret_len);
    QByteArray ecdh_secret((qsizetype)secret_len, Qt::Uninitialized);
    if (const auto res = EVP_PKEY_derive(ctx.get(), reinterpret_cast<uint8_t*>(ecdh_secret.data()), &secret_len); res <= 0) {
        qCWarning(Log) << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }

    // determine content encoding key and nonce
    const auto prk_key = hmacSha256(d->m_authSecret, ecdh_secret);
    const QByteArray key_info = QByteArrayView("WebPush: info") + '\x00' + d->m_publicKey + keyid + '\x01';
    const auto ikm = hmacSha256(prk_key, key_info);
    const auto prk = hmacSha256(salt, ikm);
    const auto cek = hmacSha256(prk, QByteArrayView("Content-Encoding: aes128gcm\x00\x01", 29)).left(16);
    const auto nonce = hmacSha256(prk, QByteArrayView("Content-Encoding: nonce\x00\x01", 25)).left(12);

    // AES 128 GCM decryption with 16 byte AEAD tag
    QByteArray plaintext(encryptedContent.size() - AEAD_TAG_SIZE, Qt::Uninitialized);
    int plaintextLen = 0, len = 0;

    openssl::evp_cipher_ctx_ptr aesCtx(EVP_CIPHER_CTX_new());
    EVP_DecryptInit(aesCtx.get(), EVP_aes_128_gcm(), reinterpret_cast<const uint8_t*>(cek.constData()), reinterpret_cast<const uint8_t*>(nonce.constData()));
    EVP_DecryptUpdate(aesCtx.get(), reinterpret_cast<uint8_t*>(plaintext.data()), &plaintextLen, reinterpret_cast<const uint8_t*>(encryptedContent.constData()), (int)encryptedContent.size() - AEAD_TAG_SIZE);
    EVP_CIPHER_CTX_ctrl(aesCtx.get(), EVP_CTRL_GCM_SET_TAG, AEAD_TAG_SIZE, const_cast<void*>(reinterpret_cast<const void*>(encryptedContent.right(AEAD_TAG_SIZE).constData())));
    if (const auto res = EVP_DecryptFinal_ex(aesCtx.get(), reinterpret_cast<uint8_t*>(plaintext.data() + plaintextLen), &len); res <= 0) {
        qCWarning(Log) << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }
    plaintextLen += len;

    // remove padding
    for (len = plaintextLen - 1; len; --len) {
        if (plaintext[len] == 0x02) {
            break;
        }
    }
    plaintext.resize(len);

    return plaintext;
}

ContentEncryption ContentEncryption::generateKeys()
{
    ContentEncryption c;
    c.d = std::make_unique<ContentEncryptionPrivate>();

    // EC keypair
    openssl::evp_pkey_ptr key(EVP_EC_gen("prime256v1"));
    if (!key) {
        return {};
    }

    OSSL_PARAM *paramPtr = nullptr;
    EVP_PKEY_todata(key.get(), EVP_PKEY_KEYPAIR, &paramPtr);
    openssl::ossl_param_ptr params(paramPtr);
    for (;paramPtr->key; ++paramPtr) {
        if (paramPtr->data_type == OSSL_PARAM_OCTET_STRING && std::strcmp(paramPtr->key, "pub") == 0) {
            c.d->m_publicKey.resize((qsizetype)paramPtr->data_size);
            std::size_t len = 0;
            auto data = reinterpret_cast<void*>(c.d->m_publicKey.data());
            OSSL_PARAM_get_octet_string(paramPtr, &data, paramPtr->data_size, &len);
        }
        if (paramPtr->data_type == OSSL_PARAM_UNSIGNED_INTEGER && std::strcmp(paramPtr->key, "priv") == 0) {
            BIGNUM *valPtr = nullptr;
            OSSL_PARAM_get_BN(paramPtr, &valPtr);
            openssl::bn_ptr val(valPtr);

            c.d->m_privateKey.resize(BN_num_bytes(valPtr));
            BN_bn2bin(valPtr, reinterpret_cast<uint8_t*>(c.d->m_privateKey.data()));
        }
    }

    // auth secret
    c.d->m_authSecret.resize(AUTH_SECRET_SIZE);
    if (RAND_bytes(reinterpret_cast<uint8_t*>(c.d->m_authSecret.data()), (int)c.d->m_authSecret.size()) != 1) {
        return {};
    }

    return c;
}
