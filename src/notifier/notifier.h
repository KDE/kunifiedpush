/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NOTIFIER_NOTIFIER_H
#define KUNIFIEDPUSH_NOTIFIER_NOTIFIER_H

#include "../shared/urgency_p.h"

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace KUnifiedPush {

/** Push notification submission job.
 *  This is only used for testing/tooling.
 */
class Notifier : public QObject
{
    Q_OBJECT
public:
    explicit Notifier(QObject *parent = nullptr);
    ~Notifier();

    inline void setEndpoint(const QUrl &url) { m_endpoint = url; }
    inline void setTtl(std::chrono::seconds ttl) { m_ttl = ttl; }
    inline void setUserAgentPublicKey(const QByteArray &uaPublicKey) { m_userAgentPublicKey = uaPublicKey; }
    inline void setAuthSecret(const QByteArray &authSecret) { m_authSecret = authSecret; }
    inline void setContact(const QString &contact) { m_contact = contact; }
    inline void setVapidPublicKey(const QByteArray &vapidPublicKey) { m_vapidPublicKey = vapidPublicKey; }
    inline void setVapidPrivateKey(const QByteArray &vapidPrivateKey) { m_vapidPrivateKey = vapidPrivateKey; }
    inline void setUrgency(Urgency urgency) { m_urgency = urgency; }

    /** Submit push message with content @p msg. */
    void submit(const QByteArray &msg, QNetworkAccessManager *nam);

Q_SIGNALS:
    void finished(QNetworkReply *reply);

private:
    QUrl m_endpoint;
    std::chrono::seconds m_ttl = std::chrono::hours(1);
    Urgency m_urgency = DefaultUrgency;

    // content encryption
    QByteArray m_userAgentPublicKey;
    QByteArray m_authSecret;

    // VAPID
    QString m_contact;
    QByteArray m_vapidPublicKey;
    QByteArray m_vapidPrivateKey;
};

}

#endif
