# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(KUnifiedPush)

target_sources(KUnifiedPush PRIVATE
    connector.cpp
    connector.h
    contentencryption.cpp
    ../shared/connectorutils.cpp
)
if (NOT ANDROID)
    qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.unifiedpush.Connector1.xml connector_p.h KUnifiedPush::ConnectorPrivate)
    qt_add_dbus_adaptor(dbus_srcs  ../interfaces/org.unifiedpush.Connector2.xml connector_p.h KUnifiedPush::ConnectorPrivate)
    qt_add_dbus_interface(dbus_srcs ../interfaces/org.unifiedpush.Distributor1.xml distributor1iface)
    qt_add_dbus_interface(dbus_srcs ../interfaces/org.unifiedpush.Distributor2.xml distributor2iface)
    qt_add_dbus_interface(dbus_srcs ../interfaces/org.freedesktop.DBus.Introspectable.xml introspectiface)
    target_sources(KUnifiedPush PRIVATE ${dbus_srcs} connector_dbus.cpp)
else()
    add_subdirectory(android)
    target_sources(KUnifiedPush PRIVATE connector_android.cpp)
endif()

set_target_properties(KUnifiedPush PROPERTIES
    VERSION ${KUNIFIEDPUSH_VERSION}
    SOVERSION ${KUNIFIEDPUSH_SOVERSION}
    EXPORT_NAME KUnifiedPush
)

generate_export_header(KUnifiedPush BASE_NAME KUnifiedPush)
ecm_qt_declare_logging_category(KUnifiedPush
    HEADER logging.h
    IDENTIFIER Log
    CATEGORY_NAME org.kde.kunifiedpush.client
    DESCRIPTION "KUnifiedPush Client"
    EXPORT KUnifiedPushLogging
)

target_include_directories(KUnifiedPush INTERFACE "$<INSTALL_INTERFACE:${KUNIFIEDPUSH_INCLUDE_INSTALLDIR}>")
target_link_libraries(KUnifiedPush PUBLIC Qt6::Core)
target_link_libraries(KUnifiedPush PRIVATE OpenSSL::Crypto)
if (NOT ANDROID)
    target_link_libraries(KUnifiedPush PRIVATE Qt6::DBus)
endif()

ecm_generate_headers(KUnifiedPush_FORWARDING_HEADERS
    HEADER_NAMES
        Connector
    PREFIX KUnifiedPush
    REQUIRED_HEADERS KUnifiedPush_HEADERS
)

install(TARGETS KUnifiedPush EXPORT KUnifiedPushTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES
    ${KUnifiedPush_FORWARDING_HEADERS}
    DESTINATION ${KUNIFIEDPUSH_INCLUDE_INSTALLDIR}/KUnifiedPush
)
install(FILES
    ${KUnifiedPush_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/kunifiedpush_export.h
    DESTINATION ${KUNIFIEDPUSH_INCLUDE_INSTALLDIR}/kunifiedpush
)
