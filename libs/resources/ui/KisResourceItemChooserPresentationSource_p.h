/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEITEMCHOOSERPRESENTATIONSOURCE_P_H
#define KISRESOURCEITEMCHOOSERPRESENTATIONSOURCE_P_H

#include "ResourceListViewModes.h"

#include <QSize>

class QAbstractItemDelegate;
class QFrame;
class QSplitter;
class QToolButton;
class KisPopupButton;
class KisResourceItemListView;
class KisResourceTaggingManager;
class KisStorageChooserWidget;

namespace KisResourceItemChooserPresentationSource
{
void setListViewMode(KisResourceItemListView *view, ListViewMode mode);
void showTaggingBar(KisResourceTaggingManager *manager, bool visible);
void showViewModeButton(KisPopupButton *button, bool visible);
void showStorageButton(KisStorageChooserWidget *button, bool visible);
void setRowHeight(KisResourceItemListView *view, int height);
void setColumnWidth(KisResourceItemListView *view, int width);
void setItemDelegate(KisResourceItemListView *view,
                     QAbstractItemDelegate *delegate);
void showImportExportButtons(QFrame *frame, bool visible);
void setPreviewOrientation(QSplitter *splitter, Qt::Orientation orientation);
QSize viewSize(KisResourceItemListView *view);
void updateIcons(KisPopupButton *viewModeButton,
                 QToolButton *importButton,
                 QToolButton *deleteButton,
                 KisStorageChooserWidget *storageButton,
                 KisResourceTaggingManager *tagManager);
}

#endif // KISRESOURCEITEMCHOOSERPRESENTATIONSOURCE_P_H
