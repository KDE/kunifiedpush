# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_push_notifications\")

set(kcm_srcs
    clientmodel.cpp
    clientmodel.h
    kcmpushnotifications.cpp
    kcmpushnotifications.h
    nextcloudauthenticator.cpp
    nextcloudauthenticator.h
    ../shared/connectorutils.cpp
)

set_source_files_properties(../interfaces/org.kde.kunifiedpush.distributor.Management.xml PROPERTIES INCLUDE ../shared/clientinfo_p.h)
qt_add_dbus_interface(kcm_srcs ../interfaces/org.kde.kunifiedpush.distributor.Management.xml managementinterface)

kcoreaddons_add_plugin(kcm_push_notifications SOURCES ${kcm_srcs} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
target_link_libraries(kcm_push_notifications PRIVATE
    KF6::Service
    KF6::KCMUtilsQuick
    Qt::DBus
    Qt::Gui
)

kpackage_install_package(package kcm_push_notifications kcms)
