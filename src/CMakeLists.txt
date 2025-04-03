# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_subdirectory(client)
if (NOT ANDROID AND NOT KUNIFIEDPUSH_CLIENT_ONLY)
    add_subdirectory(distributor)
    add_subdirectory(notifier)
    add_subdirectory(kcm)
    add_subdirectory(tools)
endif()
