# SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_executable(vapid-keygen)
target_sources(vapid-keygen PRIVATE
    vapid-keygen.cpp
    ../shared/eckey.cpp
)
target_include_directories(vapid-keygen PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(vapid-keygen PRIVATE
    Qt6::Core
    OpenSSL::Crypto
)


add_executable(webpush-notify)
target_sources(webpush-notify PRIVATE
    webpush-notify.cpp
)
target_include_directories(webpush-notify PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(webpush-notify PRIVATE
    KUnifiedPushNotifier
    Qt6::Network
)
