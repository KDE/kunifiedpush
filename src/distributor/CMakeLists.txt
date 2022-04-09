# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_executable(kunifiedpush-distributor)

qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.unifiedpush.Distributor1.xml distributor.h KUnifiedPush::Distributor)
qt_add_dbus_interface(dbus_srcs ../interfaces/org.unifiedpush.Connector1.xml connector1iface)
target_sources(kunifiedpush-distributor PRIVATE
    abstractpushprovider.cpp
    client.cpp
    distributor.cpp
    gotifypushprovider.cpp
    main.cpp
    message.cpp
    ${dbus_srcs}
)

ecm_qt_declare_logging_category(kunifiedpush-distributor
    HEADER logging.h
    IDENTIFIER Log
    CATEGORY_NAME org.kde.kunifiedpush.distributor
    DESCRIPTION "KUnifiedPush Distributor"
    EXPORT KUnifiedPushLogging
)

target_link_libraries(kunifiedpush-distributor PRIVATE
    Qt::Core
    Qt::DBus
    Qt::WebSockets
)

install(TARGETS kunifiedpush-distributor ${INSTALL_TARGETS_DEFAULT_ARGS})
