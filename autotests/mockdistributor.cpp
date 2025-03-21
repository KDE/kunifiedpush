/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/distributor/distributor.h"
#include "../src/distributor/message.h"
#include "../src/distributor/mockpushprovider.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QStandardPaths>

using namespace Qt::Literals;

class MockController : public QObject
{
    Q_OBJECT
public:
    Q_SCRIPTABLE void quit()
    {
        QCoreApplication::quit();
    }

    Q_SCRIPTABLE void receiveMessage(const QString &remoteId, const QByteArray &content, const QString &messageIdentifier)
    {
        KUnifiedPush::Message msg;
        msg.clientRemoteId = remoteId;
        msg.content = content;
        msg.messageId = messageIdentifier;
        Q_EMIT KUnifiedPush::MockPushProvider::s_instance->messageReceived(msg);
    }
};

int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);

    QCoreApplication::setOrganizationDomain(u"kde.org"_s);
    QCoreApplication::setOrganizationName(u"KDE"_s);

    QCoreApplication app(argc, argv);

    qDebug() << "Starting mock distributor";
    MockController controller;
    QDBusConnection::sessionBus().registerObject("/MockController"_L1, "org.kde.unifiedpush.MockController"_L1, &controller, QDBusConnection::ExportAllContents);

    KUnifiedPush::Distributor dist;
    if (!QDBusConnection::sessionBus().registerService(u"org.unifiedpush.Distributor.mock"_s)) {
        qCritical() << "Distributor service name already in use - aborting!";
        return 1;
    }

    const auto r = QCoreApplication::exec();
    qDebug() << "Stopping mock Distributor";
    return r;
}

#include "mockdistributor.moc"
