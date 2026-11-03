/*
 *  SPDX-FileCopyrightText: 2005 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_jpeg_import.h"


#include <kpluginfactory.h>

#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <QPointer>
#include <application/ui/workspace/KisViewManager.h>
#include <KisImportExportManager.h>
#include <qcontainerfwd.h>
#include <qobject.h>

#include "KisImportExportErrorCode.h"
#include "KisImportExportFilter.h"
#include "kis_jpeg_converter.h"
#include "kis_types.h"

K_PLUGIN_FACTORY_WITH_JSON(JPEGImportFactory, "krita_jpeg_import.json", registerPlugin<KisJPEGImport>();)

KisJPEGImport::KisJPEGImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisJPEGImport::~KisJPEGImport()
{
}

KisImportExportErrorCode KisJPEGImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP /*configuration*/)
{
    KisJPEGConverter ib(document, batchMode());
    QPointer<KisDocument> guard(document);
    KisImportExportErrorCode result;
    if (KisPart::instance()->gpuEngineEnabled()) {
        result = ib.prepareGpuImage(io);
        if (!result.isOk() || !guard) return result;
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        if (!execution) return KisImportExportErrorCode(diagnostic);
        result = ib.buildGpuImage(execution);
    } else result = ib.buildImage(io);
    if (result.isOk() && guard) {
        document->setCurrentImage(ib.image());
    }
    return result;
}

#include <kis_jpeg_import.moc>

