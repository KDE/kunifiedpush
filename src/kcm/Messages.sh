#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` -o $podir/kcm_push_notifications.pot
