/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef NEXTCLOUDAUTHENTICATOR_H
#define NEXTCLOUDAUTHENTICATOR_H

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

/*!
 * \class NextcloudAuthenticator
 * \inmodule KUnifiedPush
 * Implementation of Nextcloud's LoginFlow v2.
 * \sa https://docs.nextcloud.com/server/latest/developer_manual/client_apis/LoginFlow/index.html#login-flow-v2
 */
class NextcloudAuthenticator : public QObject
{
    Q_OBJECT
public:
    /*!
     */
    explicit NextcloudAuthenticator(QObject *parent = nullptr);
    /*!
     */
    ~NextcloudAuthenticator();

    /*!
     */
    void setNetworkAccessManager(QNetworkAccessManager *nam);
    /*!
     */
    void authenticate(const QUrl &baseUrl, const QString &appName);

Q_SIGNALS:
    void authenticated(const QString &loginName, const QString &appPassword);

private:
    void post1Finished(QNetworkReply *reply);
    void login2Poll();

    QNetworkAccessManager *m_nam = nullptr;
    QUrl m_pollEndpoint;
    QByteArray m_pollToken;
};

#endif // NEXTCLOUDAUTHENTICATOR_H
