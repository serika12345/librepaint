/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_standard_uniform_properties_factory.h"

#include "KisPaintOpPresetUpdateProxy.h"
#include "KisStandardUniformPropertyDefinition.h"
#include "kis_image_config.h"
#include "kis_paintop_settings.h"
#include "kis_slider_based_paintop_property.h"

namespace KisStandardUniformPropertiesFactory
{

KisUniformPaintOpPropertySP
createProperty(const KoID &id, KisPaintOpSettingsRestrictedSP settings, KisPaintOpPresetUpdateProxy *updateProxy)
{
    return createProperty(id.id(), settings, updateProxy);
}

KisUniformPaintOpPropertySP
createProperty(const QString &id, KisPaintOpSettingsRestrictedSP settings, KisPaintOpPresetUpdateProxy *updateProxy)
{
    const auto definition = standardUniformPropertyDefinition(id);
    if (!definition) {
        if (id == angle.id()) {
            qFatal("Not implemented");
        } else if (id == spacing.id()) {
            qFatal("Not implemented");
        } else {
            KIS_SAFE_ASSERT_RECOVER_NOOP(0 && "Unknown Uniform property id!");
        }
        return KisUniformPaintOpPropertySP();
    }

    const auto createDoubleSliderProperty = [&](const KoID &propertyId) {
        return new KisDoubleSliderBasedPaintOpPropertyCallback(KisDoubleSliderBasedPaintOpPropertyCallback::Double,
                                                               propertyId,
                                                               settings,
                                                               0);
    };

    KisDoubleSliderBasedPaintOpPropertyCallback *prop = nullptr;
    switch (definition->kind) {
    case KisStandardUniformPropertyDefinition::Kind::Size:
        prop = createDoubleSliderProperty(KoID("size", i18n("Size")));
        break;
    case KisStandardUniformPropertyDefinition::Kind::Opacity:
        prop = createDoubleSliderProperty(opacity);
        break;
    case KisStandardUniformPropertyDefinition::Kind::Flow:
        prop = createDoubleSliderProperty(flow);
        break;
    }

    const qreal maximum = definition->usesBrushSizeMaximum ? KisImageConfig(true).maxBrushSize() : definition->maximum;
    prop->setRange(definition->minimum, maximum);
    prop->setDecimals(definition->decimals);
    prop->setSingleStep(definition->singleStep);
    prop->setExponentRatio(definition->exponentRatio);
    if (definition->usesPixelSuffix) {
        prop->setSuffix(i18n(" px"));
    }

    switch (definition->kind) {
    case KisStandardUniformPropertyDefinition::Kind::Size:
        prop->setReadCallback([](KisUniformPaintOpProperty *property) {
            property->setValue(property->settings()->paintOpSize());
        });
        prop->setWriteCallback([](KisUniformPaintOpProperty *property) {
            property->settings()->setPaintOpSize(property->value().toReal());
        });
        break;
    case KisStandardUniformPropertyDefinition::Kind::Opacity:
        prop->setReadCallback([](KisUniformPaintOpProperty *property) {
            property->setValue(property->settings()->paintOpOpacity());
        });
        prop->setWriteCallback([](KisUniformPaintOpProperty *property) {
            property->settings()->setPaintOpOpacity(property->value().toReal());
        });
        break;
    case KisStandardUniformPropertyDefinition::Kind::Flow:
        prop->setReadCallback([](KisUniformPaintOpProperty *property) {
            property->setValue(property->settings()->paintOpFlow());
        });
        prop->setWriteCallback([](KisUniformPaintOpProperty *property) {
            property->settings()->setPaintOpFlow(property->value().toReal());
        });
        break;
    }

    QObject::connect(updateProxy, SIGNAL(sigSettingsChanged()), prop, SLOT(requestReadValue()));
    prop->requestReadValue();
    return toQShared(prop);
}
} // namespace KisStandardUniformPropertiesFactory
