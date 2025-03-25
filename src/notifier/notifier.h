/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NOTIFIER_NOTIFIER_H
#define KUNIFIEDPUSH_NOTIFIER_NOTIFIER_H

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
    inline void setMessage(const QByteArray &msg) { m_message = msg; }
    inline void setTtl(std::chrono::seconds ttl) { m_ttl = ttl; }

    void submit(QNetworkAccessManager *nam);

Q_SIGNALS:
    void finished(QNetworkReply *reply);

private:
    QUrl m_endpoint;
    QByteArray m_message;
    std::chrono::seconds m_ttl;

    // content encryption
    QByteArray m_userAgentPublicKey;
    QByteArray m_authSecret;

    // VAPID
    // TODO
};

}

#endif
