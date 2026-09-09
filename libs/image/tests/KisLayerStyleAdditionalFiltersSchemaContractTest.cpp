/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "layerstyles/KisLayerStyleKnockoutBlower.h"
#include "layerstyles/kis_ls_bevel_emboss_filter.h"
#include "layerstyles/kis_ls_satin_filter.h"
#include "layerstyles/kis_ls_stroke_filter.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_FILTER_SIGNATURE(type, method, ...)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)
#define ASSERT_BLOWER_SIGNATURE(method, ...)                                                                           \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisLayerStyleKnockoutBlower::method)), __VA_ARGS__>)

} // namespace

class KisLayerStyleAdditionalFiltersSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bevelTypeConstructionAndCloneSignaturesRemainStable();
    void bevelProcessingSignaturesRemainStable();
    void satinTypeConstructionAndCloneSignaturesRemainStable();
    void satinProcessingSignaturesRemainStable();
    void strokeTypeConstructionAndCloneSignaturesRemainStable();
    void strokeProcessingSignaturesRemainStable();
    void blowerTypeAndConstructionSchemaRemainStable();
    void blowerSelectionAndApplicationSignaturesRemainStable();
};

// clang-format off
void KisLayerStyleAdditionalFiltersSchemaContractTest::bevelTypeConstructionAndCloneSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_class_v<KisLsBevelEmbossFilter>);
    static_assert(std::is_base_of_v<KisLayerStyleFilter, KisLsBevelEmbossFilter>);
    static_assert(std::is_default_constructible_v<KisLsBevelEmbossFilter>);
    ASSERT_FILTER_SIGNATURE(KisLsBevelEmbossFilter, clone, KisLayerStyleFilter * (KisLsBevelEmbossFilter::*)() const);
}

void KisLayerStyleAdditionalFiltersSchemaContractTest::bevelProcessingSignaturesRemainStable()
{
    ASSERT_FILTER_SIGNATURE(KisLsBevelEmbossFilter,
                            processDirectly,
                            void (KisLsBevelEmbossFilter::*)(KisPaintDeviceSP,
                                                             KisMultipleProjection *,
                                                             KisLayerStyleKnockoutBlower *,
                                                             const QRect &,
                                                             KisPSDLayerStyleSP,
                                                             KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsBevelEmbossFilter,
        neededRect,
        QRect (KisLsBevelEmbossFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsBevelEmbossFilter,
        changedRect,
        QRect (KisLsBevelEmbossFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
}

// clang-format off
void KisLayerStyleAdditionalFiltersSchemaContractTest::satinTypeConstructionAndCloneSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_class_v<KisLsSatinFilter>);
    static_assert(std::is_base_of_v<KisLayerStyleFilter, KisLsSatinFilter>);
    static_assert(std::is_default_constructible_v<KisLsSatinFilter>);
    ASSERT_FILTER_SIGNATURE(KisLsSatinFilter, clone, KisLayerStyleFilter * (KisLsSatinFilter::*)() const);
}

void KisLayerStyleAdditionalFiltersSchemaContractTest::satinProcessingSignaturesRemainStable()
{
    ASSERT_FILTER_SIGNATURE(KisLsSatinFilter,
                            processDirectly,
                            void (KisLsSatinFilter::*)(KisPaintDeviceSP,
                                                       KisMultipleProjection *,
                                                       KisLayerStyleKnockoutBlower *,
                                                       const QRect &,
                                                       KisPSDLayerStyleSP,
                                                       KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsSatinFilter,
        neededRect,
        QRect (KisLsSatinFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsSatinFilter,
        changedRect,
        QRect (KisLsSatinFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
}

// clang-format off
void KisLayerStyleAdditionalFiltersSchemaContractTest::strokeTypeConstructionAndCloneSignaturesRemainStable()
// clang-format on
{
    static_assert(std::is_class_v<KisLsStrokeFilter>);
    static_assert(std::is_base_of_v<KisLayerStyleFilter, KisLsStrokeFilter>);
    static_assert(std::is_default_constructible_v<KisLsStrokeFilter>);
    ASSERT_FILTER_SIGNATURE(KisLsStrokeFilter, clone, KisLayerStyleFilter * (KisLsStrokeFilter::*)() const);
}

void KisLayerStyleAdditionalFiltersSchemaContractTest::strokeProcessingSignaturesRemainStable()
{
    ASSERT_FILTER_SIGNATURE(KisLsStrokeFilter,
                            processDirectly,
                            void (KisLsStrokeFilter::*)(KisPaintDeviceSP,
                                                        KisMultipleProjection *,
                                                        KisLayerStyleKnockoutBlower *,
                                                        const QRect &,
                                                        KisPSDLayerStyleSP,
                                                        KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsStrokeFilter,
        neededRect,
        QRect (KisLsStrokeFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(
        KisLsStrokeFilter,
        changedRect,
        QRect (KisLsStrokeFilter::*)(const QRect &, KisPSDLayerStyleSP, KisLayerStyleFilterEnvironment *) const);
    ASSERT_FILTER_SIGNATURE(KisLsStrokeFilter,
                            sourcePlaneOpacityThresholdRequirement,
                            KritaUtils::ThresholdMode (KisLsStrokeFilter::*)(KisPSDLayerStyleSP) const);
}

void KisLayerStyleAdditionalFiltersSchemaContractTest::blowerTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisLayerStyleKnockoutBlower>);
    static_assert(std::is_default_constructible_v<KisLayerStyleKnockoutBlower>);
    static_assert(std::is_copy_constructible_v<KisLayerStyleKnockoutBlower>);

    QVERIFY(true);
}

// clang-format off
void KisLayerStyleAdditionalFiltersSchemaContractTest::blowerSelectionAndApplicationSignaturesRemainStable()
// clang-format on
{
    ASSERT_BLOWER_SIGNATURE(knockoutSelectionLazy, KisSelectionSP (KisLayerStyleKnockoutBlower::*)());
    ASSERT_BLOWER_SIGNATURE(setKnockoutSelection, void (KisLayerStyleKnockoutBlower::*)(KisSelectionSP));
    ASSERT_BLOWER_SIGNATURE(resetKnockoutSelection, void (KisLayerStyleKnockoutBlower::*)());
    ASSERT_BLOWER_SIGNATURE(apply,
                            void (KisLayerStyleKnockoutBlower::*)(KisPainter *, KisPaintDeviceSP, const QRect &) const);
    ASSERT_BLOWER_SIGNATURE(isEmpty, bool (KisLayerStyleKnockoutBlower::*)() const);
}

#undef ASSERT_BLOWER_SIGNATURE
#undef ASSERT_FILTER_SIGNATURE

QTEST_GUILESS_MAIN(KisLayerStyleAdditionalFiltersSchemaContractTest)

#include "KisLayerStyleAdditionalFiltersSchemaContractTest.moc"
