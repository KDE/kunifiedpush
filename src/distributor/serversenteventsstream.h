/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_SERVERSENTEVENTSSTREAM_H
#define KUNIFIEDPUSH_SERVERSENTEVENTSSTREAM_H

#include <QObject>

class QNetworkReply;

namespace KUnifiedPush {

class SSEMessage
{
public:
    QByteArray event;
    QByteArray data;
};

/** Sever-sent Events (SSE) stream
 *  @see https://en.wikipedia.org/wiki/Server-sent_events
 */
class ServerSentEventsStream : public QObject
{
    Q_OBJECT
public:
    explicit ServerSentEventsStream(QObject *parent = nullptr);
    ~ServerSentEventsStream();

    void read(QNetworkReply *reply);

Q_SIGNALS:
    void messageReceived(const SSEMessage &msg);

private:
    void processBuffer();

    QByteArray m_buffer;
};

}

#endif // KUNIFIEDPUSH_SERVERSENTEVENTSSTREAM_H
