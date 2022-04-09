/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractpushprovider.h"

#include <QNetworkAccessManager>
#include <QStandardPaths>

using namespace KUnifiedPush;

AbstractPushProvider::AbstractPushProvider(QObject *parent)
    : QObject(parent)
{
}

QNetworkAccessManager* AbstractPushProvider::nam()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
        m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
        m_nam->setStrictTransportSecurityEnabled(true);
        m_nam->enableStrictTransportSecurityStore(true, QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/org.kde.kunifiedpush/hsts/"));
    }
    return m_nam;
}
