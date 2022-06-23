/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_COMMAND_H
#define KUNIFIEDPUSH_COMMAND_H

#include "client.h"

#include <QDBusMessage>

namespace KUnifiedPush {

/** Distributor command queue entries. */
class Command
{
public:
    enum Type {
        NoCommand,
        Register,
        Unregister,  ///< unregistration requested by client
        ForceUnregister, ///< unregistration triggered by distributor
        Connect,
        Disconnect,
        ChangePushProvider
    } type = NoCommand;
    Client client;
    QDBusMessage reply;
    QString pushProvider;
};

}

#endif // KUNIFIEDPUSH_COMMAND_H
