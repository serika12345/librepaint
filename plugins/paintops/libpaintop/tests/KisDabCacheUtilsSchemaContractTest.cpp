/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDabCacheUtils.h"

#include <QTest>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_DAB_FUNCTION_SIGNATURE(function, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDabCacheUtils::function)), signature>)

} // namespace

class KisDabCacheUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dabSchemaTypesAndFactoryRemainStable();
    void dabRequestValueSignaturesRemainStable();
    void dabGenerationValueSignaturesRemainStable();
    void dabRenderingResourceSignaturesRemainStable();
    void dabProcessingFunctionSignaturesRemainStable();
};

void KisDabCacheUtilsSchemaContractTest::dabSchemaTypesAndFactoryRemainStable()
{
    using namespace KisDabCacheUtils;

    static_assert(std::is_class_v<DabRequestInfo>);
    static_assert(std::is_class_v<DabGenerationInfo>);
    static_assert(std::is_class_v<DabRenderingResources>);
    static_assert(std::is_aggregate_v<DabGenerationInfo>);
    static_assert(std::is_polymorphic_v<DabRenderingResources>);
    static_assert(std::is_same_v<ResourcesFactory, std::function<DabRenderingResources *()>>);
}

void KisDabCacheUtilsSchemaContractTest::dabRequestValueSignaturesRemainStable()
{
    using namespace KisDabCacheUtils;

    static_assert(std::is_constructible_v<DabRequestInfo,
                                          const KoColor &,
                                          const QPointF &,
                                          const KisDabShape &,
                                          const KisPaintInformation &,
                                          qreal>);
    static_assert(std::is_constructible_v<DabRequestInfo,
                                          const KoColor &,
                                          const QPointF &,
                                          const KisDabShape &,
                                          const KisPaintInformation &,
                                          qreal,
                                          qreal>);
    static_assert(!std::is_copy_constructible_v<DabRequestInfo>);

    static_assert(std::is_same_v<decltype(DabRequestInfo::color), const KoColor &>);
    static_assert(std::is_same_v<decltype(DabRequestInfo::cursorPoint), const QPointF &>);
    static_assert(std::is_same_v<decltype(DabRequestInfo::shape), const KisDabShape &>);
    static_assert(std::is_same_v<decltype(DabRequestInfo::info), const KisPaintInformation &>);
    static_assert(std::is_same_v<decltype(DabRequestInfo::softnessFactor), const qreal>);
    static_assert(std::is_same_v<decltype(DabRequestInfo::lightnessStrength), const qreal>);
}

void KisDabCacheUtilsSchemaContractTest::dabGenerationValueSignaturesRemainStable()
{
    using namespace KisDabCacheUtils;

    static_assert(std::is_same_v<decltype(DabGenerationInfo::mirrorProperties), MirrorProperties>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::shape), KisDabShape>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::dstDabRect), QRect>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::subPixel), QPointF>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::solidColorFill), bool>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::paintColor), KoColor>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::info), KisPaintInformation>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::softnessFactor), qreal>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::lightnessStrength), qreal>);
    static_assert(std::is_same_v<decltype(DabGenerationInfo::needsPostprocessing), bool>);
}

void KisDabCacheUtilsSchemaContractTest::dabRenderingResourceSignaturesRemainStable()
{
    using namespace KisDabCacheUtils;
    using SyncSignature = void (DabRenderingResources::*)(int, const KisPaintInformation &);

    static_assert(std::is_default_constructible_v<DabRenderingResources>);
    static_assert(std::is_destructible_v<DabRenderingResources>);
    static_assert(std::has_virtual_destructor_v<DabRenderingResources>);
    static_assert(!std::is_copy_constructible_v<DabRenderingResources>);
    static_assert(std::is_same_v<decltype(static_cast<SyncSignature>(&DabRenderingResources::syncResourcesToSeqNo)),
                                 SyncSignature>);

    static_assert(std::is_same_v<decltype(DabRenderingResources::brush), KisBrushSP>);
    static_assert(std::is_same_v<decltype(DabRenderingResources::colorSource), std::unique_ptr<KisColorSource>>);
    static_assert(
        std::is_same_v<decltype(DabRenderingResources::sharpnessOption), std::unique_ptr<KisSharpnessOption>>);
    static_assert(std::is_same_v<decltype(DabRenderingResources::textureOption), std::unique_ptr<KisTextureOption>>);
    static_assert(std::is_same_v<decltype(DabRenderingResources::colorSourceDevice), KisPaintDeviceSP>);
}

void KisDabCacheUtilsSchemaContractTest::dabProcessingFunctionSignaturesRemainStable()
{
    using namespace KisDabCacheUtils;
    using CorrectRectSignature = QRect (*)(const QRect &, const QSize &);
    using GenerateSignature =
        void (*)(const DabGenerationInfo &, DabRenderingResources *, KisFixedPaintDeviceSP *, bool);
    using PostProcessSignature =
        void (*)(KisFixedPaintDeviceSP, const QPoint &, const KisPaintInformation &, DabRenderingResources *);

    ASSERT_DAB_FUNCTION_SIGNATURE(correctDabRectWhenFetchedFromCache, CorrectRectSignature);
    ASSERT_DAB_FUNCTION_SIGNATURE(generateDab, GenerateSignature);
    ASSERT_DAB_FUNCTION_SIGNATURE(postProcessDab, PostProcessSignature);

    static_assert(std::is_same_v<decltype(generateDab(std::declval<const DabGenerationInfo &>(),
                                                      std::declval<DabRenderingResources *>(),
                                                      std::declval<KisFixedPaintDeviceSP *>())),
                                 void>);
}

#undef ASSERT_DAB_FUNCTION_SIGNATURE

QTEST_GUILESS_MAIN(KisDabCacheUtilsSchemaContractTest)

#include "KisDabCacheUtilsSchemaContractTest.moc"
