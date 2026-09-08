/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDabCacheUtils.h"
#include "kis_simple_paintop_factory.h"

#include <QTest>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace
{

struct FactoryOperation;
struct FactorySettings;
struct FactorySettingsWidget;

struct NoFactoryCapabilities {
};

struct FactoryCapabilities {
    static void preinitializeOpStatically(KisPaintOpSettingsSP);
    static QList<KoResourceLoadResult> prepareLinkedResources(KisPaintOpSettingsSP, KisResourcesInterfaceSP);
    static QList<KoResourceLoadResult> prepareEmbeddedResources(KisPaintOpSettingsSP, KisResourcesInterfaceSP);
    static KisInterstrokeDataFactory *createInterstrokeDataFactory(KisPaintOpSettingsSP, KisResourcesInterfaceSP);
};

struct ExtendedFactoryWidget {
    ExtendedFactoryWidget(QWidget *, KisResourcesInterfaceSP, KoCanvasResourcesInterfaceSP);
};

using SimpleFactory = KisSimplePaintOpFactory<FactoryOperation, FactorySettings, FactorySettingsWidget>;

#define ASSERT_DAB_FUNCTION_SIGNATURE(function, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDabCacheUtils::function)), signature>)
#define ASSERT_SIMPLE_FACTORY_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SimpleFactory::method)), signature>)

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
    void simpleFactoryTypeAndCapabilityDetectionSchemaRemainStable();
    void simpleFactoryResourcePreparationHelperSignaturesRemainStable();
    void simpleFactoryCreationHelperSignaturesRemainStable();
    void simpleFactoryConstructionAndCreationSignaturesRemainStable();
    void simpleFactoryIdentityAndResourceSignaturesRemainStable();
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

void KisDabCacheUtilsSchemaContractTest::simpleFactoryTypeAndCapabilityDetectionSchemaRemainStable()
{
    static_assert(std::is_class_v<SimpleFactory>);
    static_assert(detail::has_create_interstroke_data_factory<FactoryCapabilities>::value);
    static_assert(detail::has_preinitialize_statically<FactoryCapabilities>::value);
    static_assert(detail::has_prepare_embedded_resources<FactoryCapabilities>::value);
    static_assert(detail::has_prepare_linked_resources<FactoryCapabilities>::value);
    static_assert(detail::supports_extended_initilization<ExtendedFactoryWidget>::value);

    static_assert(!detail::has_create_interstroke_data_factory<NoFactoryCapabilities>::value);
    static_assert(!detail::has_preinitialize_statically<NoFactoryCapabilities>::value);
    static_assert(!detail::has_prepare_embedded_resources<NoFactoryCapabilities>::value);
    static_assert(!detail::has_prepare_linked_resources<NoFactoryCapabilities>::value);
    static_assert(!detail::supports_extended_initilization<NoFactoryCapabilities>::value);
}

void KisDabCacheUtilsSchemaContractTest::simpleFactoryResourcePreparationHelperSignaturesRemainStable()
{
    using Helper = QList<KoResourceLoadResult> (*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP, void *);

    static_assert(
        std::is_same_v<decltype(static_cast<Helper>(&detail::prepareEmbeddedResources<FactoryCapabilities>)), Helper>);
    static_assert(
        std::is_same_v<decltype(static_cast<Helper>(&detail::prepareEmbeddedResources<NoFactoryCapabilities>)),
                       Helper>);
    static_assert(
        std::is_same_v<decltype(static_cast<Helper>(&detail::prepareLinkedResources<FactoryCapabilities>)), Helper>);
    static_assert(
        std::is_same_v<decltype(static_cast<Helper>(&detail::prepareLinkedResources<NoFactoryCapabilities>)), Helper>);
}

void KisDabCacheUtilsSchemaContractTest::simpleFactoryCreationHelperSignaturesRemainStable()
{
    using ConfigWidgetHelper =
        KisPaintOpConfigWidget *(*)(QWidget *, KisResourcesInterfaceSP, KoCanvasResourcesInterfaceSP, void *);
    using InterstrokeHelper = KisInterstrokeDataFactory *(*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP, void *);
    using PreinitializeHelper = void (*)(KisPaintOpSettingsSP, void *);

    static_assert(
        std::is_same_v<decltype(static_cast<ConfigWidgetHelper>(&detail::createConfigWidget<ExtendedFactoryWidget>)),
                       ConfigWidgetHelper>);
    static_assert(
        std::is_same_v<decltype(static_cast<ConfigWidgetHelper>(&detail::createConfigWidget<NoFactoryCapabilities>)),
                       ConfigWidgetHelper>);
    static_assert(std::is_same_v<decltype(static_cast<InterstrokeHelper>(
                                     &detail::createInterstrokeDataFactory<FactoryCapabilities>)),
                                 InterstrokeHelper>);
    static_assert(std::is_same_v<decltype(static_cast<InterstrokeHelper>(
                                     &detail::createInterstrokeDataFactory<NoFactoryCapabilities>)),
                                 InterstrokeHelper>);
    static_assert(std::is_same_v<decltype(static_cast<PreinitializeHelper>(
                                     &detail::preinitializeOpStatically<FactoryCapabilities>)),
                                 PreinitializeHelper>);
    static_assert(std::is_same_v<decltype(static_cast<PreinitializeHelper>(
                                     &detail::preinitializeOpStatically<NoFactoryCapabilities>)),
                                 PreinitializeHelper>);
}

void KisDabCacheUtilsSchemaContractTest::simpleFactoryConstructionAndCreationSignaturesRemainStable()
{
    static_assert(std::is_constructible_v<SimpleFactory,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QStringList &,
                                          int,
                                          bool>);
    static_assert(std::is_destructible_v<SimpleFactory>);
    ASSERT_SIMPLE_FACTORY_SIGNATURE(
        createConfigWidget,
        KisPaintOpConfigWidget * (SimpleFactory::*)(QWidget *, KisResourcesInterfaceSP, KoCanvasResourcesInterfaceSP));
    ASSERT_SIMPLE_FACTORY_SIGNATURE(createInterstrokeDataFactory,
                                    KisInterstrokeDataFactory
                                        * (SimpleFactory::*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP) const);
    ASSERT_SIMPLE_FACTORY_SIGNATURE(
        createOp,
        KisPaintOp * (SimpleFactory::*)(KisPaintOpSettingsSP, KisPainter *, KisNodeSP, KisImageSP));
    ASSERT_SIMPLE_FACTORY_SIGNATURE(createSettings, KisPaintOpSettingsSP (SimpleFactory::*)(KisResourcesInterfaceSP));
    ASSERT_SIMPLE_FACTORY_SIGNATURE(preinitializePaintOpIfNeeded, void (SimpleFactory::*)(KisPaintOpSettingsSP));
}

void KisDabCacheUtilsSchemaContractTest::simpleFactoryIdentityAndResourceSignaturesRemainStable()
{
    ASSERT_SIMPLE_FACTORY_SIGNATURE(category, QString (SimpleFactory::*)() const);
    ASSERT_SIMPLE_FACTORY_SIGNATURE(icon, QIcon (SimpleFactory::*)());
    ASSERT_SIMPLE_FACTORY_SIGNATURE(id, QString (SimpleFactory::*)() const);
    ASSERT_SIMPLE_FACTORY_SIGNATURE(lodSizeThresholdSupported, bool (SimpleFactory::*)() const);
    ASSERT_SIMPLE_FACTORY_SIGNATURE(name, QString (SimpleFactory::*)() const);
    ASSERT_SIMPLE_FACTORY_SIGNATURE(
        prepareEmbeddedResources,
        QList<KoResourceLoadResult> (SimpleFactory::*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP));
    ASSERT_SIMPLE_FACTORY_SIGNATURE(
        prepareLinkedResources,
        QList<KoResourceLoadResult> (SimpleFactory::*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP));
}

#undef ASSERT_SIMPLE_FACTORY_SIGNATURE
#undef ASSERT_DAB_FUNCTION_SIGNATURE

QTEST_GUILESS_MAIN(KisDabCacheUtilsSchemaContractTest)

#include "KisDabCacheUtilsSchemaContractTest.moc"
