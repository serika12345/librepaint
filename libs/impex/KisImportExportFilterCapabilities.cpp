/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportFilter_p.h"

#include <KisExportCheckRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpaceRegistry.h>
#include <klocalizedstring.h>

QMap<QString, KisExportCheckBase *> KisImportExportFilter::exportChecks()
{
    qDeleteAll(d->capabilities);
    initializeCapabilities();
    return d->capabilities;
}

void KisImportExportFilter::initializeCapabilities()
{
}

void KisImportExportFilter::addCapability(KisExportCheckBase *capability)
{
    d->capabilities[capability->id()] = capability;
}

void KisImportExportFilter::addSupportedColorModels(QList<QPair<KoID, KoID>> supportedColorModels,
                                                    const QString &name,
                                                    KisExportCheckBase::Level level)
{
    Q_ASSERT(level != KisExportCheckBase::SUPPORTED);
    QString layerMessage;
    QString imageMessage;
    const QList<KoID> allColorModels =
        KoColorSpaceRegistry::instance()->colorModelsList(KoColorSpaceRegistry::AllColorSpaces);
    for (const KoID &colorModelID : allColorModels) {
        const QList<KoID> allColorDepths =
            KoColorSpaceRegistry::instance()->colorDepthList(colorModelID.id(), KoColorSpaceRegistry::AllColorSpaces);
        for (const KoID &colorDepthID : allColorDepths) {
            KisExportCheckFactory *colorModelCheckFactory = KisExportCheckRegistry::instance()->get(
                "ColorModelCheck/" + colorModelID.id() + "/" + colorDepthID.id());
            KisExportCheckFactory *colorModelPerLayerCheckFactory = KisExportCheckRegistry::instance()->get(
                "ColorModelPerLayerCheck/" + colorModelID.id() + "/" + colorDepthID.id());

            if (!colorModelCheckFactory || !colorModelPerLayerCheckFactory) {
                qWarning() << "No factory for" << colorModelID << colorDepthID;
                continue;
            }

            if (supportedColorModels.contains(QPair<KoID, KoID>(colorModelID, colorDepthID))) {
                addCapability(colorModelCheckFactory->create(KisExportCheckBase::SUPPORTED));
                addCapability(colorModelPerLayerCheckFactory->create(KisExportCheckBase::SUPPORTED));
            } else {
                if (level == KisExportCheckBase::PARTIALLY) {
                    imageMessage = i18nc("image conversion warning",
                                         "%1 cannot save images with color model <b>%2</b> and depth <b>%3</b>. "
                                         "The image will be converted.",
                                         name,
                                         colorModelID.name(),
                                         colorDepthID.name());

                    layerMessage = i18nc("image conversion warning",
                                         "%1 cannot save layers with color model <b>%2</b> and depth <b>%3</b>. "
                                         "The layers will be converted or skipped.",
                                         name,
                                         colorModelID.name(),
                                         colorDepthID.name());
                } else {
                    imageMessage = i18nc("image conversion warning",
                                         "%1 cannot save images with color model <b>%2</b> and depth <b>%3</b>. "
                                         "The image will not be saved.",
                                         name,
                                         colorModelID.name(),
                                         colorDepthID.name());

                    layerMessage = i18nc("image conversion warning",
                                         "%1 cannot save layers with color model <b>%2</b> and depth <b>%3</b>. "
                                         "The layers will be skipped.",
                                         name,
                                         colorModelID.name(),
                                         colorDepthID.name());
                }

                addCapability(colorModelCheckFactory->create(level, imageMessage));
                addCapability(colorModelPerLayerCheckFactory->create(level, layerMessage));
            }
        }
    }
}
