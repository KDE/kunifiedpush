/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "nextcloudauthenticator.h"

#include <QDebug>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

NextcloudAuthenticator::NextcloudAuthenticator(QObject *parent)
    : QObject(parent)
{
}

NextcloudAuthenticator::~NextcloudAuthenticator() = default;

void NextcloudAuthenticator::setNetworkAccessManager(QNetworkAccessManager *nam)
{
    m_nam = nam;
}

void NextcloudAuthenticator::authenticate(const QUrl &baseUrl, const QString &appName)
{
    Q_ASSERT(m_nam);

    QUrl login2Url(baseUrl);
    login2Url.setPath(login2Url.path() + QLatin1String("/index.php/login/v2"));

    QNetworkRequest req(login2Url);
    req.setHeader(QNetworkRequest::UserAgentHeader, appName);
    auto reply = m_nam->post(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { post1Finished(reply); });
}

void NextcloudAuthenticator::post1Finished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << reply->errorString();
        return;
    }

    const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
    const auto loginUrl = QUrl(obj.value(QLatin1String("login")).toString());
    QDesktopServices::openUrl(loginUrl);

    const auto pollObj = obj.value(QLatin1String("poll")).toObject();
    m_pollEndpoint = QUrl(pollObj.value(QLatin1String("endpoint")).toString());
    m_pollToken = "token=" + pollObj.value(QLatin1String("token")).toString().toUtf8();

    QTimer::singleShot(std::chrono::seconds(5), Qt::VeryCoarseTimer, this, &NextcloudAuthenticator::login2Poll);
}

void NextcloudAuthenticator::login2Poll()
{
    QNetworkRequest req(m_pollEndpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/x-www-form-urlencoded"));
    auto reply = m_nam->post(req, m_pollToken);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            QTimer::singleShot(std::chrono::seconds(5), Qt::VeryCoarseTimer, this, &NextcloudAuthenticator::login2Poll);
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << reply->errorString();
            return;
        }

        const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        Q_EMIT authenticated(obj.value(QLatin1String("loginName")).toString(), obj.value(QLatin1String("appPassword")).toString());
    });
}
