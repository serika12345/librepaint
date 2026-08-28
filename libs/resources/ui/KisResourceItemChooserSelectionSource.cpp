/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooserSelectionSource_p.h"

#include "KisResourceItemListView.h"

#include <KisTagFilterResourceProxyModel.h>

#include <QItemSelectionModel>

namespace KisResourceItemChooserSelectionSource
{
bool currentIndexIsSelected(KisResourceItemListView *view)
{
    return view->selectionModel()->isSelected(view->currentIndex());
}

QModelIndex indexForResource(KisTagFilterResourceProxyModel *model,
                             KoResourceSP resource)
{
    return model->indexForResource(resource);
}

void setCurrentIndex(KisResourceItemListView *view, const QModelIndex &index)
{
    view->setCurrentIndex(index);
}

int rowCount(KisTagFilterResourceProxyModel *model)
{
    return model->rowCount();
}

int columnCount(KisTagFilterResourceProxyModel *model)
{
    return model->columnCount();
}

QModelIndex index(KisTagFilterResourceProxyModel *model, int row, int column)
{
    return model->index(row, column);
}

KoResourceSP resourceForIndex(KisTagFilterResourceProxyModel *model,
                              const QModelIndex &index)
{
    return model->resourceForIndex(index);
}

QString resourceName(KoResourceSP resource)
{
    return resource->name();
}

QModelIndex indexForRow(KisResourceItemListView *view, int row)
{
    return view->model()->index(row, 0);
}

bool resourceIsValid(KoResourceSP resource)
{
    return resource->valid();
}
}
