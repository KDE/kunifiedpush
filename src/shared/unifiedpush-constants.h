/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_UNIFIEDPUSH_CONSTANTS_H
#define KUNIFIEDPUSH_UNIFIEDPUSH_CONSTANTS_H

#include <QString>

namespace KUnifiedPush
{

constexpr inline auto UP_CONNECTOR_PATH = QLatin1StringView("/org/unifiedpush/Connector");

constexpr inline auto UP_DISTRIBUTOR_SERVICE_NAME_PREFIX = QLatin1StringView("org.unifiedpush.Distributor.");
constexpr inline auto UP_DISTRIBUTOR_SERVICE_NAME_FILTER = QLatin1StringView("org.unifiedpush.Distributor*");
constexpr inline auto UP_DISTRIBUTOR_PATH = QLatin1StringView("/org/unifiedpush/Distributor");

constexpr inline auto UP_REGISTER_RESULT_SUCCESS = QLatin1StringView("REGISTRATION_SUCCEEDED");
constexpr inline auto UP_REGISTER_RESULT_FAILURE = QLatin1StringView("REGISTRATION_FAILED");

constexpr inline auto KDE_DISTRIBUTOR_SERVICE_NAME = QLatin1StringView("org.unifiedpush.Distributor.kde");
constexpr inline auto KDE_DISTRIBUTOR_MANAGEMENT_PATH = QLatin1StringView("/Management");

}

#endif
