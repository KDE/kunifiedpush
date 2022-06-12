/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NEXTPUSHPROVIDER_H
#define KUNIFIEDPUSH_NEXTPUSHPROVIDER_H

#include "abstractpushprovider.h"
#include "serversenteventsstream.h"

#include <QUrl>

class QNetworkRequest;

namespace KUnifiedPush {

/** NextPush push provider implementation.
 *  @see https://github.com/UP-NextPush/
 */
class NextPushProvider : public AbstractPushProvider
{
public:
    explicit NextPushProvider(QObject *parent = nullptr);
    ~NextPushProvider();

    void loadSettings(const QSettings &settings) override;
    void connectToProvider() override;
    void disconnectFromProvider() override;
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;

private:
    void waitForMessage();
    QNetworkRequest prepareRequest(const char *restCmd, const QString &restArg = {}) const;

    QUrl m_url;
    QString m_userName;
    QString m_appPassword;
    QString m_deviceId;
    ServerSentEventsStream m_sseStream;
};

}

#endif // KUNIFIEDPUSH_NEXTPUSHPROVIDER_H
