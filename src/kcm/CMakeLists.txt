# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

set(kcm_srcs
    kcmpushnotifications.cpp
    ../shared/connectorutils.cpp
)

kcoreaddons_add_plugin(kcm_push_notifications SOURCES ${kcm_srcs} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
target_link_libraries(kcm_push_notifications PRIVATE
    KF5::QuickAddons
    Qt::DBus
)

kpackage_install_package(package kcm_push_notifications kcms)
