/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_FILE_TRANSFER_H
#define KIS_IMPORT_EXPORT_FILE_TRANSFER_H

#include <QFileDevice>
#include <QString>

#include "kritaimpex_export.h"

namespace Krita::ImportExport
{

struct KRITAIMPEX_EXPORT KisImportExportFileTransferResult
{
    bool success = false;
    QFileDevice::FileError error = QFileDevice::NoError;
    QString errorMessage;
};

class KRITAIMPEX_EXPORT KisImportExportFileTransfer
{
public:
    /**
     * Replaces the contents of an existing target through that target's own
     * file handle. The source remains available for recovery. This supports
     * destinations for which the operating system grants access to one file
     * without granting access to its containing directory.
     */
    static KisImportExportFileTransferResult replaceFromLocalFile(const QString &sourcePath,
                                                                   const QString &targetPath);
};

}

#endif
