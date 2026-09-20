/*
 * SPDX-FileCopyrightText: 2019 boud <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TESTAGFILTERTRESOURCEPROXYMODEL_H
#define TESTAGFILTERTRESOURCEPROXYMODEL_H

#include <QObject>

class TestStorageFilterProxyModel : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void filtersStorageRowsForChooser();
    void updatesVisibleRowsWhenFilterChanges();
};

#endif
