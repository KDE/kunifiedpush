/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_SELFTEST_H
#define KUNIFIEDPUSH_SELFTEST_H

#include <QObject>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

namespace KUnifiedPush {
class Connector;
class Notifier;
}

/*!
 * \class SelfTest
 * \inmodule KUnifiedPush
 * \brief Push notification self-test state machine.
 */
class SelfTest : public QObject
{
    Q_OBJECT
    /*!
     * \property SelfTest::state
     */
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    /*!
     * \property SelfTest::errorMessage
     */
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    /*!
     */
    explicit SelfTest(QObject *parent = nullptr);
    /*!
     */
    ~SelfTest();

    /*!
     */
    inline void setNetworkAccessManager(QNetworkAccessManager *nam) { m_nam = nam; }

    /*!
     * \enum SelfTest::State
     * \value Idle
     * \value WaitingForEndpoint
     * \value Submitting
     * \value WaitingForMessage
     * \value Success
     * \value Error
     */
    enum State {
        Idle,
        WaitingForEndpoint,
        Submitting,
        WaitingForMessage,
        Success,
        Error,
    };
    Q_ENUM(State)

    /*!
     */
    [[nodiscard]] inline State state() const { return m_state; }
    /*!
     */
    [[nodiscard]] inline QString errorMessage() const { return m_errorMsg; }

    /*!
     */
    Q_INVOKABLE void start();
    /*!
     */
    Q_INVOKABLE void reset();

Q_SIGNALS:
    void stateChanged();
    void errorMessageChanged();

private:
    void endpointChanged();
    void submissionFinished(QNetworkReply *reply);
    void messageReceived(const QByteArray &msg);

    void setState(State state);
    void setErrorMessage(const QString &errMsg);

    QNetworkAccessManager *m_nam = nullptr;
    State m_state = Idle;
    std::unique_ptr<KUnifiedPush::Connector> m_connector;
    std::unique_ptr<KUnifiedPush::Notifier> m_notifier;
    QByteArray m_msg;
    QString m_errorMsg;
    QTimer m_timer;
};

#endif
