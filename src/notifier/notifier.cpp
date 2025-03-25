/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notifier.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

using namespace KUnifiedPush;

Notifier::Notifier(QObject *parent)
    : QObject(parent)
{
}

Notifier::~Notifier() = default;

void Notifier::submit(QNetworkAccessManager *nam)
{
    assert(nam);

    QNetworkRequest req(m_endpoint);
    req.setRawHeader("TTL", QByteArray::number(m_ttl.count()));
    // TODO content encryption
    req.setRawHeader("Content-Encoding", "aes128gcm");
    // TODO VAPID
    auto reply = nam->post(req, m_message);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        Q_EMIT finished(reply);
    });
}

#include "moc_notifier.cpp"
