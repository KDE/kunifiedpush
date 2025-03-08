# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "03")
set (RELEASE_SERVICE_VERSION_MICRO "80")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(KUnifiedPush VERSION ${RELEASE_SERVICE_VERSION})

option(KUNIFIEDPUSH_CLIENT_ONLY "Build only the application client library." FALSE)

set(KF_MIN_VERSION 6.3.0)
set(QT_MIN_VERSION 6.5)
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddTests)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMDeprecationSettings)

ecm_setup_version(PROJECT
    VARIABLE_PREFIX KUNIFIEDPUSH
    VERSION_HEADER kunifiedpush_version.h
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KUnifiedPushConfigVersion.cmake"
    SOVERSION 1
)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Test)
if (NOT ANDROID)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus)

    # for the KCM
    if (NOT KUNIFIEDPUSH_CLIENT_ONLY)
        find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS WebSockets)
        find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n Service KCMUtils)
    endif()
else()
    find_package(Gradle REQUIRED)
endif()

set(KUNIFIEDPUSH_INCLUDE_INSTALLDIR ${KDE_INSTALL_INCLUDEDIR}/KUnifiedPush)
ecm_set_disabled_deprecation_versions(QT 6.7 KF 6.7.0)

add_subdirectory(src)
if (BUILD_TESTING AND NOT ANDROID)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

set(KUNIFIEDPUSH_INSTALL_CMAKECONFIGDIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KUnifiedPush")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KUnifiedPushConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KUnifiedPushConfig.cmake"
    INSTALL_DESTINATION "${KUNIFIEDPUSH_INSTALL_CMAKECONFIGDIR}"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/KUnifiedPushConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/KUnifiedPushConfigVersion.cmake"
    DESTINATION "${KUNIFIEDPUSH_INSTALL_CMAKECONFIGDIR}"
    COMPONENT Devel
)
install(EXPORT KUnifiedPushTargets DESTINATION "${KUNIFIEDPUSH_INSTALL_CMAKECONFIGDIR}" FILE KUnifiedPushTargets.cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kunifiedpush_version.h DESTINATION ${KUNIFIEDPUSH_INCLUDE_INSTALLDIR} COMPONENT Devel)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/default.conf DESTINATION ${KDE_INSTALL_FULL_CONFDIR}/KDE RENAME kunifiedpush-distributor.conf)
ecm_qt_install_logging_categories(
    EXPORT KUnifiedPushLogging
    FILE org_kde_kunifiedpush.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
if (NOT ANDROID AND NOT KUNIFIEDPUSH_CLIENT_ONLY)
    ki18n_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
