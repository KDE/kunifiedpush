# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

gradle_add_aar(kunifiedpush_aar BUILDFILE ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle NAME KUnifiedPush)
gradle_install_aar(kunifiedpush_aar DESTINATION jar)

install(FILES KUnifiedPush-android-dependencies.xml
        DESTINATION ${KDE_INSTALL_LIBDIR}
        RENAME KUnifiedPush_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml
)
