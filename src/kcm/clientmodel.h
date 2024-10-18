/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include "managementinterface.h"

#include <QAbstractListModel>

/** Model for all registered push notification client. */
class ClientModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ClientModel(org::kde::kunifiedpush::Management *iface, QObject *parent = nullptr);
    ~ClientModel();

    enum Role {
        NameRole = Qt::DisplayRole,
        DescriptionRole = Qt::UserRole,
        IconNameRole,
        TokenRole,
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void reload();

private:
    org::kde::kunifiedpush::Management *const m_iface;
    QList<KUnifiedPush::ClientInfo> m_clients;

};

#endif // CLIENTMODEL_H
