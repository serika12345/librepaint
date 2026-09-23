/*
 * KDE. Krita Project.
 *
 * SPDX-FileCopyrightText: 2020 Deif Lou <ginoba@gmail.com>
 * SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <KoUpdater.h>
#include <kis_processing_information.h>
#include <filter/kis_filter_configuration.h>
#include <kis_gradient_painter.h>
#include <kis_paint_device.h>
#include "KisGradientGeneratorConfiguration.h"
#include "KoCompositeOpIds.h"
#include <QtGlobal>
#include <qcontainerfwd.h>
#include <qpoint.h>
#include <qsize.h>

#include "KisGradientGenerator.h"
#include "KisGradientGeneratorConfigWidget.h"
#include "KoID.h"
#include "kis_assert.h"
#include "kis_generator.h"
#include "kis_types.h"
#include "ui_KisGradientGeneratorConfigWidget.h"

KisGradientGenerator::KisGradientGenerator() : KisGenerator(id(), KoID("basic"), i18n("&Gradient..."))
{
    setSupportsPainting(true);
}

void KisGradientGenerator::generate(KisProcessingInformation dst,
                                    const QSize &size,
                                    const KisFilterConfigurationSP config,
                                    KoUpdater *progressUpdater) const
{
    KisPaintDeviceSP device = dst.paintDevice();
    Q_ASSERT(!device.isNull());

    KIS_SAFE_ASSERT_RECOVER_RETURN(config);
    const KisGradientGeneratorConfiguration *generatorConfiguration =
        dynamic_cast<const KisGradientGeneratorConfiguration*>(config.data());
    KIS_SAFE_ASSERT_RECOVER_RETURN(generatorConfiguration);


    QSize imageSize = device->defaultBounds()->imageBorderRect().size();
    QPair<QPointF, QPointF> positions =
        generatorConfiguration->absoluteCartesianPositionsInPixels(imageSize.width(), imageSize.height());

    KisGradientPainter painter(device);
    painter.setCompositeOpId(COMPOSITE_COPY);
    painter.setProgress(progressUpdater);
    painter.setGradientShape(generatorConfiguration->shape());
    painter.setGradient(generatorConfiguration->gradient());
    painter.paintGradient(
        positions.first,
        positions.second,
        generatorConfiguration->repeat(),
        generatorConfiguration->antiAliasThreshold(),
        generatorConfiguration->reverse(),
        QRect(dst.topLeft(), size),
        generatorConfiguration->dither()
    );
}

KisFilterConfigurationSP KisGradientGenerator::factoryConfiguration(KisResourcesInterfaceSP resourcesInterface) const
{
    return new KisGradientGeneratorConfiguration(resourcesInterface);
}

KisFilterConfigurationSP KisGradientGenerator::defaultConfiguration(KisResourcesInterfaceSP resourcesInterface) const
{
    KisGradientGeneratorConfiguration *config = new KisGradientGeneratorConfiguration(resourcesInterface);
    config->setDefaults();
    return config;
}

KisConfigWidget* KisGradientGenerator::createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev, bool) const
{
    Q_UNUSED(dev);
    return new KisGradientGeneratorConfigWidget(parent);
}
