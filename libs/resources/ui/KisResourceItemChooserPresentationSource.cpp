/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooserPresentationSource_p.h"

#include "KisPopupButton.h"
#include "KisResourceItemListView.h"
#include "KisResourceTaggingManager.h"
#include "KisStorageChooserWidget.h"
#include "KisTagChooserWidget.h"

#include <KoIcon.h>

#include <QFrame>
#include <QSplitter>
#include <QToolButton>

namespace KisResourceItemChooserPresentationSource
{
void setListViewMode(KisResourceItemListView *view, ListViewMode mode)
{
    view->setListViewMode(mode);
}

void showTaggingBar(KisResourceTaggingManager *manager, bool visible)
{
    manager->showTaggingBar(visible);
}

void showViewModeButton(KisPopupButton *button, bool visible)
{
    button->setVisible(visible);
}

void showStorageButton(KisStorageChooserWidget *button, bool visible)
{
    button->setVisible(visible);
}

void setRowHeight(KisResourceItemListView *view, int height)
{
    view->setItemSize(QSize(view->gridSize().width(), height));
}

void setColumnWidth(KisResourceItemListView *view, int width)
{
    view->setItemSize(QSize(width, view->gridSize().height()));
}

void setItemDelegate(KisResourceItemListView *view,
                     QAbstractItemDelegate *delegate)
{
    view->setItemDelegate(delegate);
}

void showImportExportButtons(QFrame *frame, bool visible)
{
    frame->setVisible(visible);
}

void setPreviewOrientation(QSplitter *splitter, Qt::Orientation orientation)
{
    splitter->setOrientation(orientation);
}

QSize viewSize(KisResourceItemListView *view)
{
    return view->size();
}

void updateIcons(KisPopupButton *viewModeButton,
                 QToolButton *importButton,
                 QToolButton *deleteButton,
                 KisStorageChooserWidget *storageButton,
                 KisResourceTaggingManager *tagManager)
{
    viewModeButton->setIcon(KisIconUtils::loadIcon("view-choose"));
    importButton->setIcon(koIcon("document-import-16"));
    deleteButton->setIcon(koIcon("edit-delete"));
    storageButton->setIcon(koIcon("bundle_archive"));
    tagManager->tagChooserWidget()->updateIcons();
}
}
