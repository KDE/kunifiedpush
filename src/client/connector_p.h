/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTOR_P_H
#define KUNIFIEDPUSH_CONNECTOR_P_H

#include <QObject>

class OrgUnifiedpushDistributor1Interface;

namespace KUnifiedPush {
class Connector;
class ConnectorPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ConnectorPrivate(Connector *qq);

    void Message(const QString &token, const QString &message, const QString &messageIdentifier);
    void NewEndpoint(const QString &token, const QString &endpoint);
    void Unregister(const QString &token);

    void selectDistributor();

    Connector *q = nullptr;
    QString m_token;
    OrgUnifiedpushDistributor1Interface *m_distributor = nullptr;
};
}

#endif
