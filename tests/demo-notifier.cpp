/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QProcess>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kunifiedpush.demo-notifier"));

    KUnifiedPush::Connector connector;
    QObject::connect(&connector, &KUnifiedPush::Connector::messageReceived, [](const auto &msg) {
        QProcess::startDetached(QStringLiteral("kdialog"), { QStringLiteral("--msgbox"), msg});
    });

    return app.exec();
}
