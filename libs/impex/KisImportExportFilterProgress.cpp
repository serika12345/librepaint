/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter_p.h"

#include <KoUpdater.h>

KisImportExportFilter::~KisImportExportFilter()
{
    if (d->updater) {
        d->updater->setProgress(100);
    }
    delete d;
}

void KisImportExportFilter::setUpdater(QPointer<KoUpdater> updater)
{
    d->updater = updater;
}

QPointer<KoUpdater> KisImportExportFilter::updater()
{
    return d->updater;
}

void KisImportExportFilter::setProgress(int value)
{
    if (d->updater) {
        d->updater->setValue(value);
    }
}
