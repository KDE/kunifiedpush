/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KirigamiDelegates
import org.kde.kcmutils as KCM
import org.kde.kunifiedpush.kcm

KCM.ScrollViewKCM {
    id: root
    readonly property var pushProviderConfig: kcm.pushProviderConfiguration(pushProviderBox.currentText)

    headerPaddingEnabled: false

    header: ColumnLayout {
        spacing: 0

        // type of distributor, and if it is our own one, distributor status information
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Error
            text: i18n("There is no push notification service running!")
            icon.name: "dialog-error"
            visible: !kcm.hasDistributor
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Information
            text: i18n("There is a 3rd party push notification service running. Push notifications are available, but cannot be configured here.")
            icon.name: "dialog-information"
            visible: !kcm.hasKDEDistributor && kcm.hasDistributor
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Positive
            text: i18n("<b>Online</b><br>Connected to the push notification server and operational.")
            icon.name: "media-playback-playing"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.Connected
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Information
            text: i18n("<b>Idle</b><br>There are no applications using push notifications.")
            icon.name: "media-playback-paused"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.Idle
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Warning
            text: i18n("<b>Offline</b><br>Network connection to the server could not be established.")
            icon.name: "network-disconnect"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.NoNetwork
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Error
            text: i18n("<b>Offline</b><br>Could not authenticate at the server.")
            icon.name: "dialog-error"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.AuthenticationError
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Warning
            text: i18n("<b>Offline</b><br>Push notifications are not set up yet.")
            icon.name: "configure"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.NoSetup
            position: Kirigami.InlineMessage.Position.Header
        }

        QQC2.Control {
            padding: Kirigami.Units.largeSpacing
            visible: kcm.hasKDEDistributor
            Layout.fillWidth: true

            background: Rectangle {
                Kirigami.Theme.colorSet: Kirigami.Theme.Window
                Kirigami.Theme.inherit: false

                color: Kirigami.Theme.backgroundColor
            }

            // push provider configuration
            contentItem: ColumnLayout {
                spacing: 0

                Kirigami.FormLayout {
                    id: topForm
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    QQC2.ComboBox {
                        id: pushProviderBox
                        Kirigami.FormData.label: i18n("Push provider:")
                        model: ["Gotify", "NextPush", "Ntfy"]
                        currentIndex: find(kcm.pushProviderId)
                        Component.onCompleted: currentIndex = find(kcm.pushProviderId)
                    }
                }

                Loader {
                    id: providerFormLoader
                    Layout.fillWidth: true
                    visible: kcm.hasKDEDistributor
                    sourceComponent: {
                        switch (pushProviderBox.currentIndex) {
                            case 0:
                                return gotifyForm;
                            case 1:
                                return nextpushForm;
                            case 2:
                                return ntfyForm;
                        }
                        return undefined;
                    }
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
                        text: root.pushProviderConfig['Url'] ?? ''
                    }
                    QQC2.TextField {
                        id: tokenField
                        Kirigami.FormData.label: i18n("Client token:")
                        text: root.pushProviderConfig['ClientToken'] ?? ''
                    }
                }
            }
            Component {
                id: nextpushForm
                Kirigami.FormLayout {
                    id: nextpushConfig
                    readonly property bool dirty: urlField.text != root.pushProviderConfig['Url'] || userField.text != root.pushProviderConfig['Username'] || appPassword != root.pushProviderConfig['AppPassword']
                    property string appPassword: root.pushProviderConfig['AppPassword'] ?? '';
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
                        text: root.pushProviderConfig['Url'] ?? ''
                    }
                    QQC2.Label {
                        id: userField
                        Kirigami.FormData.label: i18n("User name:")
                        text: root.pushProviderConfig['Username'] ?? ''
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
            Component {
                id: ntfyForm
                Kirigami.FormLayout {
                    id: ntfyConfig
                    readonly property bool dirty: urlField.text != root.pushProviderConfig['Url']

                    function config() {
                        let c = root.pushProviderConfig;
                        c['Url'] = urlField.text;
                        return c;
                    }

                    twinFormLayouts: [topForm]
                    QQC2.TextField {
                        id: urlField
                        Kirigami.FormData.label: i18n("Url:")
                        text: root.pushProviderConfig['Url'] ?? ''
                    }
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
    }

    // registered clients
    view: ListView {
        model: kcm.clientModel
        header: Kirigami.InlineViewHeader {
            width: ListView.view.width
            text: i18n("Applications")
        }
        visible: count > 0

        delegate: QQC2.ItemDelegate {
            width: ListView.view.width

            text: model.display

            down: false
            highlighted: false
            hoverEnabled: false

            Kirigami.Theme.useAlternateBackgroundColor: true

            contentItem: RowLayout {
                spacing: 0
                KirigamiDelegates.IconTitleSubtitle {
                    title: model.name
                    subtitle: model.description
                    icon.source: model.iconName
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.ToolButton {
                    icon.name: "edit-delete"

                    onClicked: removePrompt.open()

                    QQC2.ToolTip.text: i18n("Unregister application from push notifications")

                    Kirigami.PromptDialog {
                        id: removePrompt

                        parent: QQC2.Overlay.overlay

                        title: i18nc("@title:window", "Unregister Application")
                        subtitle: i18nc("%1 is an application name", "Are you sure you want to unregister '%1'?", model.name)
                        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

                        onAccepted: kcm.forceUnregister(model.token)
                    }
                }
            }
        }
    }
}
