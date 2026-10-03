/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_qimageio_import.h"
#include "KisImportExportErrorCode.h"
#include "KisImportExportFilter.h"
#include "kis_types.h"

#include <QCheckBox>
#include <QSlider>
#include <QApplication>
#include <QImageReader>

#include <kpluginfactory.h>


#include <kis_paint_device.h>
#include <document/KisDocument.h>
#include <kis_image.h>
#include <qcontainerfwd.h>
#include <qobject.h>


K_PLUGIN_FACTORY_WITH_JSON(KisQImageIOImportFactory, "krita_qimageio_import.json", registerPlugin<KisQImageIOImport>();)

KisQImageIOImport::KisQImageIOImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisQImageIOImport::~KisQImageIOImport()
{
}

KisImportExportErrorCode KisQImageIOImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP /*configuration*/)
{

    QImage img;
    if (!img.loadFromData(io->readAll()/*, fi.suffix().toLower().toLatin1()*/)) {
        return ImportExportCodes::FileFormatIncorrect;
    }

    KisImageSP image = KisImage::fromQImage(img, document->createUndoStore());
    document->setCurrentImage(image);
    return ImportExportCodes::OK;

}

#include "kis_qimageio_import.moc"

