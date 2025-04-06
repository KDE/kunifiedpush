/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcmpushnotifications.h"
#include "managementinterface.h"
#include "nextcloudauthenticator.h"
#include "selftest.h"

#include "../shared/clientinfo_p.h"
#include "../shared/connectorutils_p.h"
#include "../shared/distributorstatus_p.h"
#include "../shared/unifiedpush-constants.h"

#include <KPluginFactory>

#include <QHostInfo>
#include <QStandardPaths>

K_PLUGIN_CLASS_WITH_JSON(KCMPushNotifications, "kcm_push_notifications.json")

using namespace KUnifiedPush;

KCMPushNotifications::KCMPushNotifications(QObject *parent, const KPluginMetaData &data, [[maybe_unused]] const QVariantList &args)
    : KQuickConfigModule(parent, data)
{
    qDBusRegisterMetaType<KUnifiedPush::ClientInfo>();
    qDBusRegisterMetaType<QList<KUnifiedPush::ClientInfo>>();

    m_nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_nam.setStrictTransportSecurityEnabled(true);
    m_nam.enableStrictTransportSecurityStore(true, QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/org.kde.kunifiedpush/hsts/"));

    // TODO do this only when we are using the KDE distributor
    m_mgmtIface = new OrgKdeKunifiedpushManagementInterface(KDE_DISTRIBUTOR_SERVICE_NAME, KDE_DISTRIBUTOR_MANAGEMENT_PATH, QDBusConnection::sessionBus(), this);
    connect(m_mgmtIface, &OrgKdeKunifiedpushManagementInterface::statusChanged, this, &KCMPushNotifications::distributorStatusChanged);
    connect(m_mgmtIface, &OrgKdeKunifiedpushManagementInterface::errorMessageChanged, this, &KCMPushNotifications::distributorErrorMessageChanged);
    connect(m_mgmtIface, &OrgKdeKunifiedpushManagementInterface::pushProviderChanged, this, &KCMPushNotifications::pushProviderChanged);

    m_clientModel = new ClientModel(m_mgmtIface, this);
    connect(this, &KCMPushNotifications::distributorChanged, m_clientModel, &ClientModel::reload);

    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &KCMPushNotifications::distributorChanged);
    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &KCMPushNotifications::distributorChanged);
    m_serviceWatcher.setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    m_serviceWatcher.addWatchedService(UP_DISTRIBUTOR_SERVICE_NAME_FILTER);
    connect(this, &KCMPushNotifications::distributorChanged, this, &KCMPushNotifications::distributorStatusChanged);
    connect(this, &KCMPushNotifications::distributorChanged, this, &KCMPushNotifications::pushProviderChanged);

    m_selfTest = new SelfTest(this);
    m_selfTest->setNetworkAccessManager(&m_nam);

    qmlRegisterUncreatableMetaObject(DistributorStatus::staticMetaObject, "org.kde.kunifiedpush.kcm", 1, 0, "DistributorStatus", {});
    qmlRegisterUncreatableMetaObject(SelfTest::staticMetaObject, "org.kde.kunifiedpush.kcm", 1, 0, "SelfTest", {});
}

KCMPushNotifications::~KCMPushNotifications() = default;

bool KCMPushNotifications::hasDistributor() const
{
    return !ConnectorUtils::selectDistributor().isEmpty();
}

bool KCMPushNotifications::hasKDEDistributor() const
{
    return ConnectorUtils::selectDistributor() == KDE_DISTRIBUTOR_SERVICE_NAME;
}

int KCMPushNotifications::distributorStatus() const
{
    return m_mgmtIface->status();
}

QString KCMPushNotifications::distributorErrorMessage() const
{
    return m_mgmtIface->errorMessage();
}

QString KCMPushNotifications::pushProviderId() const
{
    return m_mgmtIface->pushProviderId();
}

ClientModel* KCMPushNotifications::clientModel() const
{
    return m_clientModel;
}

SelfTest* KCMPushNotifications::selfTest() const
{
    return m_selfTest;
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

void KCMPushNotifications::nextcloudAuthenticate(const QUrl &url)
{
    if (!url.isValid()) {
        return;
    }

    m_nextcloudAuthenticator.reset(new NextcloudAuthenticator);
    connect(m_nextcloudAuthenticator.get(), &NextcloudAuthenticator::authenticated, this, &KCMPushNotifications::nextcloudAuthenticated);
    m_nextcloudAuthenticator->setNetworkAccessManager(&m_nam);
    m_nextcloudAuthenticator->authenticate(url, QStringLiteral("KUnifiedPush Distributor (%1)").arg(QHostInfo::localHostName()));
}

void KCMPushNotifications::save()
{
    Q_EMIT saveRequested();
    KQuickConfigModule::save();
    m_selfTest->reset();
}

#include "kcmpushnotifications.moc"
#include "../shared/moc_distributorstatus_p.cpp"

#include "moc_kcmpushnotifications.cpp"
