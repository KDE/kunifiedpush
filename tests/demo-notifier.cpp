/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QProcess>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const auto serviceName = QStringLiteral("org.kde.kunifiedpush.demo-notifier");
    QDBusConnection::sessionBus().registerService(serviceName);
    KUnifiedPush::Connector connector(serviceName);
    QObject::connect(&connector, &KUnifiedPush::Connector::stateChanged, [](auto state) {
        qDebug() << "Connector state changed:" << state;
    });
    QObject::connect(&connector, &KUnifiedPush::Connector::messageReceived, [](const auto &msg) {
        QProcess::startDetached(QStringLiteral("kdialog"), { QStringLiteral("--msgbox"), msg});
    });

    if (!connector.endpoint().isEmpty()) {
        qDebug() << "Notification endpoint:" << connector.endpoint();
    }
    QObject::connect(&connector, &KUnifiedPush::Connector::endpointChanged, [](const auto &endpoint) {
        qDebug() << "New notification endpoint:" << endpoint;
    });

    return app.exec();
}
