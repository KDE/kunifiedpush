/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.15 as Kirigami
import org.kde.kcm 1.3 as KCM
import org.kde.kunifiedpush.kcm 1.0

KCM.SimpleKCM {
    id: root
    readonly property var pushProviderConfig: kcm.pushProviderConfiguration(pushProviderBox.currentText)

    header: ColumnLayout {
        // type of distributor, and if it is our own one, distributor status information
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Error
            text: i18n("There is no push notification service running!")
            icon.name: "dialog-error"
            visible: !kcm.hasDistributor
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Information
            text: i18n("There is a 3rd party push notification service running. Push notifications are available, but cannot be configured here.")
            icon.name: "dialog-information"
            visible: !kcm.hasKDEDistributor && kcm.hasDistributor
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Positive
            text: i18n("<b>Online</b><br>Connected to the push notification server and operational.")
            icon.name: "media-playback-playing"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.Connected
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Information
            text: i18n("<b>Idle</b><br>There are no applications using push notifications.")
            icon.name: "media-playback-paused"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.Idle
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Warning
            text: i18n("<b>Offline</b><br>Network connection to the server could not be established.")
            icon.name: "network-disconnect"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.NoNetwork
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Error
            text: i18n("<b>Offline</b><br>Could not authenticate at the server.")
            icon.name: "dialog-error"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.AuthenticationError
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Warning
            text: i18n("<b>Offline</b><br>Push notifications are not set up yet.")
            icon.name: "configure"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.NoSetup
        }

        // push provider configuration
        Kirigami.FormLayout {
            id: topForm
            visible: kcm.hasKDEDistributor
            Layout.fillWidth: true
            QQC2.ComboBox {
                id: pushProviderBox
                Kirigami.FormData.label: i18n("Push provider:")
                model: [ "Gotify", "NextPush" ]
                currentIndex: find(kcm.pushProviderId)
                Component.onCompleted: currentIndex = find(kcm.pushProviderId)
            }
        }

        Component {
            id: gotifyForm
            Kirigami.FormLayout {
                readonly property bool dirty: urlField.text != root.pushProviderConfig['Url'] || tokenField.text != root.pushProviderConfig['ClientToken']
                function config() {
                    let c = root.pushProviderConfig;
                    c['Url'] = urlField.text;
                    c['ClientToken'] = tokenField.text;
                    return c;
                }
                twinFormLayouts: [topForm]
                QQC2.TextField {
                    id: urlField
                    Kirigami.FormData.label: i18n("Url:")
                    text: root.pushProviderConfig['Url']
                }
                QQC2.TextField {
                    id: tokenField
                    Kirigami.FormData.label: i18n("Client token:")
                    text: root.pushProviderConfig['ClientToken']
                }
            }
        }
        Component {
            id: nextpushForm
            Kirigami.FormLayout {
                id: nextpushConfig
                readonly property bool dirty: urlField.text != root.pushProviderConfig['Url'] || userField.text != root.pushProviderConfig['Username'] || appPassword != root.pushProviderConfig['AppPassword']
                property string appPassword: root.pushProviderConfig['AppPassword'];
                function config() {
                    let c = root.pushProviderConfig;
                    c['Url'] = urlField.text;
                    c['Username'] = userField.text;
                    c['AppPassword'] = appPassword;
                    return c;
                }
                twinFormLayouts: [topForm]
                QQC2.TextField {
                    id: urlField
                    Kirigami.FormData.label: i18n("Url:")
                    text: root.pushProviderConfig['Url']
                }
                QQC2.Label {
                    id: userField
                    Kirigami.FormData.label: i18n("User name:")
                    text: root.pushProviderConfig['Username']
                }
                RowLayout {
                    QQC2.Button {
                        enabled: urlField.text != ""
                        text: i18n("Authenticate")
                        onClicked: {
                            authBusy.running = true;
                            kcm.nextcloudAuthenticate(urlField.text);
                        }

                    }
                    QQC2.BusyIndicator {
                        id: authBusy
                        running: false
                    }
                }
                Connections {
                    target: kcm
                    function onNextcloudAuthenticated(loginName, appPassword) {
                        userField.text = loginName;
                        nextpushConfig.appPassword = appPassword
                        authBusy.running = false;
                    }
                }
            }
        }

        Loader {
            id: providerFormLoader
            Layout.fillWidth: true
            visible: kcm.hasKDEDistributor
            sourceComponent: {
                switch (pushProviderBox.currentIndex) {
                    case 0: return gotifyForm;
                    case 1: return nextpushForm;
                }
                return undefined;
            }
        }

        Connections {
            target: kcm
            function onSaveRequested() {
                kcm.setPushProviderConfiguration(pushProviderBox.currentText, providerFormLoader.item.config());
            }
        }
        Binding {
            target: kcm
            property: "needsSave"
            value: providerFormLoader.item.dirty || pushProviderBox.currentText != kcm.pushProviderId
        }
    }

    // registered clients
    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: kcm.clientModel
        header: Kirigami.Heading { text: i18n("Applications") }
        visible: count > 0
        delegate: Kirigami.SwipeListItem {
            GridLayout {
                rows: 2
                columns: 2
                Kirigami.Icon  {
                    Layout.rowSpan: 2
                    source: model.iconName
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: model.name
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: model.description
                }
            }
            text: model.display
            actions: [
                Kirigami.Action {
                    iconName: "edit-delete"
                    text: i18n("Unregister application from push notifications")
                    // TODO safety question
                    onTriggered: kcm.forceUnregister(model.token)
                }
            ]
        }
    }
}
