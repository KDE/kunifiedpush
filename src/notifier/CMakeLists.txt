# SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(KUnifiedPushNotifier STATIC)
target_sources(KUnifiedPushNotifier PRIVATE
    contentencryptor.cpp
    notifier.cpp
    vapid.cpp
    ../shared/contentencryptionutils.cpp
    ../shared/eckey.cpp
)
set_target_properties(KUnifiedPushNotifier PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(KUnifiedPushNotifier PRIVATE
    Qt6::Network
    OpenSSL::Crypto
)
