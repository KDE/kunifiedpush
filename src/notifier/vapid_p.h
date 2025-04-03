/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NOTIFIER_VAPID_P_H
#define KUNIFIEDPUSH_NOTIFIER_VAPID_P_H

#include <QByteArray>
#include <QString>

class QUrl;

namespace KUnifiedPush {

/** RFC 8292 Voluntary Application Server Identification (VAPID) for Web Push. */
class Vapid
{
public:
    explicit Vapid(const QByteArray &publicKey, const QByteArray &privateKey);
    ~Vapid();

    inline void setContact(const QString &contact) { m_contact = contact; }

    /** Generate VAPID Authorization header for push messages towards @p endpoint. */
    [[nodiscard]] QByteArray authorization(const QUrl &endpoint) const;

private:
    QString m_contact;

    QByteArray m_publicKey;
    QByteArray m_privateKey;
};

}

#endif
