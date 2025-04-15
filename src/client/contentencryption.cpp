/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contentencryption_p.h"
#include "logging.h"

#include "../shared/contentencryptionutils_p.h"
#include "../shared/eckey_p.h"
#include "../shared/opensslpp_p.h"

#include <QtEndian>

#include <openssl/ec.h>
#include <openssl/err.h>

using namespace KUnifiedPush;

namespace KUnifiedPush {
class ContentEncryptionPrivate {
public:
    QByteArray m_publicKey;
    QByteArray m_privateKey;
    QByteArray m_authSecret;
};
}

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
    // private key should be 32 byte long, but leading zeros are not included
    return d && d->m_publicKey.size() == 65 && !d->m_privateKey.isEmpty() && d->m_privateKey.size() <= 32 && d->m_authSecret.size() == CE_AUTH_SECRET_SIZE;
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
    const auto salt = QByteArrayView(encrypted).left(CE_SALT_SIZE);
    if (const auto rs = qFromBigEndian(*reinterpret_cast<const uint32_t*>(encrypted.constData() + 16)); rs != CE_RECORD_SIZE) {
        qCWarning(Log) << "unexpected rs:" << rs;
        return {};
    }
    const auto idlen = *reinterpret_cast<const uint8_t*>(encrypted.constData() + 20);
    if (encrypted.size() < 22 + idlen + CE_AEAD_TAG_SIZE) {
        qCWarning(Log) << "idlen exceeds encrypted message size!";
        return {};
    }
    const auto keyid = QByteArrayView(encrypted).mid(21, idlen); // sender public key in RFC 8291
    const auto encryptedContent = QByteArrayView(encrypted).mid(21 + idlen);

    // load user agent key pair
    const auto pkey = ECKey::load(d->m_publicKey, d->m_privateKey);
    const auto peerKey = ECKey::load(keyid);
    if (!pkey || !peerKey) {
        qCWarning(Log) << "Failed to load EC keys!";
        return {};
    }

    // derive ECDH shared secret
    const auto ecdh_secret = ContentEcryptionUtils::ecdhSharedSecret(pkey, peerKey);
    if (ecdh_secret.isEmpty()) {
        return {};
    }

    // determine content encoding key and nonce
    const auto prk_key = ContentEcryptionUtils::hmacSha256(d->m_authSecret, ecdh_secret);
    const QByteArray key_info = QByteArrayView("WebPush: info") + '\x00' + d->m_publicKey + keyid + '\x01';
    const auto ikm = ContentEcryptionUtils::hmacSha256(prk_key, key_info);
    const auto prk = ContentEcryptionUtils::hmacSha256(salt, ikm);
    const auto cek = ContentEcryptionUtils::cek(prk);
    const auto nonce = ContentEcryptionUtils::nonce(prk);

    // AES 128 GCM decryption with 16 byte AEAD tag
    QByteArray plaintext(encryptedContent.size() - CE_AEAD_TAG_SIZE, Qt::Uninitialized);
    int plaintextLen = 0, len = 0;

    openssl::evp_cipher_ctx_ptr aesCtx(EVP_CIPHER_CTX_new());
    EVP_DecryptInit(aesCtx.get(), EVP_aes_128_gcm(), reinterpret_cast<const uint8_t*>(cek.constData()), reinterpret_cast<const uint8_t*>(nonce.constData()));
    EVP_DecryptUpdate(aesCtx.get(), reinterpret_cast<uint8_t*>(plaintext.data()), &plaintextLen, reinterpret_cast<const uint8_t*>(encryptedContent.constData()), (int)encryptedContent.size() - CE_AEAD_TAG_SIZE);
    EVP_CIPHER_CTX_ctrl(aesCtx.get(), EVP_CTRL_GCM_SET_TAG, CE_AEAD_TAG_SIZE, const_cast<void*>(reinterpret_cast<const void*>(encryptedContent.right(CE_AEAD_TAG_SIZE).constData())));
    if (const auto res = EVP_DecryptFinal_ex(aesCtx.get(), reinterpret_cast<uint8_t*>(plaintext.data() + plaintextLen), &len); res <= 0) {
        qCWarning(Log) << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }
    plaintextLen += len;

    // remove padding
    for (len = plaintextLen - 1; len; --len) {
        if (plaintext[len] == CE_MESSAGE_PADDING) {
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
    const auto keyPair = ECKey::store(key);
    c.d->m_publicKey = keyPair.publicKey;
    c.d->m_privateKey = keyPair.privateKey;

    // auth secret
    c.d->m_authSecret = ContentEcryptionUtils::random(CE_AUTH_SECRET_SIZE);

    return c;
}
