/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUNIFIEDPUSH_NEXTPUSHPROVIDER_H
#define KUNIFIEDPUSH_NEXTPUSHPROVIDER_H

#include "abstractpushprovider.h"

#include <QUrl>

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
    void registerClient(const Client &client) override;
    void unregisterClient(const Client &client) override;

private:
    QUrl m_url;
    QString m_userName;
    QString m_appPassword;
    QString m_deviceId;
};

}

#endif // KUNIFIEDPUSH_NEXTPUSHPROVIDER_H
