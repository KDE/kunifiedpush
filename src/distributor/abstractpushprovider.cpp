/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractpushprovider.h"

using namespace KUnifiedPush;

AbstractPushProvider::AbstractPushProvider(QObject *parent)
    : QObject(parent)
{
}
