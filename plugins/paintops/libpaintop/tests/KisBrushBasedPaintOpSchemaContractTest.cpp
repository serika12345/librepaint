/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLightnessStrengthOptionWidget.h"
#include "KisMaskingBrushOptionProperties.h"
#include "kis_brush_based_paintop.h"
#include "kis_brush_based_paintop_options_widget.h"
#include "kis_texture_option.h"

#include <QTest>

#include <type_traits>

namespace
{

class BrushPaintOpProbe final : public KisBrushBasedPaintOp
{
public:
    using KisBrushBasedPaintOp::KisBrushBasedPaintOp;

protected:
    KisSpacingInformation paintAt(const KisPaintInformation &) override;
    KisSpacingInformation updateSpacingImpl(const KisPaintInformation &) const override;
};

class BrushBasedPaintopOptionWidgetProbe final : public KisBrushBasedPaintopOptionWidget
{
public:
    using KisBrushBasedPaintopOptionWidget::KisBrushBasedPaintopOptionWidget;

    KisPropertiesConfigurationSP configuration() const override
    {
        return {};
    }
};

#define ASSERT_TEXT_BRUSH_INITIALIZATION_SIGNATURE(method, signature)                                                  \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&TextBrushInitializationWorkaround::method)), signature>)
#define ASSERT_BRUSH_PAINTOP_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBrushBasedPaintOp::method)), signature>)

} // namespace

class KisBrushBasedPaintOpSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void brushBasedPaintopOptionWidgetSchemaRemainStable();
    void lightnessStrengthOptionWidgetSchemaRemainStable();
    void textBrushInitializationSchemaRemainStable();
    void brushPaintOpTypeConstructionAndLifetimeSchemaRemainStable();
    void brushPaintOpPaintabilitySignaturesRemainStable();
    void brushPaintOpSpacingSignaturesRemainStable();
    void brushPaintOpPreparationSignaturesRemainStable();
    void maskingBrushDataTypeAndMemberSchemaRemainStable();
    void maskingBrushDataEqualitySignatureRemainStable();
    void maskingBrushDataPersistenceSignaturesRemainStable();
    void textureOptionTypeStateAndConstructionSchemaRemainStable();
    void textureOptionProcessingAndResourceSignaturesRemainStable();
};

void KisBrushBasedPaintOpSchemaContractTest::brushBasedPaintopOptionWidgetSchemaRemainStable()
{
    using Widget = KisBrushBasedPaintopOptionWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisPaintOpSettingsWidget, Widget>);
    static_assert(std::is_abstract_v<Widget>);
    static_assert(std::is_constructible_v<BrushBasedPaintopOptionWidgetProbe, KisBrushOptionWidgetFlags, QWidget *>);
    static_assert(std::is_destructible_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::brush), KisBrushSP (Widget::*)()>);
    static_assert(std::is_same_v<decltype(&Widget::effectiveBrushSize), lager::reader<qreal> (Widget::*)() const>);
}

void KisBrushBasedPaintOpSchemaContractTest::lightnessStrengthOptionWidgetSchemaRemainStable()
{
    using Widget = KisLightnessStrengthOptionWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisCurveOptionWidget, Widget>);
    static_assert(std::is_same_v<Widget::data_type, KisLightnessStrengthOptionData>);
    static_assert(std::is_constructible_v<Widget, lager::cursor<KisLightnessStrengthOptionData>, lager::reader<bool>>);
    static_assert(std::is_destructible_v<Widget>);
}

void KisBrushBasedPaintOpSchemaContractTest::textBrushInitializationSchemaRemainStable()
{
    using Workaround = TextBrushInitializationWorkaround;

    static_assert(std::is_class_v<Workaround>);
    static_assert(std::is_default_constructible_v<Workaround>);
    static_assert(std::is_destructible_v<Workaround>);
    static_assert(std::is_same_v<decltype(&Workaround::instance), Workaround *(*)()>);
    ASSERT_TEXT_BRUSH_INITIALIZATION_SIGNATURE(preinitialize, void (Workaround::*)(KisPaintOpSettingsSP));
    ASSERT_TEXT_BRUSH_INITIALIZATION_SIGNATURE(tryGetBrush, KisBrushSP (Workaround::*)(KisPropertiesConfigurationSP));
}

void KisBrushBasedPaintOpSchemaContractTest::brushPaintOpTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisBrushBasedPaintOp>);
    static_assert(std::is_same_v<KisBrushBasedPaintOp::needs_preinitialization, int>);
    static_assert(std::is_constructible_v<BrushPaintOpProbe, KisPaintOpSettingsSP, KisPainter *, KisBrushTextureFlags>);
    static_assert(std::is_destructible_v<KisBrushBasedPaintOp>);
    static_assert(std::has_virtual_destructor_v<KisBrushBasedPaintOp>);
}

void KisBrushBasedPaintOpSchemaContractTest::brushPaintOpPaintabilitySignaturesRemainStable()
{
    ASSERT_BRUSH_PAINTOP_SIGNATURE(canPaint, bool (KisBrushBasedPaintOp::*)() const);
    ASSERT_BRUSH_PAINTOP_SIGNATURE(checkSizeTooSmall, bool (KisBrushBasedPaintOp::*)(qreal));
}

void KisBrushBasedPaintOpSchemaContractTest::brushPaintOpSpacingSignaturesRemainStable()
{
    ASSERT_BRUSH_PAINTOP_SIGNATURE(effectiveSpacing, KisSpacingInformation (KisBrushBasedPaintOp::*)(qreal) const);
    ASSERT_BRUSH_PAINTOP_SIGNATURE(
        effectiveSpacing,
        KisSpacingInformation (KisBrushBasedPaintOp::*)(qreal, qreal, const KisPaintInformation &) const);
    ASSERT_BRUSH_PAINTOP_SIGNATURE(
        effectiveSpacing,
        KisSpacingInformation (
            KisBrushBasedPaintOp::*)(qreal, qreal, const KisSpacingOption &, const KisPaintInformation &) const);
    ASSERT_BRUSH_PAINTOP_SIGNATURE(effectiveSpacing,
                                   KisSpacingInformation (KisBrushBasedPaintOp::*)(qreal,
                                                                                   qreal,
                                                                                   const KisAirbrushOptionData *,
                                                                                   const KisSpacingOption *,
                                                                                   const KisPaintInformation &) const);
}

void KisBrushBasedPaintOpSchemaContractTest::brushPaintOpPreparationSignaturesRemainStable()
{
    using ResourcePreparation = QList<KoResourceLoadResult> (*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP);

    static_assert(
        std::is_same_v<decltype(&KisBrushBasedPaintOp::preinitializeOpStatically), void (*)(KisPaintOpSettingsSP)>);
    static_assert(std::is_same_v<decltype(&KisBrushBasedPaintOp::prepareLinkedResources), ResourcePreparation>);
    static_assert(std::is_same_v<decltype(&KisBrushBasedPaintOp::prepareEmbeddedResources), ResourcePreparation>);
}

void KisBrushBasedPaintOpSchemaContractTest::maskingBrushDataTypeAndMemberSchemaRemainStable()
{
    using Data = KisBrushModel::MaskingBrushData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_same_v<decltype(Data::isEnabled), bool>);
    static_assert(std::is_same_v<decltype(Data::brush), KisBrushModel::BrushData>);
    static_assert(std::is_same_v<decltype(Data::compositeOpId), QString>);
    static_assert(std::is_same_v<decltype(Data::useMasterSize), bool>);
    static_assert(std::is_same_v<decltype(Data::masterSizeCoeff), qreal>);

    QVERIFY(true);
}

void KisBrushBasedPaintOpSchemaContractTest::maskingBrushDataEqualitySignatureRemainStable()
{
    using Data = KisBrushModel::MaskingBrushData;
    using Equality = bool (*)(const Data &, const Data &);

    static_assert(std::is_same_v<decltype(static_cast<Equality>(&KisBrushModel::operator==)), Equality>);

    QVERIFY(true);
}

void KisBrushBasedPaintOpSchemaContractTest::maskingBrushDataPersistenceSignaturesRemainStable()
{
    using Data = KisBrushModel::MaskingBrushData;
    using Read = Data (*)(const KisPropertiesConfiguration *, qreal, KisResourcesInterfaceSP);

    static_assert(std::is_same_v<decltype(&Data::read), Read>);
    static_assert(std::is_same_v<decltype(&Data::write), void (Data::*)(KisPropertiesConfiguration *) const>);

    QVERIFY(true);
}

void KisBrushBasedPaintOpSchemaContractTest::textureOptionTypeStateAndConstructionSchemaRemainStable()
{
    using Option = KisTextureOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_same_v<decltype(Option::m_enabled), bool>);
    static_assert(std::is_constructible_v<Option,
                                          const KisPropertiesConfiguration *,
                                          KisResourcesInterfaceSP,
                                          KoCanvasResourcesInterfaceSP,
                                          int,
                                          KisBrushTextureFlags>);

    QVERIFY(true);
}

void KisBrushBasedPaintOpSchemaContractTest::textureOptionProcessingAndResourceSignaturesRemainStable()
{
    using Option = KisTextureOption;
    using GradientQuery = bool (Option::*)() const;
    using ConfigurationGradientQuery = bool (*)(const KisPropertiesConfiguration *);
    using ResourcePreparation = QList<KoResourceLoadResult> (*)(KisPropertiesConfigurationSP, KisResourcesInterfaceSP);

    static_assert(std::is_same_v<decltype(&Option::apply),
                                 void (Option::*)(KisFixedPaintDeviceSP, const QPoint &, const KisPaintInformation &)>);
    static_assert(std::is_same_v<decltype(static_cast<GradientQuery>(&Option::applyingGradient)), GradientQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ConfigurationGradientQuery>(&Option::applyingGradient)),
                                 ConfigurationGradientQuery>);
    static_assert(std::is_same_v<decltype(&Option::prepareEmbeddedResources), ResourcePreparation>);
    static_assert(std::is_same_v<decltype(&Option::prepareLinkedResources), ResourcePreparation>);
    static_assert(std::is_same_v<decltype(&Option::requiresEffectiveCompositeOp), ConfigurationGradientQuery>);

    QVERIFY(true);
}

#undef ASSERT_BRUSH_PAINTOP_SIGNATURE
#undef ASSERT_TEXT_BRUSH_INITIALIZATION_SIGNATURE

QTEST_APPLESS_MAIN(KisBrushBasedPaintOpSchemaContractTest)

#include "KisBrushBasedPaintOpSchemaContractTest.moc"
