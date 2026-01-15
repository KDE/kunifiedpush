/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCMPUSHNOTIFICATIONS_H
#define KCMPUSHNOTIFICATIONS_H

#include "clientmodel.h"

#include <KQuickConfigModule>

#include <QDBusServiceWatcher>
#include <QNetworkAccessManager>

#include <memory>

class NextcloudAuthenticator;
class OrgKdeKunifiedpushManagementInterface;
class SelfTest;

/*!
 * \class KCMPushNotifications
 * \inmodule KUnifiedPush
 * \brief KCM to configure push notifications.
 */
class KCMPushNotifications : public KQuickConfigModule
{
    Q_OBJECT
    /*!
     * \property KCMPushNotifications::hasDistributor
     */
    Q_PROPERTY(bool hasDistributor READ hasDistributor NOTIFY distributorChanged)
    /*!
     * \property KCMPushNotifications::hasKDEDistributor
     */
    Q_PROPERTY(bool hasKDEDistributor READ hasKDEDistributor NOTIFY distributorChanged)
    /*!
     * \property KCMPushNotifications::distributorStatus
     */
    Q_PROPERTY(int distributorStatus READ distributorStatus NOTIFY distributorStatusChanged)
    /*!
     * \property KCMPushNotifications::distributorErrorMessage
     */
    Q_PROPERTY(QString distributorErrorMessage READ distributorErrorMessage NOTIFY distributorErrorMessageChanged)
    /*!
     * \property KCMPushNotifications::pushProviderId
     */
    Q_PROPERTY(QString pushProviderId READ pushProviderId NOTIFY pushProviderChanged)
    /*!
     * \property KCMPushNotifications::clientModel
     */
    Q_PROPERTY(ClientModel* clientModel READ clientModel CONSTANT)
    /*!
     * \property KCMPushNotifications::selfTest
     */
    Q_PROPERTY(SelfTest* selfTest READ selfTest CONSTANT)
public:
    /*!
     */
    explicit KCMPushNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    /*!
     */
    ~KCMPushNotifications();

    /*!
     */
    [[nodiscard]] bool hasDistributor() const;
    /*!
     */
    [[nodiscard]] bool hasKDEDistributor() const;
    /*!
     */
    [[nodiscard]] int distributorStatus() const;
    /*!
     */
    [[nodiscard]] QString distributorErrorMessage() const;
    /*!
     */
    [[nodiscard]] QString pushProviderId() const;
    /*!
     */
    [[nodiscard]] ClientModel *clientModel() const;
    /*!
     */
    [[nodiscard]] SelfTest *selfTest() const;

    /*!
     */
    Q_INVOKABLE [[nodiscard]] QVariantMap pushProviderConfiguration(const QString &pushProviderId) const;

    /*!
     */
    Q_INVOKABLE void nextcloudAuthenticate(const QUrl &url);

    /*!
     */
    void save() override;

public Q_SLOTS:
    /*!
     */
    void forceUnregister(const QString &token);
    /*!
     */
    void setPushProviderConfiguration(const QString &pushProviderId, const QVariantMap &config);

Q_SIGNALS:
    void distributorChanged();
    void distributorStatusChanged();
    void distributorErrorMessageChanged();
    void pushProviderChanged();

    void nextcloudAuthenticated(const QString &loginName, const QString &appPassword);

    void saveRequested();

private:
    OrgKdeKunifiedpushManagementInterface *m_mgmtIface = nullptr;
    ClientModel *m_clientModel = nullptr;
    SelfTest *m_selfTest = nullptr;
    QDBusServiceWatcher m_serviceWatcher;

    QNetworkAccessManager m_nam;
    std::unique_ptr<NextcloudAuthenticator> m_nextcloudAuthenticator;
};

#endif // KCMPUSHNOTIFICATIONS_H
