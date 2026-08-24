/* This file is part of the Krita project
 *
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisDocument.h"

#include <KisTranslateLayerNamesVisitor.h>
#include <kis_group_layer.h>
#include <kis_image.h>

void KisDocument::translateTemplateRootLayerName(const QMap<QString, QString> &dictionary)
{
    KisImageSP documentImage = image();
    if (!documentImage) {
        return;
    }

    KisTranslateLayerNamesVisitor visitor(dictionary);
    documentImage->rootLayer()->accept(visitor);
}
