# SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_test(connectorutilstest.cpp LINK_LIBRARIES Qt::Test Qt::DBus)
if (NOT KUNIFIEDPUSH_CLIENT_ONLY)
    ecm_add_test(connectortest.cpp LINK_LIBRARIES Qt::Test KUnifiedPush KUnifiedPushDistributor)
    ecm_add_test(serversenteventsstreamtest.cpp LINK_LIBRARIES Qt::Test KUnifiedPushDistributor)
endif()
