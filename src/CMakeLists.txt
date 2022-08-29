# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_subdirectory(client)
if (NOT ANDROID)
    add_subdirectory(distributor)
    add_subdirectory(kcm)
endif()
