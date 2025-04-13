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

// The VAPID public key is usually provided by the application server,
// the VAPID private key is inaccessible to the application.
// This is only provided here for demonstration purposes as we don't
// have an application server here and instead use the "webpush-notify"
// CLI tool for this.
constexpr inline auto VAPID_PUBLIC_KEY = "BNH1upJppSLEASpoPmGDaeafgJ6KjZjxbDA3LbHG5yE1cAiKQ5qm_K08nVgibtxqm245l0BlaaMsu4iBGzV1wEU";
constexpr inline auto VAPID_PRIVATE_KEY = "9rxELCH8cvjOn2pUjSbfmYrm0NyNusqtPb5-OwcZUD4";

void printNoficationCommand(const KUnifiedPush::Connector &connector)
{
    if (connector.endpoint().isEmpty()) {
        return;
    }

    qDebug().noquote().nospace() << "echo \"Hello world!\" | "
        << QCoreApplication::applicationDirPath() << "/webpush-notify --endpoint \""
        << connector.endpoint() << "\" --user-agent-public-key " << connector.contentEncryptionPublicKey().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
        << " --auth-secret " << connector.contentEncryptionAuthSecret().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
        << " --vapid-public-key " << VAPID_PUBLIC_KEY
        << " --vapid-private-key " << VAPID_PRIVATE_KEY;
}

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
    connector.setVapidPublicKeyRequired(true);
    connector.setVapidPublicKey(QString::fromLatin1(VAPID_PUBLIC_KEY));
    QObject::connect(&connector, &KUnifiedPush::Connector::stateChanged, [unregisterRequested](auto state) {
        qDebug() << "Connector state changed:" << state;
        if (unregisterRequested && state == KUnifiedPush::Connector::Unregistered) {
            QCoreApplication::quit();
        }
    });
    QObject::connect(&connector, &KUnifiedPush::Connector::messageReceived, [](const auto &msg) {
        QProcess::startDetached(QStringLiteral("kdialog"), { QStringLiteral("--msgbox"), QString::fromUtf8(msg)});
    });

    if (!connector.endpoint().isEmpty()) {
        qDebug() << "Notification endpoint:" << connector.endpoint();
        printNoficationCommand(connector);
    }
    QObject::connect(&connector, &KUnifiedPush::Connector::endpointChanged, [&connector](const auto &endpoint) {
        qDebug() << "New notification endpoint:" << endpoint;
        printNoficationCommand(connector);
    });

    if (unregisterRequested) {
        if (connector.state() == KUnifiedPush::Connector::Unregistered) {
            return 0;
        }
        connector.unregisterClient();
    } else {
        connector.registerClient(QStringLiteral("Demonstrating push notifications."));
    }

    if (parser.isSet(dbusActivatedOpt)) {
        QTimer::singleShot(std::chrono::seconds(5), &app, &QCoreApplication::quit);
    }

    return app.exec();
}
