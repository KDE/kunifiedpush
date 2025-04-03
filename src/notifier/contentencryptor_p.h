/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NOTIFIER_CONTENTENCRYPTOR_P_H
#define KUNIFIEDPUSH_NOTIFIER_CONTENTENCRYPTOR_P_H

#include <QByteArray>

namespace KUnifiedPush {

/** RFC 8291 push message content encryption. */
class ContentEncryptor
{
public:
    explicit ContentEncryptor(const QByteArray &userAgentPublicKey, const QByteArray &authSecret);
    ~ContentEncryptor();

    [[nodiscard]] QByteArray encrypt(const QByteArray &msg);

private:
    QByteArray m_userAgentPublicKey;
    QByteArray m_authSecret;

};

}

#endif
