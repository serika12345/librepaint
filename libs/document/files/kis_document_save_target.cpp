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
#else
    return {fileInfo.exists(), fileInfo.isWritable()};
#endif
}

}
