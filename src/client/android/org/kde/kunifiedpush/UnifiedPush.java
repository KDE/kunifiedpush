/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

package org.kde.kunifiedpush;

public class UnifiedPush
{
    static final String ACTION_NEW_ENDPOINT = "org.unifiedpush.android.connector.NEW_ENDPOINT";
    static final String ACTION_REGISTRATION_FAILED = "org.unifiedpush.android.connector.REGISTRATION_FAILED";
    static final String ACTION_UNREGISTERED = "org.unifiedpush.android.connector.UNREGISTERED";
    static final String ACTION_MESSAGE = "org.unifiedpush.android.connector.MESSAGE";

    static final String ACTION_REGISTER = "org.unifiedpush.android.distributor.REGISTER";
    static final String ACTION_UNREGISTER = "org.unifiedpush.android.distributor.UNREGISTER";
    static final String ACTION_MESSAGE_ACK = "org.unifiedpush.android.distributor.MESSAGE_ACK";

    static final String EXTRA_APPLICATION = "application";
    static final String EXTRA_TOKEN = "token";
    static final String EXTRA_FEATURES = "features";
    static final String EXTRA_ENDPOINT = "endpoint";
    static final String EXTRA_MESSAGE = "message";
    static final String EXTRA_BYTES_MESSAGE = "bytesMessage";
    static final String EXTRA_MESSAGE_ID = "id";
}
