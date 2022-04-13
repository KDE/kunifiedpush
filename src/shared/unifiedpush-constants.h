/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_UNIFIEDPUSH_CONSTANTS_H
#define KUNIFIEDPUSH_UNIFIEDPUSH_CONSTANTS_H

namespace KUnifiedPush
{

constexpr const char UP_CONNECTOR_PATH[] = "/org/unifiedpush/Connector";

constexpr const char UP_DISTRIBUTOR_SERVICE_NAME_PREFIX[] = "org.unifiedpush.Distributor.";
constexpr const char UP_DISTRIBUTOR_PATH[] = "/org/unifiedpush/Distributor";

constexpr const char UP_REGISTER_RESULT_SUCCESS[] = "REGISTRATION_SUCCEEDED";
constexpr const char UP_REGISTER_RESULT_FAILURE[] = "REGISTRATION_FAILED";

}

#endif
