/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notifier.h"

#include "contentencryptor_p.h"
#include "vapid_p.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

using namespace KUnifiedPush;

Notifier::Notifier(QObject *parent)
    : QObject(parent)
{
}

Notifier::~Notifier() = default;

void Notifier::submit(const QByteArray &msg, QNetworkAccessManager *nam)
{
    assert(nam);

    QNetworkRequest req(m_endpoint);
    req.setRawHeader("TTL", QByteArray::number(m_ttl.count()));

    QByteArray content;
    if (!m_userAgentPublicKey.isEmpty() && !m_authSecret.isEmpty()) {
        ContentEncryptor encryptor(m_userAgentPublicKey, m_authSecret);
        content = encryptor.encrypt(msg);
        req.setRawHeader("Content-Encoding", "aes128gcm");
    } else {
        content = msg;
    }

    if (!m_vapidPublicKey.isEmpty() && !m_vapidPrivateKey.isEmpty()) {
        Vapid vapid(m_vapidPublicKey, m_vapidPrivateKey);
        vapid.setContact(m_contact);
        req.setRawHeader("Authorization", vapid.authorization(m_endpoint));
    }

    // prevent ntfy from turning encrypted content into attachments
    // (attachments aren't enabled by default in ntfy)
    req.setRawHeader("x-unifiedpush", "1");

#if 0
    for (const auto &hdr : req.rawHeaderList()) {
        qDebug() << hdr << req.rawHeader(hdr);
    }
#endif

    auto reply = nam->post(req, content);
    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        Q_EMIT finished(reply);
    });
}

#include "moc_notifier.cpp"
