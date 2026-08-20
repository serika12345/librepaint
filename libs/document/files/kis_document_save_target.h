/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_SAVE_TARGET_H
#define KIS_DOCUMENT_SAVE_TARGET_H

#include <QString>

#include <kritadocumentfiles_export.h>

namespace Krita::Document
{

struct KRITADOCUMENTFILES_EXPORT KisDocumentSaveTarget
{
    bool exists {false};
    bool writable {false};

    static KisDocumentSaveTarget inspect(const QString &filePath);
};

}

#endif
