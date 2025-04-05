/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_SELFTEST_H
#define KUNIFIEDPUSH_SELFTEST_H

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

namespace KUnifiedPush {
class Connector;
class Notifier;
}

/** Push notification self-test state machine. */
class SelfTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
public:
    explicit SelfTest(QObject *parent = nullptr);
    ~SelfTest();

    inline void setNetworkAccessManager(QNetworkAccessManager *nam) { m_nam = nam; }

    enum State {
        Idle,
        WaitingForEndpoint,
        Submitting,
        WaitingForMessage,
        Success,
        Error,
    };
    [[nodiscard]] inline State state() const { return m_state; }

    Q_INVOKABLE void start();

Q_SIGNALS:
    void stateChanged();

private:
    void endpointChanged();
    void submissionFinished(QNetworkReply *reply);
    void messageReceived(const QByteArray &msg);

    void setState(State state);

    QNetworkAccessManager *m_nam = nullptr;
    State m_state = Idle;
    std::unique_ptr<KUnifiedPush::Connector> m_connector;
    std::unique_ptr<KUnifiedPush::Notifier> m_notifier;
    QByteArray m_msg;
};

#endif
