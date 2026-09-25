/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportFileTransfer.h"

#include <QFile>

namespace Krita::ImportExport
{

namespace
{

KisImportExportFileTransferResult failure(
    const QFile &file,
    const QString &operation,
    const QString &path,
    QFileDevice::FileError fallbackError = QFileDevice::UnspecifiedError)
{
    const QFileDevice::FileError error = file.error() == QFileDevice::NoError
        ? fallbackError
        : file.error();
    return {false,
            error,
            QStringLiteral("%1 '%2': %3").arg(operation, path, file.errorString())};
}

}

KisImportExportFileTransferResult KisImportExportFileTransfer::replaceFromLocalFile(
    const QString &sourcePath,
    const QString &targetPath)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        return failure(source,
                       QStringLiteral("Failed to open completed document"),
                       sourcePath,
                       QFileDevice::OpenError);
    }

    QFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return failure(target,
                       QStringLiteral("Failed to open selected document"),
                       targetPath,
                       QFileDevice::OpenError);
    }

    QByteArray buffer(64 * 1024, Qt::Uninitialized);
    while (true) {
        const qint64 bytesRead = source.read(buffer.data(), buffer.size());
        if (bytesRead < 0) {
            return failure(source,
                           QStringLiteral("Failed to read completed document"),
                           sourcePath,
                           QFileDevice::ReadError);
        }
        if (bytesRead == 0) {
            break;
        }

        qint64 offset = 0;
        while (offset < bytesRead) {
            const qint64 bytesWritten = target.write(buffer.constData() + offset,
                                                     bytesRead - offset);
            if (bytesWritten <= 0) {
                return failure(target,
                               QStringLiteral("Failed to write selected document"),
                               targetPath,
                               QFileDevice::WriteError);
            }
            offset += bytesWritten;
        }
    }

    if (!target.flush()) {
        return failure(target,
                       QStringLiteral("Failed to flush selected document"),
                       targetPath,
                       QFileDevice::WriteError);
    }

    target.close();
    if (target.error() != QFileDevice::NoError) {
        return failure(target,
                       QStringLiteral("Failed to close selected document"),
                       targetPath,
                       QFileDevice::WriteError);
    }

    return {true, QFileDevice::NoError, {}};
}

}
