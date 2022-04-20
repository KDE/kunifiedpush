/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KUnifiedPush/Connector>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QProcess>
#include <QTimer>

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption unregisterOpt(QStringLiteral("unregister"));
    parser.addOption(unregisterOpt);
    QCommandLineOption dbusActivatedOpt(QStringLiteral("dbus-activated"));
    parser.addOption(dbusActivatedOpt);


    QCoreApplication app(argc, argv);
    parser.process(app);

    const auto unregisterRequested = parser.isSet(unregisterOpt);

    const auto serviceName = QStringLiteral("org.kde.kunifiedpush.demo-notifier");
    if (!QDBusConnection::sessionBus().registerService(serviceName)) {
        qCritical("Service name already in use.");
        return 1;
    }

    KUnifiedPush::Connector connector(serviceName);
    QObject::connect(&connector, &KUnifiedPush::Connector::stateChanged, [unregisterRequested](auto state) {
        qDebug() << "Connector state changed:" << state;
        if (unregisterRequested && state == KUnifiedPush::Connector::Unregistered) {
            QCoreApplication::quit();
        }
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

    if (unregisterRequested) {
        if (connector.state() == KUnifiedPush::Connector::Unregistered) {
            return 0;
        }
        connector.unregisterClient();
    } else {
        connector.registerClient();
    }

    if (parser.isSet(dbusActivatedOpt)) {
        QTimer::singleShot(std::chrono::seconds(5), &app, &QCoreApplication::quit);
    }

    return app.exec();
}
