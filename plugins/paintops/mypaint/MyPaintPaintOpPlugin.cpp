/*
 * SPDX-FileCopyrightText: 2020 Ashwin Dhakaita <ashwingpdhakaita@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MyPaintPaintOpPlugin.h"

#include <KisResourceLoader.h>
#include <KisResourceLoaderRegistry.h>
#include <KisDynamicSensorFactoryRegistry.h>
#include <KisSimpleDynamicSensorFactory.h>
#include <brushengine/kis_paintop_registry.h>
#include <kis_debug.h>
#include <kis_fixed_paint_device.h>
#include <kis_global.h>
#include <kis_simple_paintop_factory.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "MyPaintPaintOpFactory.h"
#include "MyPaintPaintOpPreset.h"
#include "MyPaintSensorPack.h"

namespace {

void registerMyPaintSensorFactories()
{
    static const bool registered = [] {
        auto addFactory = [](const KoID &id,
                             int minimumValue,
                             int maximumValue,
                             const QString &minimumLabel,
                             const QString &maximumLabel,
                             const QString &valueSuffix) {
            KisDynamicSensorFactoryRegistry::instance()->add(new KisSimpleDynamicSensorFactory(id.id(),
                                                                                               minimumValue,
                                                                                               maximumValue,
                                                                                               minimumLabel,
                                                                                               maximumLabel,
                                                                                               valueSuffix));
        };

        addFactory(MyPaintPressureId, 0, 20, "", "", "");
        addFactory(MyPaintFineSpeedId, -20, 20, "", "", "");
        addFactory(MyPaintGrossSpeedId, -20, 20, "", "", "");
        addFactory(MyPaintRandomId, 0, 1, "", "", "");
        addFactory(MyPaintStrokeId, 0, 1, "", "", "");
        addFactory(MyPaintDirectionId, 0, 180, "", "", "");
        addFactory(MyPaintDeclinationId, 0, 90, "", "", i18n("%"));
        addFactory(MyPaintAscensionId, -180, 180, "", "", i18n("%"));
        addFactory(MyPaintCustomId, -20, 20, "", "", i18n("%"));

        return true;
    }();

    Q_UNUSED(registered);
}

} // namespace

K_PLUGIN_FACTORY_WITH_JSON(MyPaintOpPluginFactory, "kritamypaintop.json", registerPlugin<MyPaintOpPlugin>();)


MyPaintOpPlugin::MyPaintOpPlugin(QObject *parent, const QVariantList &)
    : QObject(parent)
{
    registerMyPaintSensorFactories();
    KisResourceLoaderRegistry::instance()->registerLoader(new KisResourceLoader<KisMyPaintPaintOpPreset>(ResourceSubType::MyPaintPaintOpPresets, ResourceType::PaintOpPresets
                                                                                                         , i18n("MyPaint Brush Presets")
                                                                                                         , QStringList() << "application/x-mypaint-brush"));
    KisPaintOpRegistry::instance()->add(new KisMyPaintOpFactory());
}

MyPaintOpPlugin::~MyPaintOpPlugin()
{
}

#include "MyPaintPaintOpPlugin.moc"
