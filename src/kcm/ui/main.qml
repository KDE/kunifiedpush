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
    readonly property var pushProviderConfig: kcm.pushProviderConfiguration(pushProviderBox.currentValue)

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
            text: kcm.distributorErrorMessage ?
                i18n("<b>Offline</b><br>Network connection to the server could not be established (%1).", kcm.distributorErrorMessage) :
                i18n("<b>Offline</b><br>Network connection to the server could not be established.")
            icon.name: "network-disconnect"
            visible: kcm.hasKDEDistributor && kcm.distributorStatus == DistributorStatus.NoNetwork
            position: Kirigami.InlineMessage.Position.Header
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            showCloseButton: false
            type: Kirigami.MessageType.Error
            text: kcm.distributorErrorMessage ?
                i18n("<b>Offline</b><br>Could not authenticate at the server (%1).", kcm.distributorErrorMessage) :
                i18n("<b>Offline</b><br>Could not authenticate at the server.")
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
                        model: ListModel {
                            ListElement { text: "Gotify"; key: "Gotify" }
                            ListElement { text: "Mozilla WebPush"; key: "Autopush" }
                            ListElement { text: "NextPush"; key: "NextPush" }
                            ListElement { text: "Ntfy"; key: "Ntfy" }
                        }
                        textRole: "text"
                        valueRole: "key"
                        currentIndex: indexOfValue(kcm.pushProviderId)
                        Component.onCompleted: currentIndex = indexOfValue(kcm.pushProviderId)
                    }
                }

                Loader {
                    id: providerFormLoader
                    Layout.fillWidth: true
                    visible: kcm.hasKDEDistributor
                    sourceComponent: {
                        const forms = [gotifyForm, autopushForm, nextpushForm, ntfyForm];
                        return forms[pushProviderBox.currentIndex];
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
                        Kirigami.FormData.label: i18n("URL:")
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
                        Kirigami.FormData.label: i18n("URL:")
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
                        Kirigami.FormData.label: i18n("URL:")
                        text: root.pushProviderConfig['Url'] ?? ''
                    }
                    Kirigami.InlineMessage {
                        Kirigami.FormData.isSection: true
                        visible: urlField.text.startsWith("https://ntfy.sh")
                        icon.name: "data-warning"
                        text: i18n("Note that ntfy.sh is rate-limiting applications. It will therefore not work with applications with a larger amount of users, such as public Matrix or Mastodon instances.")
                        type: Kirigami.MessageType.Warning
                    }
                }
            }
            Component {
                id: autopushForm
                Kirigami.FormLayout {
                    readonly property bool dirty: urlField.text != root.pushProviderConfig['Url']

                    function config() {
                        let c = root.pushProviderConfig;
                        c['Url'] = urlField.text;
                        return c;
                    }

                    twinFormLayouts: [topForm]
                    QQC2.TextField {
                        id: urlField
                        Kirigami.FormData.label: i18n("URL:")
                        text: root.pushProviderConfig['Url'] ?? 'https://push.services.mozilla.com'
                    }
                }
            }

            Connections {
                target: kcm

                function onSaveRequested() {
                    kcm.setPushProviderConfiguration(pushProviderBox.currentValue, providerFormLoader.item.config());
                }
            }
            Binding {
                target: kcm
                property: "needsSave"
                value: providerFormLoader.item.dirty || pushProviderBox.currentValue != kcm.pushProviderId
            }
        }

        RowLayout {
            visible: kcm.hasDistributor
            enabled: !kcm.needsSave
            Kirigami.Icon {
                source: {
                    switch (kcm.selfTest.state) {
                        case SelfTest.Success:
                            return "dialog-positive";
                        case SelfTest.Error:
                            return "dialog-error";
                        case SelfTest.WaitingForEndpoint:
                            return "network-connect";
                        case SelfTest.Submitting:
                            return "cloud-upload";
                        case SelfTest.WaitingForMessage:
                            return "cloud-download";
                    }
                }
            }
            Kirigami.TitleSubtitle {
                Layout.fillWidth: true
                title: {
                    switch (kcm.selfTest.state) {
                        case SelfTest.Idle:
                            return "";
                        case SelfTest.Success:
                            return i18n("Push notifications are working correctly.")
                        case SelfTest.Error:
                            return i18n("Push notification test failed.")
                        case SelfTest.WaitingForEndpoint:
                            return i18n("Registering with push server…")
                        case SelfTest.Submitting:
                            return i18n("Sending push notification…")
                        case SelfTest.WaitingForMessage:
                            return i18n("Waiting to receive push notification…")
                    }
                }
                subtitle: kcm.selfTest.errorMessage
            }
            QQC2.Button {
                id: testButton
                text: i18nc("@action:button", "Test")
                icon.name: "media-playback-start"
                visible: kcm.selfTest.state === SelfTest.Idle || kcm.selfTest.state === SelfTest.Success || kcm.selfTest.state === SelfTest.Error
                onClicked: kcm.selfTest.start()
            }
            QQC2.BusyIndicator {
                visible: !testButton.visible
                running: visible
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
