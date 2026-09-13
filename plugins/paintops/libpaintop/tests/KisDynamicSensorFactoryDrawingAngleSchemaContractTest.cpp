/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDarkenOption.h>
#include <KisDynamicSensorFactoryDistance.h>
#include <KisDynamicSensorFactoryDrawingAngle.h>
#include <KisDynamicSensorFactoryFade.h>
#include <KisDynamicSensorFactoryRegistry.h>
#include <KisDynamicSensorFactoryTime.h>
#include <KisEmbeddedTextureData.h>
#include <KisFlowOpacityOption.h>
#include <KisHSVOption.h>
#include <KisMaskingBrushOption.h>
#include <KisMirrorOption.h>
#include <KisOpacityOption.h>
#include <KisRotationOption.h>
#include <KisScatterOption.h>
#include <KisSharpnessOption.h>
#include <KisSpacingOption.h>

#include <kis_color_source_option.h>

#include <QTest>

#include <type_traits>

namespace
{

template<typename Factory>
void verifyLengthAwareFactorySchema()
{
    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KisSimpleDynamicSensorFactory, Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::createConfigWidget),
                                 QWidget *(Factory::*)(lager::cursor<KisCurveOptionDataCommon>, QWidget *)>);
    static_assert(std::is_same_v<decltype(&Factory::maximumLabel), QString (Factory::*)(int)>);
    static_assert(std::is_same_v<decltype(&Factory::maximumValue), int (Factory::*)(int)>);
}

} // namespace

class KisDynamicSensorFactoryDrawingAngleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSourceOptionSchemaRemainStable();
    void drawingAngleFactorySchemaRemainStable();
    void embeddedTextureDataSchemaRemainStable();
    void distanceFactorySchemaRemainStable();
    void darkenOptionSchemaRemainStable();
    void fadeFactorySchemaRemainStable();
    void factoryRegistrySchemaRemainStable();
    void flowOpacityOptionSchemaRemainStable();
    void hsvOptionSchemaRemainStable();
    void maskingBrushOptionSchemaRemainStable();
    void mirrorOptionSchemaRemainStable();
    void opacityOptionSchemaRemainStable();
    void rotationOptionSchemaRemainStable();
    void scatterOptionSchemaRemainStable();
    void sharpnessOptionSchemaRemainStable();
    void spacingOptionSchemaRemainStable();
    void timeFactorySchemaRemainStable();
};

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::colorSourceOptionSchemaRemainStable()
{
    using Option = KisColorSourceOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(std::is_destructible_v<Option>);
    static_assert(
        std::is_same_v<decltype(&Option::createColorSource), KisColorSource *(Option::*)(const KisPainter *) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::drawingAngleFactorySchemaRemainStable()
{
    using Factory = KisDynamicSensorFactoryDrawingAngle;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KisSimpleDynamicSensorFactory, Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::createConfigWidget),
                                 QWidget *(Factory::*)(lager::cursor<KisCurveOptionDataCommon>, QWidget *)>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::embeddedTextureDataSchemaRemainStable()
{
    using Data = KisEmbeddedTextureData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_same_v<decltype(&Data::fromPattern), Data (*)(KoPatternSP)>);
    static_assert(std::is_same_v<decltype(&Data::isNull), bool (Data::*)() const>);
    static_assert(std::is_same_v<decltype(&Data::loadLinkedPattern),
                                 KoResourceLoadResult (Data::*)(KisResourcesInterfaceSP) const>);
    static_assert(std::is_same_v<decltype(&Data::read), bool (Data::*)(const KisPropertiesConfiguration *)>);
    static_assert(std::is_same_v<decltype(&Data::write), void (Data::*)(KisPropertiesConfiguration *) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::distanceFactorySchemaRemainStable()
{
    verifyLengthAwareFactorySchema<KisDynamicSensorFactoryDistance>();
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::darkenOptionSchemaRemainStable()
{
    using Option = KisDarkenOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(
        std::is_same_v<decltype(static_cast<KoColor (Option::*)(KisPainter *, const KisPaintInformation &) const>(
                           &Option::apply)),
                       KoColor (Option::*)(KisPainter *, const KisPaintInformation &) const>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Option::*)(KisColorSource *, const KisPaintInformation &) const>(
                           &Option::apply)),
                       void (Option::*)(KisColorSource *, const KisPaintInformation &) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::fadeFactorySchemaRemainStable()
{
    verifyLengthAwareFactorySchema<KisDynamicSensorFactoryFade>();
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::factoryRegistrySchemaRemainStable()
{
    using Registry = KisDynamicSensorFactoryRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<KoGenericRegistry<KisDynamicSensorFactory *>, Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::flowOpacityOptionSchemaRemainStable()
{
    using Option = KisFlowOpacityOption2;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *, KisNodeSP>);
    static_assert(std::is_same_v<decltype(static_cast<void (Option::*)(KisPainter *, const KisPaintInformation &)>(
                                     &Option::apply)),
                                 void (Option::*)(KisPainter *, const KisPaintInformation &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Option::*)(const KisPaintInformation &, qreal *, qreal *)>(
                                     &Option::apply)),
                                 void (Option::*)(const KisPaintInformation &, qreal *, qreal *)>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::hsvOptionSchemaRemainStable()
{
    using Option = KisHSVOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_same_v<decltype(&Option::createHueOption), Option *(*)(const KisPropertiesConfiguration *)>);
    static_assert(
        std::is_same_v<decltype(&Option::createSaturationOption), Option *(*)(const KisPropertiesConfiguration *)>);
    static_assert(
        std::is_same_v<decltype(&Option::createValueOption), Option *(*)(const KisPropertiesConfiguration *)>);
    static_assert(std::is_same_v<decltype(&Option::apply),
                                 void (Option::*)(KoColorTransformation *, const KisPaintInformation &) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::maskingBrushOptionSchemaRemainStable()
{
    using Option = KisMaskingBrushOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisPaintOpOption, Option>);
    static_assert(std::is_constructible_v<Option, lager::reader<qreal>>);
    static_assert(std::is_destructible_v<Option>);
    static_assert(
        std::is_same_v<decltype(&Option::writeOptionSetting), void (Option::*)(KisPropertiesConfigurationSP) const>);
    static_assert(
        std::is_same_v<decltype(&Option::readOptionSetting), void (Option::*)(const KisPropertiesConfigurationSP)>);
    static_assert(std::is_same_v<decltype(&Option::setImage), void (Option::*)(KisImageWSP)>);
    static_assert(
        std::is_same_v<decltype(&Option::lodLimitations), void (Option::*)(KisPaintopLodLimitations *) const>);
    static_assert(
        std::is_same_v<decltype(&Option::maskingBrushEnabledReader), lager::reader<bool> (Option::*)() const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::mirrorOptionSchemaRemainStable()
{
    using Option = KisMirrorOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(
        std::is_same_v<decltype(&Option::apply), MirrorProperties (Option::*)(const KisPaintInformation &) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::opacityOptionSchemaRemainStable()
{
    using Option = KisOpacityOption;
    using Base = KisStandardOption<KisOpacityOptionData>;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<Base, Option>);
    static_assert(std::is_same_v<Option::BaseClass, Base>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *, KisNodeSP>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Option::*)(KisPainter *, const KisPaintInformation &) const>(
                           &Option::apply)),
                       void (Option::*)(KisPainter *, const KisPaintInformation &) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::rotationOptionSchemaRemainStable()
{
    using Option = KisRotationOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(std::is_same_v<decltype(&Option::apply), qreal (Option::*)(const KisPaintInformation &) const>);
    static_assert(std::is_same_v<decltype(&Option::applyFanCornersInfo), void (Option::*)(KisPaintOp *)>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::scatterOptionSchemaRemainStable()
{
    using Option = KisScatterOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(
        std::is_same_v<decltype(&Option::apply), QPointF (Option::*)(const KisPaintInformation &, qreal, qreal) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::sharpnessOptionSchemaRemainStable()
{
    using Option = KisSharpnessOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(std::is_same_v<
                  decltype(&Option::apply),
                  void (Option::*)(const KisPaintInformation &, const QPointF &, qint32 &, qint32 &, qreal &, qreal &)
                      const>);
    static_assert(std::is_same_v<decltype(&Option::applyThreshold),
                                 void (Option::*)(KisFixedPaintDeviceSP, const KisPaintInformation &)>);
    static_assert(std::is_same_v<decltype(&Option::alignOutlineToPixels), bool (Option::*)() const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::spacingOptionSchemaRemainStable()
{
    using Option = KisSpacingOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(std::is_same_v<decltype(&Option::apply), qreal (Option::*)(const KisPaintInformation &) const>);
    static_assert(std::is_same_v<decltype(&Option::isotropicSpacing), bool (Option::*)() const>);
    static_assert(std::is_same_v<decltype(&Option::usingSpacingUpdates), bool (Option::*)() const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::timeFactorySchemaRemainStable()
{
    verifyLengthAwareFactorySchema<KisDynamicSensorFactoryTime>();
}

QTEST_GUILESS_MAIN(KisDynamicSensorFactoryDrawingAngleSchemaContractTest)

#include "KisDynamicSensorFactoryDrawingAngleSchemaContractTest.moc"
