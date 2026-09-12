/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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

QTEST_GUILESS_MAIN(KisDabCacheBaseSchemaContractTest)

#include "KisDabCacheBaseSchemaContractTest.moc"
