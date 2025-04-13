/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NTFYPUSHPROVIDER_H
#define KUNIFIEDPUSH_NTFYPUSHPROVIDER_H

#include "abstractpushprovider.h"
#include "serversenteventsstream.h"

#include <QPointer>
#include <QUrl>

class QNetworkReply;

namespace KUnifiedPush {

/** Ntfy push provider. */
class NtfyPushProvider : public AbstractPushProvider
{
    Q_OBJECT
public:
    explicit NtfyPushProvider(QObject *parent = nullptr);
    ~NtfyPushProvider();

    bool loadSettings(const QSettings &settings) override;
    void resetSettings(QSettings &settings) override;
    void connectToProvider(Urgency urgency) override;
    void disconnectFromProvider() override;
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;

    static constexpr inline auto Id = QLatin1StringView("Ntfy");

protected:
    // TODO see doConnectToProvider
    // void doChangeUrgency(Urgency urgency) override;

private:
    void doConnectToProvider(Urgency urgency);
    void storeState();

    QUrl m_url;
    QStringList m_topics;
    QString m_lastMessageId;

    QPointer<QNetworkReply> m_sseReply;
    ServerSentEventsStream m_sseStream;
};

}

#endif // KUNIFIEDPUSH_NTFYPUSHPROVIDER_H
