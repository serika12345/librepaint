/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooser.h"

#include "KisResourceItemChooserPresentationSource_p.h"
#include "KisResourceItemChooser_p.h"

void KisResourceItemChooser::setResponsiveness(bool isResponsive)
{
    if (isResponsive && !d->isResponsive) {
        d->isResponsive = true;
        changeLayoutBasedOnSize();
    } else {
        d->isResponsive = isResponsive;
    }
}

void KisResourceItemChooser::setListViewMode(ListViewMode newViewMode)
{
    d->requestedViewMode = newViewMode;
    if (d->layout == Layout::NotSet || d->layout == Layout::Vertical) {
        KisResourceItemChooserPresentationSource::setListViewMode(d->view,
                                                                  newViewMode);
    }
}

KisTagFilterResourceProxyModel *KisResourceItemChooser::tagFilterModel() const
{
    return d->tagFilterProxyModel;
}

void KisResourceItemChooser::showImportExportBtns(bool show)
{
    KisResourceItemChooserPresentationSource::showImportExportButtons(
        d->importExportBtns, show);
}

void KisResourceItemChooser::showTaggingBar(bool show)
{
    KisResourceItemChooserPresentationSource::showTaggingBar(d->tagManager,
                                                              show);
}

void KisResourceItemChooser::setRowHeight(int rowHeight)
{
    KisResourceItemChooserPresentationSource::setRowHeight(d->view, rowHeight);
}

void KisResourceItemChooser::setColumnWidth(int columnWidth)
{
    KisResourceItemChooserPresentationSource::setColumnWidth(d->view,
                                                              columnWidth);
}

void KisResourceItemChooser::setItemDelegate(QAbstractItemDelegate *delegate)
{
    KisResourceItemChooserPresentationSource::setItemDelegate(d->view,
                                                               delegate);
}

void KisResourceItemChooser::setPreviewOrientation(Qt::Orientation orientation)
{
    KisResourceItemChooserPresentationSource::setPreviewOrientation(
        d->resourcesSplitter, orientation);
}

QSize KisResourceItemChooser::viewSize() const
{
    return KisResourceItemChooserPresentationSource::viewSize(d->view);
}

KisResourceItemListView *KisResourceItemChooser::itemView() const
{
    return d->view;
}

void KisResourceItemChooser::showStorageBtn(bool visible)
{
    KisResourceItemChooserPresentationSource::showStorageButton(
        d->storagePopupButton, visible);
    d->showStoragePopupBtn = visible;
}

void KisResourceItemChooser::showViewModeBtn(bool visible)
{
    KisResourceItemChooserPresentationSource::showViewModeButton(
        d->viewModeButton, visible);
    d->showViewModeBtn = visible;
}

KisPopupButton *KisResourceItemChooser::viewModeButton() const
{
    return d->viewModeButton;
}

void KisResourceItemChooser::updateView()
{
    KisResourceItemChooserPresentationSource::updateIcons(
        d->viewModeButton,
        d->importButton,
        d->deleteButton,
        d->storagePopupButton,
        d->tagManager);
}
