/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector_p.h"

using namespace KUnifiedPush;

void ConnectorPrivate::init()
{
    // TODO create/register MessageReceiver, register native method callbacks
}

void ConnectorPrivate::deinit()
{
}

void ConnectorPrivate::doSetDistributor(const QString &distServiceName)
{
    // TODO create Distrubutor JNI object
}

bool ConnectorPrivate::hasDistributor() const
{
    return false;
}

void ConnectorPrivate::doRegister()
{
}

void ConnectorPrivate::doUnregister()
{
}
