# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_test(connectorutilstest.cpp LINK_LIBRARIES Qt::Test Qt::DBus)
ecm_add_test(contentencryptiontest.cpp LINK_LIBRARIES Qt::Test Qt::DBus KUnifiedPush)
if (NOT KUNIFIEDPUSH_CLIENT_ONLY)
    add_executable(mockdistributor mockdistributor.cpp)
    target_link_libraries(mockdistributor PRIVATE KUnifiedPushDistributor)

    set_source_files_properties(../src/interfaces/org.kde.kunifiedpush.distributor.Management.xml PROPERTIES INCLUDE ../src/shared/clientinfo_p.h)
    qt_add_dbus_interface(dbus_srcs ../src/interfaces/org.kde.kunifiedpush.distributor.Management.xml managementinterface)
    ecm_add_test(connectortest.cpp ${dbus_srcs} TEST_NAME connectortest LINK_LIBRARIES Qt::Test KUnifiedPush KUnifiedPushDistributor)
    ecm_add_test(serversenteventsstreamtest.cpp LINK_LIBRARIES Qt::Test KUnifiedPushDistributor)

    ecm_add_test(contentencryptortest.cpp LINK_LIBRARIES Qt::Test KUnifiedPushNotifier KUnifiedPush)
    ecm_add_test(vapidtest.cpp LINK_LIBRARIES Qt::Test KUnifiedPushNotifier)
endif()
