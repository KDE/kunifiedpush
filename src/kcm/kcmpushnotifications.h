/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCMPUSHNOTIFICATIONS_H
#define KCMPUSHNOTIFICATIONS_H

#include "clientmodel.h"

#include <KQuickAddons/ConfigModule>

#include <QDBusServiceWatcher>

class OrgKdeKunifiedpushManagementInterface;

/** KCM to configure push notifications. */
class KCMPushNotifications : public KQuickAddons::ConfigModule
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

    void save() override;

public Q_SLOTS:
    void forceUnregister(const QString &token);
    void setPushProviderConfiguration(const QString &pushProviderId, const QVariantMap &config);

Q_SIGNALS:
    void distributorChanged();
    void distributorStatusChanged();
    void pushProviderChanged();

    void saveRequested();

private:
    OrgKdeKunifiedpushManagementInterface *m_mgmtIface = nullptr;
    ClientModel *m_clientModel = nullptr;
    QDBusServiceWatcher m_serviceWatcher;
};

#endif // KCMPUSHNOTIFICATIONS_H
