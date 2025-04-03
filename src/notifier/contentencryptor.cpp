/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contentencryptor_p.h"

#include "../shared/contentencryptionutils_p.h"
#include "../shared/eckey_p.h"

#include <QtEndian>
#include <QDebug>

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/params.h>

using namespace KUnifiedPush;

ContentEncryptor::ContentEncryptor(const QByteArray &userAgentPublicKey, const QByteArray &authSecret)
    : m_userAgentPublicKey(userAgentPublicKey)
    , m_authSecret(authSecret)
{
}

ContentEncryptor::~ContentEncryptor() = default;

QByteArray ContentEncryptor::encrypt(const QByteArray &msg)
{
    // application server EC keypair
    const openssl::evp_pkey_ptr key(EVP_EC_gen("prime256v1"));
    const auto keyPair = ECKey::store(key, EVP_PKEY_PUBLIC_KEY);
    // user agent EC public key
    const auto peerKey = ECKey::load(m_userAgentPublicKey);
    if (!key || !peerKey) {
        qWarning() << "Failed to load EC keys!";
        return {};
    }

    // salt
    const auto salt = ContentEcryptionUtils::random(CE_SALT_SIZE);
    if (salt.size() != CE_SALT_SIZE) {
        return {};
    }

    // shared ECDH secret
    const auto ecdh_secret = ContentEcryptionUtils::ecdhSharedSecret(key, peerKey);
    if (ecdh_secret.isEmpty()) {
        return {};
    }

    // determine content encoding key and nonce
    const auto prk_key = ContentEcryptionUtils::hmacSha256(m_authSecret, ecdh_secret);
    const QByteArray key_info = QByteArrayView("WebPush: info") + '\x00' + m_userAgentPublicKey + keyPair.publicKey + '\x01';
    const auto ikm = ContentEcryptionUtils::hmacSha256(prk_key, key_info);
    const auto prk = ContentEcryptionUtils::hmacSha256(salt, ikm);
    const auto cek = ContentEcryptionUtils::cek(prk);
    const auto nonce = ContentEcryptionUtils::nonce(prk);

    // AES 128 GCM decryption with 16 byte AEAD tag
    const QByteArray input = msg + (char)CE_MESSAGE_PADDING;
    // TODO inplace into encoded?
    QByteArray encrypted(input.size() + CE_AEAD_TAG_SIZE + 16, Qt::Uninitialized);
    int encryptedLen = 0, len = 0;

    openssl::evp_cipher_ctx_ptr aesCtx(EVP_CIPHER_CTX_new());
    EVP_EncryptInit(aesCtx.get(), EVP_aes_128_gcm(), reinterpret_cast<const uint8_t*>(cek.constData()), reinterpret_cast<const uint8_t*>(nonce.constData()));
    EVP_EncryptUpdate(aesCtx.get(), reinterpret_cast<uint8_t*>(encrypted.data()), &len, reinterpret_cast<const uint8_t*>(input.constData()), (int)input.size());
    encryptedLen = len;
    if (EVP_EncryptFinal(aesCtx.get(), reinterpret_cast<uint8_t*>(encrypted.data() + len), &len) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }
    encrypted.resize(encryptedLen + len + CE_AEAD_TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(aesCtx.get(), EVP_CTRL_GCM_GET_TAG, CE_AEAD_TAG_SIZE, reinterpret_cast<uint8_t*>(encrypted.data() + encrypted.size() - CE_AEAD_TAG_SIZE)) != 1) {
        qWarning() << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }

    //
    // created encoded message
    //
    QByteArray encoded;

    // 16 byte salt
    encoded += salt;

    // 4 byte record size
    const auto rs = qToBigEndian<uint32_t>(CE_RECORD_SIZE);
    encoded += QByteArrayView(reinterpret_cast<const char*>(&rs), 4);

    // 1 byte key len + application server public key
    const char idlen = (char)(uint8_t)keyPair.publicKey.size();
    encoded += idlen;
    encoded += keyPair.publicKey;

    // actual message
    encoded += encrypted;
    return encoded;
}
