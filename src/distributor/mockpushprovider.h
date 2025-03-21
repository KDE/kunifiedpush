/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_MOCKPUSHPROVIDER_H
#define KUNIFIEDPUSH_MOCKPUSHPROVIDER_H

#include "abstractpushprovider.h"

namespace KUnifiedPush {

/** Mock push provider for automated tests. */
class MockPushProvider : public AbstractPushProvider
{
public:
    explicit MockPushProvider(QObject *parent = nullptr);
    ~MockPushProvider();

    bool loadSettings(const QSettings &settings) override;
    void connectToProvider() override;
    void disconnectFromProvider() override;
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;
    void acknowledgeMessage(const Client &client, const QString &messageIdentifier) override;

    static constexpr inline auto Id = QLatin1StringView("Mock");
    static MockPushProvider *s_instance;
};

}

#endif // KUNIFIEDPUSH_MOCKPUSHPROVIDER_H
