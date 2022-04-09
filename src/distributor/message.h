/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_MESSAGE_H
#define KUNIFIEDPUSH_MESSAGE_H

#include <QString>

namespace KUnifiedPush {

/** A received push notification message. */
class Message
{
public:
    QString clientRemoteId;
    QString content;
};

}

#endif // KUNIFIEDPUSH_MESSAGE_H
