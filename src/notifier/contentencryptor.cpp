/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contentencryptor_p.h"

#include "../shared/contentencryptionutils_p.h"

#include <QtEndian>
#include <QDebug>

#include <openssl/ec.h>
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
    // user agent EC public key
    const auto peerKey = ContentEcryptionUtils::ecKey(m_userAgentPublicKey);
    if (!key || !peerKey) {
        qWarning() << "Failed to load EC keys!";
        return {};
    }

    const auto salt = ContentEcryptionUtils::random(CE_SALT_SIZE);
    if (salt.size() != CE_SALT_SIZE) {
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
    OSSL_PARAM *paramPtr = nullptr;
    EVP_PKEY_todata(key.get(), EVP_PKEY_PUBLIC_KEY, &paramPtr);
    openssl::ossl_param_ptr params(paramPtr);
    for (;paramPtr->key; ++paramPtr) {
        if (paramPtr->data_type == OSSL_PARAM_OCTET_STRING && std::strcmp(paramPtr->key, "pub") == 0) {
            const uint8_t idlen = paramPtr->data_size;
            encoded += idlen;
            encoded.resize(encoded.size() + idlen);
            std::size_t len = 0;
            auto data = reinterpret_cast<void*>(encoded.data() + encoded.size() - idlen);
            OSSL_PARAM_get_octet_string(paramPtr, &data, paramPtr->data_size, &len);
        }
    }

    encoded += "TODO";
    qDebug() << encoded.toHex();
    // TODO encrypted message

    return encoded;
}
