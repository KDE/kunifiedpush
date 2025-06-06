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

constexpr inline auto UP_ARG_DESCRIPTION = QLatin1StringView("description");
constexpr inline auto UP_ARG_ENDPOINT = QLatin1StringView("endpoint");
constexpr inline auto UP_ARG_MESSAGE = QLatin1StringView("message");
constexpr inline auto UP_ARG_MESSAGE_IDENTIFIER = QLatin1StringView("id");
constexpr inline auto UP_ARG_REASON = QLatin1StringView("reason");
constexpr inline auto UP_ARG_SERVICE = QLatin1StringView("service");
constexpr inline auto UP_ARG_SUCCESS = QLatin1StringView("success");
constexpr inline auto UP_ARG_TOKEN = QLatin1StringView("token");
constexpr inline auto UP_ARG_VAPID = QLatin1StringView("vapid");

constexpr inline auto UP_REGISTER_RESULT_SUCCESS = QLatin1StringView("REGISTRATION_SUCCEEDED");
constexpr inline auto UP_REGISTER_RESULT_FAILURE = QLatin1StringView("REGISTRATION_FAILED");

constexpr inline auto UP_REGISTER_FAILURE_INTERNAL_ERROR = QLatin1StringView("INTERNAL_ERROR");
constexpr inline auto UP_REGISTER_FAILURE_NETWORK = QLatin1StringView("NETWORK");
constexpr inline auto UP_REGISTER_FAILURE_ACTION_REQUIRED = QLatin1StringView("ACTION_REQUIRED");
constexpr inline auto UP_REGISTER_FAILURE_VAPID_REQUIRED = QLatin1StringView("VAPID_REQUIRED");
constexpr inline auto UP_REGISTER_FAILURE_UNAUTHORIZED = QLatin1StringView("UNAUTHORIZED");

constexpr inline auto KDE_DISTRIBUTOR_SERVICE_NAME = QLatin1StringView("org.unifiedpush.Distributor.kde");
constexpr inline auto KDE_DISTRIBUTOR_MANAGEMENT_PATH = QLatin1StringView("/Management");

}

#endif
