/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "distributor.h"
#include "logging.h"

#include <QCoreApplication>
#include <QDBusConnection>

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));

    QCoreApplication app(argc, argv);
    KUnifiedPush::Distributor distributor;
    if (!QDBusConnection::sessionBus().registerService(QStringLiteral("org.unifiedpush.Distributor.kde"))) {
        qCCritical(Log) << "Distributor service name already in use - aborting!";
        return 1;
    }

    return app.exec();
}
