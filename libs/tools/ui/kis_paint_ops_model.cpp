/*
 *  SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paint_ops_model.h"
#include "kis_paint_ops_model_source_p.h"

namespace Source = KisPaintOpsModelSource;

KisPaintOpListModel::KisPaintOpListModel(QObject *parent)
    : BasePaintOpCategorizedListModel(parent)
{
}

QVariant KisPaintOpListModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid()) return QVariant();

    DataItem *item = categoriesMapper()->itemFromRow(idx.row());
    Q_ASSERT(item);

    if(role == Qt::DecorationRole) {
        if (!item->isCategory()) {
            return item->data()->icon;
        }
    } else if (role == SortRole) {
        return item->isCategory() ? item->name() :
            QString("%1%2%3")
            .arg(item->parentCategory()->name())
            .arg(item->data()->priority, 4)
            .arg(item->name());
    }

    return BasePaintOpCategorizedListModel::data(idx, role);
}

void KisPaintOpListModel::fill(const QList<KisPaintOpFactory*>& list)
{
    Q_FOREACH (KisPaintOpFactory *factory, list) {
        const Source::PaintOpState state = Source::paintOpState(factory);

        categoriesMapper()->addEntry(state.category,
                                     KisPaintOpInfo(state.id,
                                                    state.name,
                                                    state.category,
                                                    state.icon,
                                                    state.priority));
    }
    categoriesMapper()->expandAllCategories();
}

KisSortedPaintOpListModel::KisSortedPaintOpListModel(QObject *parent)
    : KisSortedCategorizedListModel<KisPaintOpListModel>(parent)
    , m_model(new KisPaintOpListModel(this))
{
    initializeModel(m_model);
}

void KisSortedPaintOpListModel::fill(const QList<KisPaintOpFactory*> &list)
{
    m_model->fill(list);
}

bool KisSortedPaintOpListModel::lessThan(const QModelIndex &left,
                                         const QModelIndex &right) const
{
    return lessThanPriority(left, right, Source::stableCategory());
}
