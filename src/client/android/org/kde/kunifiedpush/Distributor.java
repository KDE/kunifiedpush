/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

package org.kde.kunifiedpush;

import android.content.*;
import android.content.pm.*;
import android.util.Log;
import java.util.List;
import java.util.ListIterator;

/** Interface to the distributor service. */
public class Distributor
{
    private static final String TAG = "org.kde.kunifiedpush";

    private String m_distributorName;
    private Context m_context;

    public Distributor(String distributorName, Context context)
    {
        m_distributorName = distributorName;
        m_context = context;
    }

    public void register(String token, String message)
    {
        Intent i = new Intent();
        i.setPackage(m_distributorName);
        i.setAction(UnifiedPush.ACTION_REGISTER);
        i.putExtra(UnifiedPush.EXTRA_TOKEN, token);
        i.putExtra(UnifiedPush.EXTRA_MESSAGE, message);
        i.putExtra(UnifiedPush.EXTRA_APPLICATION, m_context.getPackageName());
        m_context.sendBroadcast(i);
    }

    public void unregister(String token)
    {
        Intent i = new Intent();
        i.setPackage(m_distributorName);
        i.setAction(UnifiedPush.ACTION_UNREGISTER);
        i.putExtra(UnifiedPush.EXTRA_TOKEN, token);
        m_context.sendBroadcast(i);
    }

    private static boolean isExcludedDistributor(ResolveInfo info)
    {
        // TODO is there a way to check for the receiver no being exported?

        return info.activityInfo.packageName.equals("im.vector.app"); // Element claims to be a distributor...
    }

    public static String selectDistributor(Context context)
    {
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> r = pm.queryBroadcastReceivers(new Intent(UnifiedPush.ACTION_REGISTER), PackageManager.GET_RESOLVED_FILTER);
        ListIterator<ResolveInfo> it = r.listIterator();
        while (it.hasNext()) {
            ResolveInfo info = it.next();
            Log.d(TAG, info.activityInfo.packageName);
            if (isExcludedDistributor(info)) {
                Log.d(TAG, "... excluded");
                it.remove();
            }
        }

        if (r.isEmpty()) {
            return null;
        }
        // TODO proper multi-distributor selection
        return r.get(0).activityInfo.packageName;
    }
}
