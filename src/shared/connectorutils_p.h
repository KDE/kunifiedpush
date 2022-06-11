/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_CONNECTORUTILS_P_H
#define KUNIFIEDPUSH_CONNECTORUTILS_P_H

#include <qcontainerfwd.h>

class QString;

namespace KUnifiedPush
{

/** Connector-side functions shared between the connector library and the KCM. */
class ConnectorUtils {
public:

    /** Searches for all available distributors and picks one to use.
    *  Considers explicit user configuration.
    */
    static QString selectDistributor();

private:
    friend class ConnectorUtilsTest;
    static QString selectDistributor(QStringList &&services);
};

}

#endif

