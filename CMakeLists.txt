# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "04")
set (RELEASE_SERVICE_VERSION_MICRO "00")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(KUnifiedPush VERSION ${RELEASE_SERVICE_VERSION})

set(KF5_MIN_VERSION 5.94)
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
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
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()

ecm_setup_version(PROJECT
    VARIABLE_PREFIX KUNIFIEDPUSH
    VERSION_HEADER kunifiedpush_version.h
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KUnifiedPushConfigVersion.cmake"
    SOVERSION 1
)

set(QT_MIN_VERSION 5.15)
find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Test WebSockets)
if (NOT ANDROID)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus)

    # for the KCM
    find_package(KF${KF_MAJOR_VERSION} ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Declarative Service Package)
else()
    find_package(Gradle REQUIRED)
    if (QT_MAJOR_VERSION EQUAL "5")
        find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED AndroidExtras)
    endif()
endif()

set(KUNIFIEDPUSH_INCLUDE_INSTALLDIR ${KDE_INSTALL_INCLUDEDIR}/KUnifiedPush)
ecm_set_disabled_deprecation_versions(QT 6.4 KF 5.102.0)

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
ecm_qt_install_logging_categories(
    EXPORT KUnifiedPushLogging
    FILE org_kde_kunifiedpush.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
