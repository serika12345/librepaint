/*
 * SPDX-FileCopyrightText: 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kra_import.h"

#include <kpluginfactory.h>

#include <document/KisDocument.h>
#include <kis_image.h>

#include "kra_converter.h"

K_PLUGIN_CLASS_WITH_JSON(KraImport, "krita_kra_import.json")

KraImport::KraImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KraImport::~KraImport()
{
}

KisImportExportErrorCode KraImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP /*configuration*/)
{
    KraConverter kraConverter(document);
    KisImportExportErrorCode result = kraConverter.buildImage(io);
    if (result.isOk()) {
        KisNodeSP preActivatedNode = !kraConverter.activeNodes().isEmpty() ? kraConverter.activeNodes().first() : nullptr;
        document->setCurrentImage(kraConverter.image(), true, preActivatedNode);

        if (kraConverter.assistants().size() > 0) {
            document->setAssistants(kraConverter.assistants());
        }
        if (kraConverter.storyboardItemList().size() > 0) {
            document->setStoryboardItemList(kraConverter.storyboardItemList(), true);
        }
        if (kraConverter.storyboardCommentList().size() > 0) {
            document->setStoryboardCommentList(kraConverter.storyboardCommentList(), true);
        }
    }
    return result;
}

#include <kra_import.moc>
