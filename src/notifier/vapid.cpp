/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "vapid_p.h"

#include "../shared/eckey_p.h"
#include "../shared/opensslpp_p.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <openssl/err.h>

using namespace Qt::Literals;
using namespace KUnifiedPush;

Vapid::Vapid(const QByteArray &publicKey, const QByteArray &privateKey)
    : m_publicKey(publicKey)
    , m_privateKey(privateKey)
{
}

Vapid::~Vapid() = default;

QByteArray Vapid::authorization(const QUrl &endpoint) const
{
    const auto header = QByteArray(R"({"typ":"JWT","alg":"ES256"})").toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    auto url(endpoint);
    url.setUserName({});
    url.setPassword({});
    url.setPath({});

    const auto claimObj = QJsonObject({
        {"aud"_L1, url.toString()},
        {"exp"_L1, QDateTime::currentDateTimeUtc().addDuration(std::chrono::hours(12)).toSecsSinceEpoch()},
        {"sub"_L1, m_contact.isEmpty() ? u"https://invent.kde.org/libraries/kunifiedpush"_s : m_contact}
    });
    const auto claim = QJsonDocument(claimObj).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    const QByteArray token = header + '.' + claim;

    const auto key = ECKey::load(m_publicKey, m_privateKey);
    const openssl::evp_md_ctx_ptr mdCtx(EVP_MD_CTX_new());
    EVP_DigestSignInit(mdCtx.get(), nullptr, EVP_sha256(), nullptr, key.get());

    std::size_t len;
    if (EVP_DigestSign(mdCtx.get(), nullptr, &len, reinterpret_cast<const uint8_t*>(token.constData()), token.size()) != 1) {
        qWarning() << "Failed to determine VAPID JWT signature size" << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }

    QByteArray derSignature((qsizetype)len, Qt::Uninitialized);
    if (EVP_DigestSign(mdCtx.get(), reinterpret_cast<uint8_t*>(derSignature.data()), &len, reinterpret_cast<const uint8_t*>(token.constData()), token.size()) != 1) {
        qWarning() << "Failed to sign VAPID JWT" << ERR_error_string(ERR_get_error(), nullptr);
        return {};
    }

    // convert DER signature to raw r || s format
    auto *derSigIt = reinterpret_cast<const uint8_t*>(derSignature.constData());
    openssl::ecdsa_sig_ptr ecdsaSig(d2i_ECDSA_SIG(nullptr, &derSigIt, derSignature.size()));

    QByteArray rawSignature(64, '\0');
    const auto r = ECDSA_SIG_get0_r(ecdsaSig.get());
    const auto rSize = BN_num_bytes(r);
    if (rSize > 32) {
        qWarning() << "Invalid r size" << rSize;
        return {};
    }
    BN_bn2bin(r, reinterpret_cast<uint8_t*>(rawSignature.data() + 32 - rSize));

    const auto s = ECDSA_SIG_get0_s(ecdsaSig.get());
    const auto sSize = BN_num_bytes(s);
    if (rSize > 32) {
        qWarning() << "Invalid s size" << sSize;
        return {};
    }
    BN_bn2bin(s, reinterpret_cast<uint8_t*>(rawSignature.data() + 64 - sSize));

    // assemble HTTP header argument
    return "vapid t=" + token + '.' + rawSignature.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
         + ",k=" + m_publicKey.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}
