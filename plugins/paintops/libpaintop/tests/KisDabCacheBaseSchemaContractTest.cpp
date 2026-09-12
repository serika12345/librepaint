/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_dab_cache.h"
#include "kis_dab_cache_base.h"

#include <QTest>

#include <type_traits>

class KisDabCacheBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeRemainStable();
    void optionConfigurationSignaturesRemainStable();
    void cacheBehaviorControlSignaturesRemainStable();
    void dabCacheTypeConstructionAndLifetimeSchemaRemainStable();
    void dabCacheFetchSignaturesRemainStable();
    void dabCachePostprocessingSignaturesRemainStable();
};

void KisDabCacheBaseSchemaContractTest::typeConstructionAndLifetimeRemainStable()
{
    static_assert(std::is_class_v<KisDabCacheBase>);
    static_assert(std::is_default_constructible_v<KisDabCacheBase>);
    static_assert(std::is_destructible_v<KisDabCacheBase>);

    QVERIFY(true);
}

void KisDabCacheBaseSchemaContractTest::optionConfigurationSignaturesRemainStable()
{
    using Cache = KisDabCacheBase;

    static_assert(std::is_same_v<decltype(&Cache::setMirrorPostprocessing), void (Cache::*)(KisMirrorOption *)>);
    static_assert(std::is_same_v<decltype(&Cache::setPrecisionOption), void (Cache::*)(KisPrecisionOption *)>);

    QVERIFY(true);
}

void KisDabCacheBaseSchemaContractTest::cacheBehaviorControlSignaturesRemainStable()
{
    using Cache = KisDabCacheBase;

    static_assert(std::is_same_v<decltype(&Cache::disableSubpixelPrecision), void (Cache::*)()>);
    static_assert(std::is_same_v<decltype(&Cache::needSeparateOriginal),
                                 bool (Cache::*)(KisTextureOption *, KisSharpnessOption *) const>);

    QVERIFY(true);
}

void KisDabCacheBaseSchemaContractTest::dabCacheTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Cache = KisDabCache;

    static_assert(std::is_class_v<Cache>);
    static_assert(std::is_constructible_v<Cache, KisBrushSP>);
    static_assert(std::is_destructible_v<Cache>);

    QVERIFY(true);
}

void KisDabCacheBaseSchemaContractTest::dabCacheFetchSignaturesRemainStable()
{
    using Cache = KisDabCache;
    using ColorSourceFetch = KisFixedPaintDeviceSP (Cache::*)(const KoColorSpace *,
                                                              KisColorSource *,
                                                              const QPointF &,
                                                              const KisDabShape &,
                                                              const KisPaintInformation &,
                                                              qreal,
                                                              QRect *,
                                                              qreal);
    using ColorFetch = KisFixedPaintDeviceSP (Cache::*)(const KoColorSpace *,
                                                        const KoColor &,
                                                        const QPointF &,
                                                        const KisDabShape &,
                                                        const KisPaintInformation &,
                                                        qreal,
                                                        QRect *,
                                                        qreal);
    using NormalizedFetch = KisFixedPaintDeviceSP (Cache::*)(const KoColorSpace *,
                                                             const QPointF &,
                                                             const KisDabShape &,
                                                             const KisPaintInformation &,
                                                             qreal,
                                                             QRect *);

    static_assert(std::is_same_v<decltype(static_cast<ColorSourceFetch>(&Cache::fetchDab)), ColorSourceFetch>);
    static_assert(std::is_same_v<decltype(static_cast<ColorFetch>(&Cache::fetchDab)), ColorFetch>);
    static_assert(std::is_same_v<decltype(&Cache::fetchNormalizedImageDab), NormalizedFetch>);

    QVERIFY(true);
}

void KisDabCacheBaseSchemaContractTest::dabCachePostprocessingSignaturesRemainStable()
{
    using Cache = KisDabCache;

    static_assert(std::is_same_v<decltype(&Cache::setSharpnessPostprocessing), void (Cache::*)(KisSharpnessOption *)>);
    static_assert(std::is_same_v<decltype(&Cache::setTexturePostprocessing), void (Cache::*)(KisTextureOption *)>);
    static_assert(std::is_same_v<decltype(&Cache::needSeparateOriginal), bool (Cache::*)() const>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisDabCacheBaseSchemaContractTest)

#include "KisDabCacheBaseSchemaContractTest.moc"
