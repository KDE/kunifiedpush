/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "clientmodel.h"

#include <KService>

ClientModel::ClientModel(org::kde::kunifiedpush::Management *iface, QObject *parent)
    : QAbstractListModel(parent)
    , m_iface(iface)
{
    m_clients = iface->registeredClients();
    connect(iface, &org::kde::kunifiedpush::Management::registeredClientsChanged, this, &ClientModel::reload);
}

ClientModel::~ClientModel() = default;

int ClientModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_clients.size();
}

QVariant ClientModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index)) {
        return {};
    }

    const auto &c = m_clients.at(index.row());
    const auto service = KService::serviceByDesktopName(c.serviceName);
    switch (role) {
        case NameRole:
            return service ? service->name() : c.serviceName;
        case DescriptionRole:
            return c.description;
        case IconNameRole:
            return service ? service->icon() : QStringLiteral("application-x-executable");
        case TokenRole:
            return c.token;
    }

    return {};
}

QHash<int, QByteArray> ClientModel::roleNames() const
{
    auto n = QAbstractListModel::roleNames();
    n.insert(NameRole, "name");
    n.insert(DescriptionRole, "description");
    n.insert(IconNameRole, "iconName");
    n.insert(TokenRole, "token");
    return n;
}

void ClientModel::reload()
{
    beginResetModel();
    m_clients = m_iface->registeredClients();
    endResetModel();
}
