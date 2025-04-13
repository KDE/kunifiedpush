/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractpushprovider.h"

#include <QNetworkAccessManager>
#include <QStandardPaths>

using namespace KUnifiedPush;

AbstractPushProvider::AbstractPushProvider(QLatin1StringView providerId, QObject *parent)
    : QObject(parent)
    , m_providerId(providerId)
{
}

AbstractPushProvider::~AbstractPushProvider() = default;

void AbstractPushProvider::resetSettings([[maybe_unused]] QSettings &settings)
{
}

void AbstractPushProvider::acknowledgeMessage(const Client &client, const QString &messageIdentifier)
{
    Q_EMIT messageAcknowledged(client, messageIdentifier);
}

void AbstractPushProvider::changeUrgency(Urgency urgency)
{
    if (m_currentUrgency == urgency) {
        Q_EMIT urgencyChanged();
        return;
    }

    doChangeUrgency(urgency);
}

void AbstractPushProvider::doChangeUrgency(Urgency urgency)
{
    m_currentUrgency = urgency;
    Q_EMIT urgencyChanged();
}

QLatin1StringView AbstractPushProvider::providerId() const
{
    return m_providerId;
}

Urgency AbstractPushProvider::urgency() const
{
    return m_currentUrgency;
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

#include "moc_abstractpushprovider.cpp"
