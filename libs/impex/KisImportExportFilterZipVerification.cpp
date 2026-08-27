/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter.h"
#include "KisImportExportMimeType.h"

#include <KoStore.h>
#include <QScopedPointer>
#include <klocalizedstring.h>

QString KisImportExportFilter::verifyZiPBasedFiles(const QString &fileName, const QStringList &filesToCheck) const
{
    QScopedPointer<KoStore> store(KoStore::createStore(fileName, KoStore::Read, KIS_MIME_TYPE, KoStore::Zip));

    if (!store || store->bad()) {
        return i18n("Could not open the saved file %1. Please try to save again in a different location.", fileName);
    }

    for (const QString &file : filesToCheck) {
        if (!store->hasFile(file)) {
            return i18n("Component %1 is missing in %2. Please try to save again in a different location.",
                        file,
                        fileName);
        }
    }

    return QString();
}
