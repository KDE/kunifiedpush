/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CLIENT_H
#define KUNIFIEDPUSH_CLIENT_H

#include <QString>

namespace KUnifiedPush {

/** Information about a registered client */
class Client
{
public:
    QString serviceName;
    QString token;
    QString remoteId;
};

}

#endif // KUNIFIEDPUSH_CLIENT_H
