/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcmpushnotifications.h"

#include "../shared/connectorutils_p.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(KCMPushNotifications, "kcm_push_notifications.json")

using namespace KUnifiedPush;

KCMPushNotifications::KCMPushNotifications(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, data, args)
{
}

KCMPushNotifications::~KCMPushNotifications() = default;

QString KCMPushNotifications::distributorId() const
{
    return ConnectorUtils::selectDistributor();
}

#include "kcmpushnotifications.moc"
