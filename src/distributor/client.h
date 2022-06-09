/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CLIENT_H
#define KUNIFIEDPUSH_CLIENT_H

#include <QMetaType>
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

    /** Contains all required information for a client. */
    bool isValid() const;

    /** Activate client on D-Bus. */
    void activate() const;
    /** D-Bus UnifiedPush connector interface. */
    OrgUnifiedpushConnector1Interface connector() const;

    QString serviceName;
    QString token;
    QString remoteId;
    QString endpoint;
};

}

Q_DECLARE_METATYPE(KUnifiedPush::Client)

#endif // KUNIFIEDPUSH_CLIENT_H
