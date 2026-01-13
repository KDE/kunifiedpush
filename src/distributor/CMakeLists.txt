# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(KUnifiedPushDistributor STATIC)

qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.unifiedpush.Distributor1.xml distributor.h KUnifiedPush::Distributor)
qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.unifiedpush.Distributor2.xml distributor.h KUnifiedPush::Distributor)
qt_add_dbus_interface(dbus_srcs ../interfaces/org.unifiedpush.Connector1.xml connector1iface)
qt_add_dbus_interface(dbus_srcs ../interfaces/org.unifiedpush.Connector2.xml connector2iface)
qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.kde.kunifiedpush.distributor.Management.xml distributor.h KUnifiedPush::Distributor)
target_sources(KUnifiedPushDistributor PRIVATE
    abstractpushprovider.cpp
    abstractpushprovider.h
    autopushprovider.cpp
    client.cpp
    client.h
    command.cpp
    command.h
    distributor.cpp
    distributor.h
    gotifypushprovider.cpp
    gotifypushprovider.h
    message.cpp
    message.h
    mockpushprovider.cpp
    mockpushprovider.h
    nextpushprovider.cpp
    nextpushprovider.h
    ntfypushprovider.cpp ntfypushprovider.h
    serversenteventsstream.cpp
    serversenteventsstream.h
    ${dbus_srcs}
)
ecm_qt_declare_logging_category(KUnifiedPushDistributor
    HEADER logging.h
    IDENTIFIER Log
    CATEGORY_NAME org.kde.kunifiedpush.distributor
    DESCRIPTION "KUnifiedPush Distributor"
    EXPORT KUnifiedPushLogging
)
target_link_libraries(KUnifiedPushDistributor PUBLIC
    KF6::Solid
    KF6::CoreAddons
    KF6::Crash
    Qt::Core
    Qt::DBus
    Qt::WebSockets
)

add_executable(kunifiedpush-distributor)
target_sources(kunifiedpush-distributor PRIVATE
    main.cpp
)

target_include_directories(kunifiedpush-distributor PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(kunifiedpush-distributor PRIVATE KUnifiedPushDistributor)

install(TARGETS kunifiedpush-distributor ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kunifiedpush-distributor.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

configure_file(kunifiedpush-distributor.service.in ${CMAKE_CURRENT_BINARY_DIR}/kunifiedpush-distributor.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kunifiedpush-distributor.service DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
install(CODE "
    include(${CMAKE_SOURCE_DIR}/cmake/SystemctlEnable.cmake)
    systemctl_enable(kunifiedpush-distributor.service graphical-session.target ${KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR})
")
