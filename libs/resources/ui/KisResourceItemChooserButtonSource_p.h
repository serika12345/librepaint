/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEITEMCHOOSERBUTTONSOURCE_P_H
#define KISRESOURCEITEMCHOOSERBUTTONSOURCE_P_H

#include <KoResource.h>

#include <QModelIndex>
#include <QStringList>

class QWidget;
class KisResourceItemListView;
class KisTagFilterResourceProxyModel;

namespace KisResourceItemChooserButtonSource
{
QStringList mimeTypes(const QString &resourceType);
QStringList importFilenames(const QStringList &mimeTypes);
bool fileIsReadable(const QString &filename);
KoResourceSP importResource(QWidget *parent,
                            const QString &resourceType,
                            const QString &filename);
QModelIndex indexForResource(KisTagFilterResourceProxyModel *model,
                             KoResourceSP resource);
QModelIndex currentIndex(KisResourceItemListView *view);
void setResourceInactive(KisTagFilterResourceProxyModel *model,
                         const QModelIndex &index);
QModelIndex index(KisTagFilterResourceProxyModel *model, int row, int column);
void sort(KisTagFilterResourceProxyModel *model, int role);
}

#endif // KISRESOURCEITEMCHOOSERBUTTONSOURCE_P_H
