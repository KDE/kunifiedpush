/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "distributor.h"
#include "logging.h"

#include "../shared/unifiedpush-constants.h"
#include "kunifiedpush_version.h"

#include <QCoreApplication>
#include <QDBusConnection>

#include <KAboutData>

using namespace KUnifiedPush;
using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));

    QCoreApplication app(argc, argv);

    KAboutData about(u"kunifiedpush-distributor"_s, QString(), QString::fromLatin1(KUNIFIEDPUSH_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KUnifiedPush::Distributor distributor;
    if (!QDBusConnection::sessionBus().registerService(KDE_DISTRIBUTOR_SERVICE_NAME)) {
        qCCritical(Log) << "Distributor service name already in use - aborting!";
        return 1;
    }

    return app.exec();
}
