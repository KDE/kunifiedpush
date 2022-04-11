/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CLIENT_H
#define KUNIFIEDPUSH_CLIENT_H

#include <QString>

class QSettings;
class OrgUnifiedpushConnector1Interface;

namespace KUnifiedPush {

/** Information about a registered client */
class Client
{
public:
    void store(QSettings &settings) const;
    static Client load(const QString &token, QSettings &settings);

    void activate() const;
    OrgUnifiedpushConnector1Interface connector() const;

    QString serviceName;
    QString token;
    QString remoteId;
    QString endpoint;
};

}

#endif // KUNIFIEDPUSH_CLIENT_H
