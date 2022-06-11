/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCMPUSHNOTIFICATIONS_H
#define KCMPUSHNOTIFICATIONS_H

#include <KQuickAddons/ConfigModule>

/** KCM to configure push notifications. */
class KCMPushNotifications : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(QString distributorId READ distributorId CONSTANT)
public:
    explicit KCMPushNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMPushNotifications();

    QString distributorId() const;
};

#endif // KCMPUSHNOTIFICATIONS_H
