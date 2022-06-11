/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_DISTRIBUTORSTATUS_P_H
#define KUNIFIEDPUSH_DISTRIBUTORSTATUS_P_H

namespace KUnifiedPush
{

/** Connector-side functions shared between the connector library and the KCM. */
namespace DistributorStatus {
    Q_NAMESPACE

    enum Status {
        Unknown,
        Idle,
        Connected,
        NoNetwork,
        AuthenticationError,
        NoSetup
    };
    Q_ENUM_NS(Status)
}

}

#endif
