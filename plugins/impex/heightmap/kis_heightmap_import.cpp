/*
 *  SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2017 Victor Wåhlström <victor.wahlstrom@initiali.se>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "kis_heightmap_import.h"
#include <klocalizedstring.h>


#include <QApplication>
#include <cmath>
#include <qcontainerfwd.h>
#include <qdialog.h>

#include <kpluginfactory.h>
#include <KoDialog.h>

#include <KisImportExportManager.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpace.h>

#include <document/KisDocument.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_iterator_ng.h>
#include <application/kis_config.h>
#include <kis_image_config.h>
#include <events/kis_cursor_override_hijacker.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <QtGlobal>
#include <qvariant.h>

#include "KisImportExportErrorCode.h"
#include "KoColorSpaceConstants.h"
#include "KoGrayColorSpaceTraits.h"
#include "KoID.h"
#include "kis_assert.h"
#include "kis_properties_configuration.h"
#include "kis_types.h"
#include "kis_wdg_options_heightmap.h"
#include "kis_heightmap_utils.h"
#include "ui_kis_wdg_options_heightmap.h"

K_PLUGIN_FACTORY_WITH_JSON(HeightMapImportFactory, "krita_heightmap_import.json", registerPlugin<KisHeightMapImport>();)

template<typename T>
void fillData(KisPaintDeviceSP pd, int w, int h, QDataStream &stream) {
    KIS_ASSERT_RECOVER_RETURN(pd);

    T pixel;

    for (int i = 0; i < h; ++i) {
        KisHLineIteratorSP it = pd->createHLineIteratorNG(0, i, w);
        do {
            stream >> pixel;
            KoGrayTraits<T>::setGray(it->rawData(), pixel);
            KoGrayTraits<T>::setOpacity(it->rawData(), OPACITY_OPAQUE_F, 1);
        } while(it->nextPixel());
    }
}

KisHeightMapImport::KisHeightMapImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisHeightMapImport::~KisHeightMapImport()
{
}

KisImportExportErrorCode KisHeightMapImport::convert(KisDocument *document, QIODevice *io, KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(configuration);
    KoID depthId = KisHeightmapUtils::mimeTypeToKoID(mimeType());
    if (depthId.id().isNull()) {
        document->setErrorMessage(i18n("Unknown file type"));
        return ImportExportCodes::FileFormatIncorrect;
    }

    int w = 0;
    int h = 0;

    KIS_ASSERT(io->isOpen());
    const quint64 size = io->size();
    if (size == 0) {
        return ImportExportCodes::FileFormatIncorrect;
    }

    QDataStream::ByteOrder bo = QDataStream::LittleEndian;

    if (!batchMode()) {
        KisCursorOverrideHijacker cursorHijacker;

        KoDialog* kdb = new KoDialog(qApp->activeWindow());
        kdb->setWindowTitle(i18n("Heightmap Import Options"));
        kdb->setButtons(KoDialog::Ok | KoDialog::Cancel);

        KisWdgOptionsHeightmap* wdg = new KisWdgOptionsHeightmap(kdb);

        kdb->setMainWidget(wdg);

        connect(wdg, SIGNAL(statusUpdated(bool)), kdb, SLOT(enableButtonOk(bool)));

        KisImageConfig config(false);

        QString filterConfig = config.importConfiguration(mimeType());
        KisPropertiesConfigurationSP cfg(new KisPropertiesConfiguration);
        cfg->fromXML(filterConfig);


        int endianness = cfg->getInt("endianness", 1);
        if (endianness == 0) {
            wdg->radioBig->setChecked(true);
        }
        else {
            wdg->radioLittle->setChecked(true);
        }

        wdg->fileSizeLabel->setText(QString::number(size));

        if(depthId == Integer8BitsColorDepthID) {
            wdg->bppLabel->setText(QString::number(8));
            wdg->typeLabel->setText("Integer");
        }
        else if(depthId == Integer16BitsColorDepthID) {
            wdg->bppLabel->setText(QString::number(16));
            wdg->typeLabel->setText("Integer");
        }
        else if(depthId == Float32BitsColorDepthID) {
            wdg->bppLabel->setText(QString::number(32));
            wdg->typeLabel->setText("Float");
        }
        else {
            KIS_ASSERT_RECOVER_RETURN_VALUE(true, ImportExportCodes::InternalError);
            return ImportExportCodes::InternalError;
        }

        if (kdb->exec() == QDialog::Rejected) {
            return ImportExportCodes::Cancelled;
        }

        cfg->setProperty("endianness", wdg->radioBig->isChecked() ? 0 : 1);

        config.setImportConfiguration(mimeType(), cfg);

        w = wdg->widthInput->value();
        h = wdg->heightInput->value();

        bo = QDataStream::LittleEndian;
        cfg->setProperty("endianness", 1);
        if (wdg->radioBig->isChecked()) {
            bo = QDataStream::BigEndian;
            cfg->setProperty("endianness", 0);
        }
        KisImageConfig(false).setExportConfiguration(mimeType(), cfg);

    } else {
        const int pixelSize =
            depthId == Float32BitsColorDepthID ? 4 :
            depthId == Integer16BitsColorDepthID ? 2 : 1;

        const int numPixels = size / pixelSize;

        w = std::sqrt(numPixels);
        h = numPixels / w;
        bo = QDataStream::LittleEndian;
    }


    QDataStream s(io);
    s.setByteOrder(bo);
    // needed for 32bit float data
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), depthId.id(), "Gray-D50-elle-V2-srgbtrc.icc");
    KisImageSP image = new KisImage(document->createUndoStore(), w, h, colorSpace, "imported heightmap");
    KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);

    if (depthId == Float32BitsColorDepthID) {
        fillData<float>(layer->paintDevice(), w, h, s);
    }
    else if (depthId == Integer16BitsColorDepthID) {
        fillData<quint16>(layer->paintDevice(), w, h, s);
    }
    else if (depthId == Integer8BitsColorDepthID) {
        fillData<quint8>(layer->paintDevice(), w, h, s);
    }
    else {
        KIS_ASSERT_RECOVER_RETURN_VALUE(true, ImportExportCodes::InternalError);
        return ImportExportCodes::InternalError;
    }

    image->addNode(layer.data(), image->rootLayer().data());
    document->setCurrentImage(image);
    return ImportExportCodes::OK;
}

#include "kis_heightmap_import.moc"
