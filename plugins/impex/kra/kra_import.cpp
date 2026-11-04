/*
 * SPDX-FileCopyrightText: 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kra_import.h"

#include <kpluginfactory.h>

#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <QBuffer>

#include "KisImportExportErrorCode.h"
#include "KisImportExportFilter.h"
#include "kis_types.h"
#include "kra_converter.h"
#include <kra/KisKraRasterCodec.h>

K_PLUGIN_CLASS_WITH_JSON(KraImport, "krita_kra_import.json")

KraImport::KraImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KraImport::~KraImport()
{
}

KisImportExportErrorCode KraImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP /*configuration*/)
{
    if (KisPart::instance()->gpuEngineEnabled() &&
        !LibrePaint::ImportExport::Kra::RasterCodec::isNonRgbaOrAnimated(*io)) {
        if (const auto execution = KisPart::instance()->gpuSessionExecution()) {
            if (!LibrePaint::ImportExport::Kra::RasterCodec::requiresLegacyConversion(*io))
                return document->importGpuKra(*io);
            QScopedPointer<KisDocument> legacy(KisPart::instance()->createDocument());
            legacy->setFileBatchMode(true);
            legacy->setLocalFilePath(document->localFilePath());
            QBuffer current;
            current.open(QIODevice::ReadWrite);
            {
                KraConverter converter(legacy.data());
                const auto result = converter.convertToCurrentFormat(io, &current);
                if (!result.isOk()) {
                    document->setErrorMessage(legacy->errorMessage());
                    return result;
                }
            }
            current.close();
            current.open(QIODevice::ReadOnly);
            const auto result = document->importGpuKra(current);
            if (result.isOk()) {
                QStringList warnings{legacy->warningMessage(), document->warningMessage()};
                warnings.removeAll(QString());
                document->setWarningMessage(warnings.join(QLatin1Char('\n')));
            }
            return result;
        }
    }
    KraConverter kraConverter(document);
    KisImportExportErrorCode result = kraConverter.buildImage(io);
    if (result.isOk()) {
        KisNodeSP preActivatedNode = !kraConverter.activeNodes().isEmpty() ? kraConverter.activeNodes().first() : nullptr;
        document->setCurrentImage(kraConverter.image(), true, preActivatedNode);

        if (kraConverter.assistants().size() > 0) {
            document->setAssistants(kraConverter.assistants());
        }
        document->setStoryboardItemList(kraConverter.storyboardItemList(), true);
        document->setStoryboardCommentList(kraConverter.storyboardCommentList(), true);
    }
    return result;
}

#include <kra_import.moc>
