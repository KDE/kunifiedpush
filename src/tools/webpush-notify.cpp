/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kunifiedpush_version.h"

#include "../notifier/notifier.h"

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

using namespace Qt::Literals;

int main(int argc, char **argv)
{
    QCoreApplication::setApplicationName(u"WebPush Notifier"_s);
    QCoreApplication::setOrganizationName(u"KDE"_s);
    QCoreApplication::setOrganizationDomain(u"kde.org"_s);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(KUNIFIEDPUSH_VERSION_STRING));
    QCoreApplication app(argc, argv);

    QCommandLineOption endpointOpt(u"endpoint"_s, u"Push endpoint URL"_s, u"url"_s);
    QCommandLineOption uaPublicKeyOpt(u"user-agent-public-key"_s, u"User agent public key, Base64 URL encoded"_s, u"key"_s);
    QCommandLineOption authSecrectOpt(u"auth-secret"_s, u"User agent auth secrect, Base64 URL encoded"_s, u"secret"_s);
    QCommandLineOption vapidPublicKeyOpt(u"vapid-public-key"_s, u"VAPID public key, Base64 URL encoded"_s, u"key"_s);
    QCommandLineOption vapidPrivateKeyOpt(u"vapid-private-key"_s, u"VAPID private key, Base64 URL encoded"_s, u"key"_s);
    QCommandLineOption ttlOpt(u"ttl"_s, u"Time-to-live of the push message, in seconds"_s, u"seconds"_s);
    QCommandLineOption urgencyOpt(u"urgency"_s, u"Urgency of the message [very-low, low, normal, high]"_s, u"urgency"_s);

    QCommandLineParser parser;
    parser.addOptions({ endpointOpt, uaPublicKeyOpt, authSecrectOpt, vapidPublicKeyOpt, vapidPrivateKeyOpt, ttlOpt });
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    QNetworkAccessManager nam;
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    nam.setStrictTransportSecurityEnabled(true);
    nam.enableStrictTransportSecurityStore(true, QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1StringView("/hsts/"));

    KUnifiedPush::Notifier notifier;
    notifier.setEndpoint(QUrl(parser.value(endpointOpt)));
    notifier.setUserAgentPublicKey(QByteArray::fromBase64(parser.value(uaPublicKeyOpt).toUtf8(), QByteArray::Base64UrlEncoding));
    notifier.setAuthSecret(QByteArray::fromBase64(parser.value(authSecrectOpt).toUtf8(), QByteArray::Base64UrlEncoding));
    notifier.setVapidPublicKey(QByteArray::fromBase64(parser.value(vapidPublicKeyOpt).toUtf8(), QByteArray::Base64UrlEncoding));
    notifier.setVapidPrivateKey(QByteArray::fromBase64(parser.value(vapidPrivateKeyOpt).toUtf8(), QByteArray::Base64UrlEncoding));
    if (parser.isSet(ttlOpt)) {
        notifier.setTtl(std::chrono::seconds(std::min(parser.value(ttlOpt).toInt(), 5)));
    }
    if (const auto urgency = parser.value(urgencyOpt); !urgency.isEmpty()) {
        if (urgency == "very-low"_L1) {
            notifier.setUrgency(KUnifiedPush::Urgency::VeryLow);
        } else if (urgency == "low"_L1) {
            notifier.setUrgency(KUnifiedPush::Urgency::Low);
        } else  if (urgency == "high"_L1) {
            notifier.setUrgency(KUnifiedPush::Urgency::High);
        }
    }

    QFile msg;
    msg.open(stdin, QFile::ReadOnly);
    notifier.submit(msg.readAll(), &nam);
    QObject::connect(&notifier, &KUnifiedPush::Notifier::finished, &app, [](QNetworkReply *reply) {
        // TODO should Notifier parse results?
        qDebug() << reply->errorString() << reply->readAll();
        QCoreApplication::quit();
    });

    return QCoreApplication::exec();
}
