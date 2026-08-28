/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooser.h"

#include "KisResourceItemChooserButtonSource_p.h"
#include "KisResourceItemChooser_p.h"

void KisResourceItemChooser::slotButtonClicked(int button)
{
    if (button == Button_Import) {
        const QStringList mimeTypes =
            KisResourceItemChooserButtonSource::mimeTypes(d->resourceType);
        const QStringList filenames =
            KisResourceItemChooserButtonSource::importFilenames(mimeTypes);

        for (const QString &filename : filenames) {
            if (!KisResourceItemChooserButtonSource::fileIsReadable(filename)) {
                continue;
            }

            const KoResourceSP previousResource = currentResource();
            const KoResourceSP newResource =
                KisResourceItemChooserButtonSource::importResource(
                    this, d->resourceType, filename);

            if (previousResource && newResource && !currentResource()) {
                setCurrentResource(newResource);
            } else if (currentResource() == newResource) {
                const QModelIndex index =
                    KisResourceItemChooserButtonSource::indexForResource(
                        d->tagFilterProxyModel, newResource);
                updatePreview(index);
            }
        }
        KisResourceItemChooserButtonSource::sort(d->tagFilterProxyModel,
                                                 Qt::DisplayRole);
    } else if (button == Button_Remove) {
        const QModelIndex index =
            KisResourceItemChooserButtonSource::currentIndex(d->view);
        if (index.isValid()) {
            KisResourceItemChooserButtonSource::setResourceInactive(
                d->tagFilterProxyModel, index);
        }
        int row = index.row();
        const int previousRow = --row;
        row = qBound(0, previousRow, row);
        setCurrentItem(row);
        activate(KisResourceItemChooserButtonSource::index(
            d->tagFilterProxyModel, row, index.column()));
    }
    updateButtonState();
}
