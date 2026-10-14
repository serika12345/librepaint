/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KRA_REFERENCE_READER_H
#define KRA_REFERENCE_READER_H

#include <document/KisDocument.h>
#include <KisImportExportManager.h>

inline bool readWithExistingKraPlugin(KisDocument *document, const QString &path)
{
    document->setFileBatchMode(true);
    return document->importExportManager()->importDocument(path, KisDocument::nativeFormatMimeType()).isOk();
}

#endif
