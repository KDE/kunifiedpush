/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CLIENT_H
#define KUNIFIEDPUSH_CLIENT_H

#include <QMetaType>
#include <QString>

class QSettings;

namespace KUnifiedPush {

class Distributor;

/** Information about a registered client */
class Client
{
public:
    void store(QSettings &settings) const;
    static Client load(const QString &token, QSettings &settings);

    /** Contains all required information for a client. */
    [[nodiscard]] bool isValid() const;

    /** Activate client on D-Bus. */
    void activate() const;

    /** D-Bus UnifiedPush connector interface. */
    void message(Distributor *distributor, const QByteArray &message, const QString &messageIdentifier) const;
    void newEndpoint() const;
    void unregistered(bool isConfirmation) const;

    QString serviceName;
    QString token;
    QString remoteId;
    QString endpoint;
    QString description;
    // UnifiedPush protocol version
    enum class UnifiedPushVersion {
        v1 = 1,
        v2 = 2,
    };

    UnifiedPushVersion version = UnifiedPushVersion::v1;
};

}

Q_DECLARE_METATYPE(KUnifiedPush::Client)

#endif // KUNIFIEDPUSH_CLIENT_H
