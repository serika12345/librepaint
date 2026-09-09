/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "layerstyles/kis_layer_style_filter_environment.h"
#include "layerstyles/kis_ls_drop_shadow_filter.h"
#include "layerstyles/kis_ls_overlay_filter.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_FILTER_SIGNATURE(type, method, ...)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)
#define ASSERT_ENVIRONMENT_SIGNATURE(method, ...)                                                                      \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisLayerStyleFilterEnvironment::method)), __VA_ARGS__>)

} // namespace

class KisLayerStyleConcreteFiltersSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dropShadowTypeAndModesRemainStable();
    void dropShadowConstructionAndCloneSignaturesRemainStable();
    void dropShadowProcessingSignaturesRemainStable();
    void overlayTypeAndModesRemainStable();
    void overlayConstructionAndCloneSignaturesRemainStable();
    void overlayProcessingSignaturesRemainStable();
    void environmentTypeLifetimeAndBoundsSignaturesRemainStable();
    void environmentResourceAndPainterSignaturesRemainStable();
};

void KisLayerStyleConcreteFiltersSchemaContractTest::dropShadowTypeAndModesRemainStable()
{
    static_assert(std::is_class_v<KisLsDropShadowFilter>);
    static_assert(std::is_base_of_v<KisLayerStyleFilter, KisLsDropShadowFilter>);
    static_assert(KisLsDropShadowFilter::DropShadow == 0);
    static_assert(KisLsDropShadowFilter::InnerShadow == 1);
    static_assert(KisLsDropShadowFilter::OuterGlow == 2);
    static_assert(KisLsDropShadowFilter::InnerGlow == 3);

    QVERIFY(true);
}

// clang-format off
void KisLayerStyleConcreteFiltersSchemaContractTest::dropShadowConstructionAndCloneSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_default_constructible_v<KisLsDropShadowFilter>);
    static_assert(std::is_constructible_v<KisLsDropShadowFilter, KisLsDropShadowFilter::Mode>);
    ASSERT_FILTER_SIGNATURE(KisLsDropShadowFilter, clone, KisLayerStyleFilter * (KisLsDropShadowFilter::*)() const);
}

void KisLayerStyleConcreteFiltersSchemaContractTest::dropShadowProcessingSignaturesRemainStable()
{
    ASSERT_FILTER_SIGNATURE(KisLsDropShadowFilter,
                            processDirectly,
                            void (KisLsDropShadowFilter::*)(KisPaintDeviceSP,
                                                            KisMultipleProjection *,
                                                            KisLayerStyleKnockoutBlower *,
                                                            const QRect &,
                                                            KisPSDLayerStyleSP,
                                                            KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsDropShadowFilter,
        neededRect,
        QRect (KisLsDropShadowFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsDropShadowFilter,
        changedRect,
        QRect (KisLsDropShadowFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
}

void KisLayerStyleConcreteFiltersSchemaContractTest::overlayTypeAndModesRemainStable()
{
    static_assert(std::is_class_v<KisLsOverlayFilter>);
    static_assert(std::is_base_of_v<KisLayerStyleFilter, KisLsOverlayFilter>);
    static_assert(KisLsOverlayFilter::Color == 0);
    static_assert(KisLsOverlayFilter::Gradient == 1);
    static_assert(KisLsOverlayFilter::Pattern == 2);

    QVERIFY(true);
}

// clang-format off
void KisLayerStyleConcreteFiltersSchemaContractTest::overlayConstructionAndCloneSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_constructible_v<KisLsOverlayFilter, KisLsOverlayFilter::Mode>);
    ASSERT_FILTER_SIGNATURE(KisLsOverlayFilter, clone, KisLayerStyleFilter * (KisLsOverlayFilter::*)() const);
}

void KisLayerStyleConcreteFiltersSchemaContractTest::overlayProcessingSignaturesRemainStable()
{
    ASSERT_FILTER_SIGNATURE(KisLsOverlayFilter,
                            processDirectly,
                            void (KisLsOverlayFilter::*)(KisPaintDeviceSP,
                                                         KisMultipleProjection *,
                                                         KisLayerStyleKnockoutBlower *,
                                                         const QRect &,
                                                         KisPSDLayerStyleSP,
                                                         KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsOverlayFilter,
        neededRect,
        QRect (KisLsOverlayFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsOverlayFilter,
        changedRect,
        QRect (KisLsOverlayFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
}

// clang-format off
void KisLayerStyleConcreteFiltersSchemaContractTest::environmentTypeLifetimeAndBoundsSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_class_v<KisLayerStyleFilterEnvironment>);
    static_assert(std::is_constructible_v<KisLayerStyleFilterEnvironment, KisLayer *>);
    static_assert(std::is_destructible_v<KisLayerStyleFilterEnvironment>);
    ASSERT_ENVIRONMENT_SIGNATURE(layerBounds, QRect (KisLayerStyleFilterEnvironment::*)() const);
    ASSERT_ENVIRONMENT_SIGNATURE(defaultBounds, QRect (KisLayerStyleFilterEnvironment::*)() const);
    ASSERT_ENVIRONMENT_SIGNATURE(currentLevelOfDetail, int (KisLayerStyleFilterEnvironment::*)() const);
}

// clang-format off
void KisLayerStyleConcreteFiltersSchemaContractTest::environmentResourceAndPainterSignaturesRemainStable()
// clang-format on
{
    ASSERT_ENVIRONMENT_SIGNATURE(setupFinalPainter,
                                 void (KisLayerStyleFilterEnvironment::*)(KisPainter *, quint8, const QBitArray &)
                                     const);
    ASSERT_ENVIRONMENT_SIGNATURE(cachedRandomSelection,
                                 KisPixelSelectionSP (KisLayerStyleFilterEnvironment::*)(const QRect &) const);
    ASSERT_ENVIRONMENT_SIGNATURE(cachedFlattenedPattern,
                                 KoPatternSP (KisLayerStyleFilterEnvironment::*)(KoPatternSP) const);
    ASSERT_ENVIRONMENT_SIGNATURE(cachedSelection, KisCachedSelection * (KisLayerStyleFilterEnvironment::*)());
    ASSERT_ENVIRONMENT_SIGNATURE(cachedPaintDevice, KisCachedPaintDevice * (KisLayerStyleFilterEnvironment::*)());
}

#undef ASSERT_ENVIRONMENT_SIGNATURE
#undef ASSERT_FILTER_SIGNATURE

QTEST_GUILESS_MAIN(KisLayerStyleConcreteFiltersSchemaContractTest)

#include "KisLayerStyleConcreteFiltersSchemaContractTest.moc"
