# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_executable(demo-notifier demo-notifier.cpp)
target_link_libraries(demo-notifier PRIVATE
    KUnifiedPush
    Qt::DBus
)

configure_file(org.kde.kunifiedpush.demo-notifier.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kunifiedpush.demo-notifier.service)
