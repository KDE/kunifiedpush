# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(KUnifiedPushDistributor STATIC)

qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.unifiedpush.Distributor1.xml distributor.h KUnifiedPush::Distributor)
qt_add_dbus_interface(dbus_srcs ../interfaces/org.unifiedpush.Connector1.xml connector1iface)
target_sources(KUnifiedPushDistributor PRIVATE
    abstractpushprovider.cpp
    client.cpp
    command.cpp
    distributor.cpp
    gotifypushprovider.cpp
    message.cpp
    nextpushprovider.cpp
    serversenteventsstream.cpp
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
    Qt::Core
    Qt::DBus
    Qt::WebSockets
)

add_executable(kunifiedpush-distributor)
target_sources(kunifiedpush-distributor PRIVATE
    main.cpp
)

target_link_libraries(kunifiedpush-distributor PRIVATE KUnifiedPushDistributor)

install(TARGETS kunifiedpush-distributor ${INSTALL_TARGETS_DEFAULT_ARGS})
