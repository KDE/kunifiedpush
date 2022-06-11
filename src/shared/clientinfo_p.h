/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CLIENT_INFO_P_H
#define KUNIFIEDPUSH_CLIENT_INFO_P_H

#include <QDBusArgument>
#include <QString>

namespace KUnifiedPush
{

struct ClientInfo {
    QString token;
    QString serviceName;
    QString description;
};

}
Q_DECLARE_METATYPE(KUnifiedPush::ClientInfo)

inline QDBusArgument &operator<<(QDBusArgument &argument, const KUnifiedPush::ClientInfo &client)
{
    argument.beginStructure();
    argument << client.token << client.serviceName << client.description;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, KUnifiedPush::ClientInfo &client)
{
    argument.beginStructure();
    argument >> client.token >> client.serviceName >> client.description;
    argument.endStructure();
    return argument;
}

#endif
