/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_URGENCY_P_H
#define KUNIFIEDPUSH_URGENCY_P_H

#include <qglobal.h>

namespace KUnifiedPush {

enum class Urgency {
    VeryLow,
    Low,
    Normal,
    High
};

constexpr inline auto DefaultUrgency = Urgency::Normal;

constexpr const char* urgencyValue(Urgency urgency)
{
    constexpr const char* map[] = { "very-low", "low", "normal", "high" };
    return map[qToUnderlying(urgency)];
}

}

#endif
