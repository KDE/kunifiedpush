/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NEXTPUSHPROVIDER_H
#define KUNIFIEDPUSH_NEXTPUSHPROVIDER_H

#include "abstractpushprovider.h"
#include "serversenteventsstream.h"

#include <QPointer>
#include <QUrl>

class QNetworkRequest;
class QNetworkReply;

namespace KUnifiedPush {

/** NextPush push provider implementation.
 *  @see https://github.com/UP-NextPush/
 */
class NextPushProvider : public AbstractPushProvider
{
public:
    explicit NextPushProvider(QObject *parent = nullptr);
    ~NextPushProvider();

    bool loadSettings(const QSettings &settings) override;
    void resetSettings(QSettings &settings) override;
    void connectToProvider(Urgency urgency) override;
    void disconnectFromProvider() override;
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;

    static constexpr inline auto Id = QLatin1StringView("NextPush");
protected:
    void doChangeUrgency(Urgency urgency) override;

private:
    void waitForMessage(Urgency urgency);
    QNetworkRequest prepareRequest(const char *restCmd, const QString &restArg = {}) const;

    QUrl m_url;
    QString m_userName;
    QString m_appPassword;
    QString m_deviceId;

    QPointer<QNetworkReply> m_sseReply;
    ServerSentEventsStream m_sseStream;
};

}

#endif // KUNIFIEDPUSH_NEXTPUSHPROVIDER_H
