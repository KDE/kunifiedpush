/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcmpushnotifications.h"
#include "managementinterface.h"

#include "../shared/clientinfo_p.h"
#include "../shared/connectorutils_p.h"
#include "../shared/distributorstatus_p.h"
#include "../shared/unifiedpush-constants.h"

#include <KPluginFactory>

#include <QDebug>

K_PLUGIN_CLASS_WITH_JSON(KCMPushNotifications, "kcm_push_notifications.json")

using namespace KUnifiedPush;

KCMPushNotifications::KCMPushNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, data, args)
{
    qDBusRegisterMetaType<KUnifiedPush::ClientInfo>();
    qDBusRegisterMetaType<QList<KUnifiedPush::ClientInfo>>();

    // TODO do this only when we are using the KDE distributor
    m_mgmtIface = new OrgKdeKunifiedpushManagementInterface(QLatin1String(KDE_DISTRIBUTOR_SERVICE_NAME), QLatin1String(KDE_DISTRIBUTOR_MANAGEMENT_PATH), QDBusConnection::sessionBus(), this);
    connect(m_mgmtIface, &OrgKdeKunifiedpushManagementInterface::statusChanged, this, &KCMPushNotifications::distributorStatusChanged);
    connect(m_mgmtIface, &OrgKdeKunifiedpushManagementInterface::pushProviderChanged, this, &KCMPushNotifications::pushProviderChanged);

    m_clientModel = new ClientModel(m_mgmtIface, this);
    connect(this, &KCMPushNotifications::distributorChanged, m_clientModel, &ClientModel::reload);

    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &KCMPushNotifications::distributorChanged);
    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &KCMPushNotifications::distributorChanged);
    m_serviceWatcher.setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    m_serviceWatcher.addWatchedService(QLatin1String(UP_DISTRIBUTOR_SERVICE_NAME_FILTER));
    connect(this, &KCMPushNotifications::distributorChanged, this, &KCMPushNotifications::distributorStatusChanged);
    connect(this, &KCMPushNotifications::distributorChanged, this, &KCMPushNotifications::pushProviderChanged);

    qmlRegisterUncreatableMetaObject(DistributorStatus::staticMetaObject, "org.kde.kunifiedpush.kcm", 1, 0, "DistributorStatus", {});
}

KCMPushNotifications::~KCMPushNotifications() = default;

bool KCMPushNotifications::hasDistributor() const
{
    return !ConnectorUtils::selectDistributor().isEmpty();
}

bool KCMPushNotifications::hasKDEDistributor() const
{
    return ConnectorUtils::selectDistributor() == QLatin1String(KDE_DISTRIBUTOR_SERVICE_NAME);
}

int KCMPushNotifications::distributorStatus() const
{
    return m_mgmtIface->status();
}

QString KCMPushNotifications::pushProviderId() const
{
    return m_mgmtIface->pushProviderId();
}

ClientModel* KCMPushNotifications::clientModel() const
{
    return m_clientModel;
}

QVariantMap KCMPushNotifications::pushProviderConfiguration(const QString &pushProviderId) const
{
    return m_mgmtIface->pushProviderConfiguration(pushProviderId);
}

void KCMPushNotifications::setPushProviderConfiguration(const QString &pushProviderId, const QVariantMap &config)
{
    m_mgmtIface->setPushProvider(pushProviderId, config);
}

void KCMPushNotifications::forceUnregister(const QString &token)
{
    m_mgmtIface->forceUnregisterClient(token);
}

void KCMPushNotifications::save()
{
    Q_EMIT saveRequested();
    KQuickAddons::ConfigModule::save();
}

#include "kcmpushnotifications.moc"
#include "../shared/moc_distributorstatus_p.cpp"
