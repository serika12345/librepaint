/*
 *  SPDX-FileCopyrightText: 2005 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_png_import.h"

#include <kpluginfactory.h>

#include <KisImportExportManager.h>

#include <document/KisDocument.h>

#include <application/ui/workspace/KisViewManager.h>
#include <qcontainerfwd.h>
#include <qobject.h>

#include "KisImportExportErrorCode.h"
#include "KisImportExportFilter.h"
#include "kis_png_converter.h"
#include "kis_types.h"

K_PLUGIN_CLASS_WITH_JSON(KisPNGImport, "krita_png_import.json")

KisPNGImport::KisPNGImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisPNGImport::~KisPNGImport()
{
}

KisImportExportErrorCode KisPNGImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP /*configuration*/)
{
    KisPNGConverter ib(document, batchMode());
    KisImportExportErrorCode res = ib.buildImage(io);
    if (res.isOk()){
        document->setCurrentImage(ib.image());
    }
    return res;

}

#include <kis_png_import.moc>
