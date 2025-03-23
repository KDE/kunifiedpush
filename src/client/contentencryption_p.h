/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONTENTENCRYPTION_H
#define KUNIFIEDPUSH_CONTENTENCRYPTION_H

#include "kunifiedpush_export.h"

#include <memory>

class QByteArray;

namespace KUnifiedPush {

class ContentEncryptionPrivate;

/** Client-side content encryption methods.
 *
 *  Contains:
 *  - client-side key and auth secret generation
 *  - message decryption
 *
 *  @see RFC 8291
 *
 *  @internal only exported for unit tests
 */
class KUNIFIEDPUSH_EXPORT ContentEncryption
{
public:
    ContentEncryption();
    ContentEncryption(ContentEncryption &&) noexcept;
    /**
     * @param publicKey The user-agent public key.
     * @param privateKey The user-agent private key.
     * @param authSecret Authentication secret.
     */
    explicit ContentEncryption(const QByteArray &publicKey, const QByteArray &privateKey, const QByteArray &authSecret);
    ~ContentEncryption();

    ContentEncryption& operator=(ContentEncryption &&) noexcept;

    [[nodiscard]] QByteArray publicKey() const;
    [[nodiscard]] QByteArray privateKey() const;
    [[nodiscard]] QByteArray authSecret() const;

    /** Has keys and authentication secrets set so that decryption is possible. */
    [[nodiscard]] bool hasKeys() const;

    /** Decrypt a RFC 8291 message. */
    [[nodiscard]] QByteArray decrypt(const QByteArray &encrypted) const;

    /** Generate a new set of keys and authentication secrets. */
    [[nodiscard]] static ContentEncryption generateKeys();

private:
    std::unique_ptr<ContentEncryptionPrivate> d;
};

}

#endif
