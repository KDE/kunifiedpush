/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "serversenteventsstream.h"
#include "logging.h"

#include <QIODevice>

#include <algorithm>
#include <cstring>

using namespace KUnifiedPush;

ServerSentEventsStream::ServerSentEventsStream(QObject *parent)
    : QObject(parent)
{
}

ServerSentEventsStream::~ServerSentEventsStream() = default;

void ServerSentEventsStream::read(QIODevice *device)
{
    m_buffer.clear();
    connect(device, &QIODevice::readyRead, this, [device, this]() {
        m_buffer.append(device->read(device->bytesAvailable()));
        processBuffer();
    });
}

QByteArray ServerSentEventsStream::buffer() const
{
    return m_buffer;
}

static bool isLineBreak(char c)
{
    return c == '\n' || c == '\r';
}

static QByteArray::ConstIterator findLineBreak(const QByteArray::const_iterator &begin, const QByteArray::const_iterator &end)
{
    return std::find_if(begin, end, isLineBreak);
}

static QByteArray::const_iterator consumeLineBreak(const QByteArray::const_iterator &begin, const QByteArray::const_iterator &end)
{
    auto it = begin;
    if (it == end) {
    } else if ((*it) == '\n') {
        ++it;
    } else if ((*it) == '\r') {
        ++it;
        if (it != end && (*it) == '\n') {
            ++it;
        }
    }

    return it;
}

static QByteArray::const_iterator findMessageEnd(const QByteArray::const_iterator &begin, const QByteArray::const_iterator &end)
{
    for (auto it = findLineBreak(begin, end); it != end; it = findLineBreak(it, end)) {
        auto lookAhead = consumeLineBreak(it, end);
        if (lookAhead != end && isLineBreak(*lookAhead)) {
            return it;
        }
        it = lookAhead;
    }

    return end;
}

void ServerSentEventsStream::processBuffer()
{
    qCDebug(Log) << m_buffer;
    auto msgEnd = findMessageEnd(m_buffer.begin(), m_buffer.end());
    if (msgEnd == m_buffer.end()) {
        qCDebug(Log) << "buffer incomplete, waiting for more";
        return;
    }

    SSEMessage msg;
    for (auto it = m_buffer.constBegin(); it != msgEnd;) {
        auto lineBegin = it;
        auto lineEnd = findLineBreak(lineBegin, msgEnd);
        it = consumeLineBreak(lineEnd, msgEnd);
        Q_ASSERT(lineBegin != it);

        auto colonIt = std::find(lineBegin, lineEnd, ':');
        auto valueBegin = colonIt;
        if (valueBegin != lineEnd) {
            ++valueBegin;
            if (valueBegin != lineEnd && (*valueBegin) == ' ') {
                ++valueBegin;
            }
        }

        if (colonIt == lineBegin || valueBegin == lineEnd) {
            continue; // comment or value-less field
        }

        const auto fieldNameLen = std::distance(lineBegin, colonIt);
        if (fieldNameLen == 4 && std::strncmp(lineBegin, "data", fieldNameLen) == 0) {
            msg.data.append(valueBegin, std::distance(valueBegin, lineEnd));
        } else if (fieldNameLen == 5 && std::strncmp(lineBegin, "event", fieldNameLen) == 0) {
            msg.event = QByteArray(valueBegin, std::distance(valueBegin, lineEnd));
        } else {
            msg.metaData.insert(QByteArray(lineBegin, std::distance(lineBegin, colonIt)), QByteArray(valueBegin, std::distance(valueBegin, lineEnd)));
        }
    }

    // defer emission of messages until the below is finished as well
    // this avoids reaction to this pulling things out under our feet here
    QMetaObject::invokeMethod(this, [msg, this]() { Q_EMIT messageReceived(msg); }, Qt::QueuedConnection);

    msgEnd = consumeLineBreak(msgEnd, m_buffer.end());
    msgEnd = consumeLineBreak(msgEnd, m_buffer.end());
    if (msgEnd == m_buffer.end()) {
        m_buffer.clear();
    } else {
        const auto remainingLen = m_buffer.size() - std::distance(m_buffer.constBegin(), msgEnd);
        std::memmove(m_buffer.begin(), msgEnd, remainingLen);
        m_buffer.truncate(remainingLen);
        processBuffer();
    }
}

#include "moc_serversenteventsstream.cpp"
