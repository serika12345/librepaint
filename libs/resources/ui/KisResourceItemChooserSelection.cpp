/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooser.h"

#include "KisResourceItemChooserSelectionSource_p.h"
#include "KisResourceItemChooser_p.h"

KoResourceSP KisResourceItemChooser::currentResource(bool includeHidden) const
{
    if (includeHidden ||
        KisResourceItemChooserSelectionSource::currentIndexIsSelected(d->view)) {
        return d->currentResource;
    }

    return nullptr;
}

void KisResourceItemChooser::setCurrentResource(KoResourceSP resource)
{
    if (d->updatesBlocked) {
        return;
    }

    const QModelIndex index =
        KisResourceItemChooserSelectionSource::indexForResource(
            d->tagFilterProxyModel, resource);
    KisResourceItemChooserSelectionSource::setCurrentIndex(d->view, index);

    if (!index.isValid()) {
        d->currentResource = resource;
    }
    updatePreview(index);
}

void KisResourceItemChooser::setCurrentResource(QString resourceName)
{
    if (d->updatesBlocked) {
        return;
    }

    for (int row = 0;
         row < KisResourceItemChooserSelectionSource::rowCount(
                   d->tagFilterProxyModel);
         ++row) {
        for (int column = 0;
             column < KisResourceItemChooserSelectionSource::columnCount(
                          d->tagFilterProxyModel);
             ++column) {
            const QModelIndex index =
                KisResourceItemChooserSelectionSource::index(
                    d->tagFilterProxyModel, row, column);
            const KoResourceSP resource =
                KisResourceItemChooserSelectionSource::resourceForIndex(
                    d->tagFilterProxyModel, index);

            if (KisResourceItemChooserSelectionSource::resourceName(resource) ==
                resourceName) {
                KisResourceItemChooserSelectionSource::setCurrentIndex(d->view,
                                                                        index);
                if (!index.isValid()) {
                    d->currentResource = resource;
                }
                updatePreview(index);
            }
        }
    }
}

void KisResourceItemChooser::setPreviewTiled(bool tiled)
{
    d->tiledPreview = tiled;
}

void KisResourceItemChooser::setGrayscalePreview(bool grayscale)
{
    d->grayscalePreview = grayscale;
}

void KisResourceItemChooser::setCurrentItem(int row)
{
    const QModelIndex index =
        KisResourceItemChooserSelectionSource::indexForRow(d->view, row);
    if (!index.isValid()) {
        return;
    }

    KisResourceItemChooserSelectionSource::setCurrentIndex(d->view, index);
    updatePreview(index);
}

void KisResourceItemChooser::activate(const QModelIndex &index)
{
    if (!index.isValid()) {
        updateButtonState();
        return;
    }

    const KoResourceSP resource = resourceFromModelIndex(index);
    if (resource &&
        KisResourceItemChooserSelectionSource::resourceIsValid(resource)) {
        if (resource != d->currentResource) {
            d->currentResource = resource;
            d->updatesBlocked = true;
            Q_EMIT resourceSelected(resource);
            d->updatesBlocked = false;
        }
        updatePreview(index);
        updateButtonState();
    }
}

void KisResourceItemChooser::clicked(const QModelIndex &index)
{
    Q_UNUSED(index);

    const KoResourceSP resource = currentResource();
    if (resource) {
        Q_EMIT resourceClicked(resource);
    }
}

KoResourceSP KisResourceItemChooser::resourceFromModelIndex(
    const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }
    return KisResourceItemChooserSelectionSource::resourceForIndex(
        d->tagFilterProxyModel, index);
}
