/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.15 as Kirigami
import org.kde.kcm 1.3 as KCM

KCM.SimpleKCM {
    id: root

    Kirigami.FormLayout {
        // TODO indicate whether there is a running distributor (and which one that is)
        QQC2.Label {
            Kirigami.FormData.label: "Distributor Id:"
            text: kcm.distributorId
        }

        // if using our own distributor
        // TODO indicate whether we are connected

        // TODO push provider configuration

        // TODO registered clients
    }
}
