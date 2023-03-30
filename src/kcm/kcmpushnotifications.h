/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCMPUSHNOTIFICATIONS_H
#define KCMPUSHNOTIFICATIONS_H

#include "clientmodel.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KQuickAddons/ConfigModule>
#else
#include <KQuickConfigModule>
#endif

#include <QDBusServiceWatcher>
#include <QNetworkAccessManager>

#include <memory>

class NextcloudAuthenticator;
class OrgKdeKunifiedpushManagementInterface;

/** KCM to configure push notifications. */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class KCMPushNotifications : public KQuickAddons::ConfigModule
#else
class KCMPushNotifications : public KQuickConfigModule
#endif
{
    Q_OBJECT
    Q_PROPERTY(bool hasDistributor READ hasDistributor NOTIFY distributorChanged)
    Q_PROPERTY(bool hasKDEDistributor READ hasKDEDistributor NOTIFY distributorChanged)
    Q_PROPERTY(int distributorStatus READ distributorStatus NOTIFY distributorStatusChanged)
    Q_PROPERTY(QString pushProviderId READ pushProviderId NOTIFY pushProviderChanged)
    Q_PROPERTY(ClientModel* clientModel READ clientModel CONSTANT)
public:
    explicit KCMPushNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMPushNotifications();

    bool hasDistributor() const;
    bool hasKDEDistributor() const;
    int distributorStatus() const;
    QString pushProviderId() const;
    ClientModel *clientModel() const;

    Q_INVOKABLE QVariantMap pushProviderConfiguration(const QString &pushProviderId) const;

    Q_INVOKABLE void nextcloudAuthenticate(const QUrl &url);

    void save() override;

public Q_SLOTS:
    void forceUnregister(const QString &token);
    void setPushProviderConfiguration(const QString &pushProviderId, const QVariantMap &config);

Q_SIGNALS:
    void distributorChanged();
    void distributorStatusChanged();
    void pushProviderChanged();

    void nextcloudAuthenticated(const QString &loginName, const QString &appPassword);

    void saveRequested();

private:
    OrgKdeKunifiedpushManagementInterface *m_mgmtIface = nullptr;
    ClientModel *m_clientModel = nullptr;
    QDBusServiceWatcher m_serviceWatcher;

    QNetworkAccessManager m_nam;
    std::unique_ptr<NextcloudAuthenticator> m_nextcloudAuthenticator;
};

#endif // KCMPUSHNOTIFICATIONS_H
