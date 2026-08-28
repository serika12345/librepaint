/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEITEMCHOOSERSELECTIONSOURCE_P_H
#define KISRESOURCEITEMCHOOSERSELECTIONSOURCE_P_H

#include <KoResource.h>

#include <QModelIndex>

class KisResourceItemListView;
class KisTagFilterResourceProxyModel;

namespace KisResourceItemChooserSelectionSource
{
bool currentIndexIsSelected(KisResourceItemListView *view);
QModelIndex indexForResource(KisTagFilterResourceProxyModel *model,
                             KoResourceSP resource);
void setCurrentIndex(KisResourceItemListView *view, const QModelIndex &index);
int rowCount(KisTagFilterResourceProxyModel *model);
int columnCount(KisTagFilterResourceProxyModel *model);
QModelIndex index(KisTagFilterResourceProxyModel *model, int row, int column);
KoResourceSP resourceForIndex(KisTagFilterResourceProxyModel *model,
                              const QModelIndex &index);
QString resourceName(KoResourceSP resource);
QModelIndex indexForRow(KisResourceItemListView *view, int row);
bool resourceIsValid(KoResourceSP resource);
}

#endif // KISRESOURCEITEMCHOOSERSELECTIONSOURCE_P_H
