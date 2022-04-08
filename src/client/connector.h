/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTOR_H
#define KUNIFIEDPUSH_CONNECTOR_H

#include "kunifiedpush_export.h"

#include <QObject>

#include <memory>

/** Client-side integration with UnifiedPush. */
namespace KUnifiedPush {

class ConnectorPrivate;

/** Client connector to UnifiedPush.
 *  Registers with a local UnifedPush distributor if present
 *  and obtains an endpoint that can be used for push notifications.
 */
class KUNIFIEDPUSH_EXPORT Connector : public QObject
{
    Q_OBJECT
public:
    explicit Connector(QObject *parent = nullptr);
    ~Connector();

Q_SIGNALS:
    // TODO this is supposed to be a byte array, but gotify deviates from the spec...
    void messageReceived(const QString &msg);

private:
    ConnectorPrivate *d;
};

}

#endif
