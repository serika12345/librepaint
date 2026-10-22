/*
 * SPDX-FileCopyrightText: 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kra_export.h"

#include <QCheckBox>
#include <QSlider>

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <QApplication>

#include <KisImportExportManager.h>

#include <KisExportCheckRegistry.h>
#include <document/KisDocument.h>
#include <application/kis_config.h>
#include <kis_shape_layer.h>
#include <kis_properties_configuration.h>
#include <qcontainerfwd.h>
#include <QtGlobal>
#include <qobject.h>

#include "KisExportCheckBase.h"
#include "KisImportExportErrorCode.h"
#include "kis_assert.h"
#include "kis_debug.h"
#include "kis_types.h"
#include "kra_converter.h"

class KisExternalLayer;

K_PLUGIN_CLASS_WITH_JSON(KraExport, "krita_kra_export.json")

KraExport::KraExport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KraExport::~KraExport()
{
}

KisImportExportErrorCode KraExport::convert(KisDocument *document, QIODevice *io, KisPropertiesConfigurationSP configuration)
{
    KisImageSP image = document->savingImage();
    KIS_ASSERT_RECOVER_RETURN_VALUE(image, ImportExportCodes::InternalError);

    KraConverter kraConverter(document, updater());
    const auto saveTime = configuration && configuration->hasProperty("documentSaveTime")
        ? configuration->getProperty("documentSaveTime").toDateTime() : QDateTime::currentDateTime();
    if (!saveTime.isValid()) {
        document->setErrorMessage(i18n("The document save time is invalid."));
        return ImportExportCodes::FormatFeaturesUnsupported;
    }
    KisImportExportErrorCode res = kraConverter.buildFile(io, filename(), !document->isAutosaving(), saveTime,
        configuration ? configuration->getBool("compressRasterData", KisConfig(true).compressKra()) : KisConfig(true).compressKra());
    dbgFile << "KraExport::convert result =" << res;
    return res;
}

void KraExport::initializeCapabilities()
{
    // Kra supports everything, by definition
    KisExportCheckFactory *factory = 0;
    Q_FOREACH(const QString &id, KisExportCheckRegistry::instance()->keys()) {
        factory = KisExportCheckRegistry::instance()->get(id);
        addCapability(factory->create(KisExportCheckBase::SUPPORTED));
    }
}

QString KraExport::verify(const QString &fileName) const
{
    QString error = KisImportExportFilter::verify(fileName);
    if (error.isEmpty()) {
        return KisImportExportFilter::verifyZiPBasedFiles(fileName,
                                                          QStringList()
                                                          << "mimetype"
                                                          << "documentinfo.xml"
                                                          << "maindoc.xml"
                                                          << "preview.png");
    }
    return error;
}

bool KraExport::exportSupportsGuides() const {
    return true;
}


#include <kra_export.moc>
