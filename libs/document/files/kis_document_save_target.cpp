/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_save_target.h"

#include <QFileInfo>

namespace Krita::Document
{

KisDocumentSaveTarget KisDocumentSaveTarget::inspect(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);

#ifdef Q_OS_ANDROID
    return {fileInfo.exists() && fileInfo.size() > 0, true};
#elif defined(Q_OS_IOS)
    // UIDocumentPicker creates the selected export file before returning it.
    // An empty placeholder has no previous document contents to preserve.
    return {fileInfo.exists() && fileInfo.size() > 0, fileInfo.isWritable()};
#else
    return {fileInfo.exists(), fileInfo.isWritable()};
#endif
}

}
