/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter.h"

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <klocalizedstring.h>

QString KisImportExportFilter::verify(const QString &fileName) const
{
    const QFileInfo fileInfo(fileName);

    if (!fileInfo.exists()) {
        return i18n("%1 does not exist after writing. Try saving again under a different name, in another location.",
                    fileName);
    }

    if (!fileInfo.isReadable()) {
        return i18n("%1 is not readable", fileName);
    }

    if (fileInfo.size() < 10) {
        return i18n(
            "%1 is smaller than 10 bytes, it must be corrupt. Try saving again under a different name, in "
            "another location.",
            fileName);
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        return i18n("%1 could not be opened", fileName);
    }
    const QByteArray data = file.read(std::min(file.size(), qint64(1000)));
    bool foundNonZeroByte = false;
    for (const char byte : data) {
        if (byte > 0) {
            foundNonZeroByte = true;
            break;
        }
    }

    if (!foundNonZeroByte) {
        return i18n(
            "%1 has only zero bytes in the first 1000 bytes, it's probably corrupt. Try saving again under a "
            "different name, in another location.",
            fileName);
    }

    return QString();
}
