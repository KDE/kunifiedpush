/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connectorutils_p.h"
#include "unifiedpush-constants.h"

#ifndef Q_OS_ANDROID
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#else
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtAndroid>
#include <QAndroidJniObject>
#else
#include <QCoreApplication>
#include <QJniObject>
using QAndroidJniObject = QJniObject;
#endif
#endif
#include <QStringList>

using namespace KUnifiedPush;

QString ConnectorUtils::selectDistributor()
{
#ifndef Q_OS_ANDROID
    return selectDistributor(QDBusConnection::sessionBus().interface()->registeredServiceNames());
#else
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject context = QtAndroid::androidContext();
#else
    QJniObject context = QNativeInterface::QAndroidApplication::context();
#endif
    return QAndroidJniObject::callStaticObjectMethod("org/kde/kunifiedpush/Distributor", "selectDistributor", "(Landroid/content/Context;)Ljava/lang/String;", context.object()).toString();
#endif
}

QString ConnectorUtils::selectDistributor(QStringList &&services)
{
    services.erase(std::remove_if(services.begin(), services.end(), [](const auto &s) { return !s.startsWith(QLatin1String(UP_DISTRIBUTOR_SERVICE_NAME_PREFIX)); }), services.end());
    std::sort(services.begin(), services.end()); // make sure results are reproducible

    if (services.isEmpty()) {
        return {};
    }

    // check if one specific distributor was requested
    const auto requestedDist = QString::fromUtf8(qgetenv("UNIFIEDPUSH_DISTRIBUTOR"));
    if (!requestedDist.isEmpty()) {
        const QString distServiceName = QLatin1String(UP_DISTRIBUTOR_SERVICE_NAME_PREFIX) + requestedDist;
        if (!services.contains(distServiceName)) {
            return {};
        } else {
            return distServiceName;
        }
        return {};
    }

    // prefer our own distributor over 3rdparty ones
    const auto it = std::find(services.begin(), services.end(), QLatin1String(KDE_DISTRIBUTOR_SERVICE_NAME));
    if (it != services.end()) {
        return (*it);
    }

    // ... otherwise take a random one
    return services.at(0);
}
