/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_brush_based_paintop.h"

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
    void textBrushInitializationSchemaRemainStable();
    void brushPaintOpTypeConstructionAndLifetimeSchemaRemainStable();
    void brushPaintOpPaintabilitySignaturesRemainStable();
    void brushPaintOpSpacingSignaturesRemainStable();
    void brushPaintOpPreparationSignaturesRemainStable();
};

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

#undef ASSERT_BRUSH_PAINTOP_SIGNATURE
#undef ASSERT_TEXT_BRUSH_INITIALIZATION_SIGNATURE

QTEST_APPLESS_MAIN(KisBrushBasedPaintOpSchemaContractTest)

#include "KisBrushBasedPaintOpSchemaContractTest.moc"
