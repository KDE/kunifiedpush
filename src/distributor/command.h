/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_COMMAND_H
#define KUNIFIEDPUSH_COMMAND_H

#include "client.h"

#include <QDBusMessage>

namespace KUnifiedPush {

/*!
 * \class KUnifiedPush::Command
 * \inmodule KUnifiedPush
 * \brief Distributor command queue entries.
 */
class Command
{
public:
    /*!
     * \enum KUnifiedPush::Command::Type
     * \value NoCommand
     * \value Register
     * \value Unregister
     *        Unregistration requested by client
     * \value ForceUnregister
     *        Unregistration triggered by distributor
     * \value SilentUnregister
     *        Unregistration for moving to a different push provider
     * \value Connect
     * \value Disconnect
     * \value ChangePushProvider
     * \value MessageAck
     * \value ChangeUrgency
     * \value Wait
     */
    enum Type {
        NoCommand,
        Register,
        Unregister,
        ForceUnregister,
        SilentUnregister,
        Connect,
        Disconnect,
        ChangePushProvider,
        MessageAck,
        ChangeUrgency,
        Wait,
    } type = NoCommand;
    /*!
     */
    Client client;
    /*!
     */
    QDBusMessage reply;
    /*!
     */
    QString value;
};

}

#endif // KUNIFIEDPUSH_COMMAND_H
