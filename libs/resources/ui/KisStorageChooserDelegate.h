/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISSTORAGECHOOSERDELEGATE_H
#define KISSTORAGECHOOSERDELEGATE_H

#include <QAbstractItemDelegate>

#include "kritaresourceui_export.h"

class KRITARESOURCEUI_EXPORT KisStorageChooserDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit KisStorageChooserDelegate(QObject *parent = 0);
    ~KisStorageChooserDelegate() override {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;
};

#endif // KISSTORAGECHOOSERDELEGATE_H
