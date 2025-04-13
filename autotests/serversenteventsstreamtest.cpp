/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/distributor/serversenteventsstream.h"

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>

using namespace KUnifiedPush;

static void fakeStreamWrite(QBuffer &buffer, const char *data)
{
    const auto pos = buffer.pos();
    buffer.write(data);
    buffer.seek(pos);
    Q_EMIT buffer.readyRead();
}

class ServerSentEventsStreamTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testRead()
    {
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        ServerSentEventsStream stream;
        QSignalSpy eventSpy(&stream, &ServerSentEventsStream::messageReceived);
        stream.read(&buffer);

        fakeStreamWrite(buffer, "event: start\ndata: {\"type\":\"start\"}\n\n");
        QVERIFY(eventSpy.wait());
        QCOMPARE(eventSpy.size(), 1);
        auto msg = eventSpy.at(0).at(0).value<SSEMessage>();
        QCOMPARE(msg.event, "start");
        QCOMPARE(msg.data, "{\"type\":\"start\"}");
        eventSpy.clear();

        fakeStreamWrite(buffer, "");
        QCOMPARE(eventSpy.size(), 0);
        fakeStreamWrite(buffer, "event: keepalive\r\n");
        QCOMPARE(eventSpy.size(), 0);
        fakeStreamWrite(buffer, "data: {\"type\":\"keepalive\",\"keepalive\":300}\r\n\r\n");
        QVERIFY(eventSpy.wait());
        QCOMPARE(eventSpy.size(), 1);
        msg = eventSpy.at(0).at(0).value<SSEMessage>();
        QCOMPARE(msg.event, "keepalive");
        QCOMPARE(msg.data, "{\"type\":\"keepalive\",\"keepalive\":300}");
    }
};

QTEST_GUILESS_MAIN(ServerSentEventsStreamTest)

#include "serversenteventsstreamtest.moc"
