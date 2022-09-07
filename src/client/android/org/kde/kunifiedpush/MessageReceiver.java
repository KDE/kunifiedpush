/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

package org.kde.kunifiedpush;

import android.content.*;
import android.util.Log;

/** Notification message receiver interface. */
public class MessageReceiver extends BroadcastReceiver
{
    private static final String TAG = "org.kde.kunifiedpush";

    @Override
    public void onReceive(Context context, Intent intent)
    {
        Log.d(TAG, "onReceive:" + intent.getAction());
        try {
            switch (intent.getAction()) {
            case UnifiedPush.ACTION_NEW_ENDPOINT:
                newEndpoint(intent.getStringExtra(UnifiedPush.EXTRA_TOKEN), intent.getStringExtra(UnifiedPush.EXTRA_ENDPOINT));
                break;
            case UnifiedPush.ACTION_REGISTRATION_FAILED:
                registrationFailed(intent.getStringExtra(UnifiedPush.EXTRA_TOKEN), intent.getStringExtra(UnifiedPush.EXTRA_MESSAGE));
                break;
            case UnifiedPush.ACTION_UNREGISTERED:
                unregistered(intent.getStringExtra(UnifiedPush.EXTRA_TOKEN));
                break;
            case UnifiedPush.ACTION_MESSAGE:
                message(intent.getStringExtra(UnifiedPush.EXTRA_TOKEN), intent.getByteArrayExtra(UnifiedPush.EXTRA_BYTES_MESSAGE),  intent.getStringExtra(UnifiedPush.EXTRA_MESSAGE_ID));
                break;
            }
        } catch (java.lang.UnsatisfiedLinkError e) {
            // TODO when the app isn't running the C++ part isn't loaded at this point
            Log.e(TAG, "native part not loaded " + e.toString());
        }
    }

    private native void newEndpoint(String token, String endpoint);
    private native void registrationFailed(String token, String reason);
    private native void unregistered(String token);
    private native void message(String token, byte[] message, String messageId);
}
