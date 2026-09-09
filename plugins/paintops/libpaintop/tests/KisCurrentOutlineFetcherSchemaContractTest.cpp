/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_current_outline_fetcher.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisCurrentOutlineFetcherSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void optionSchemaRemainsStable();
    void invalidationSignatureRemainsStable();
    void outlineFetchSignatureRemainsStable();
};

void KisCurrentOutlineFetcherSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Fetcher = KisCurrentOutlineFetcher;

    static_assert(std::is_class_v<Fetcher>);
    static_assert(std::is_constructible_v<Fetcher, Fetcher::Options>);
    static_assert(std::is_destructible_v<Fetcher>);
}

void KisCurrentOutlineFetcherSchemaContractTest::optionSchemaRemainsStable()
{
    using Fetcher = KisCurrentOutlineFetcher;

    static_assert(std::is_enum_v<Fetcher::Option>);
    static_assert(std::is_same_v<Fetcher::Options, QFlags<Fetcher::Option>>);
    static_assert(static_cast<int>(Fetcher::NO_OPTION) == 0);
    static_assert(static_cast<int>(Fetcher::SIZE_OPTION) == 1);
    static_assert(static_cast<int>(Fetcher::ROTATION_OPTION) == 2);
    static_assert(static_cast<int>(Fetcher::MIRROR_OPTION) == 3);
    static_assert(static_cast<int>(Fetcher::SHARPNESS_OPTION) == 4);
}

void KisCurrentOutlineFetcherSchemaContractTest::invalidationSignatureRemainsStable()
{
    using Fetcher = KisCurrentOutlineFetcher;

    static_assert(std::is_same_v<decltype(&Fetcher::setDirty), void (Fetcher::*)()>);
}

void KisCurrentOutlineFetcherSchemaContractTest::outlineFetchSignatureRemainsStable()
{
    using Fetcher = KisCurrentOutlineFetcher;
    using Signature = KisOptimizedBrushOutline (Fetcher::*)(const KisPaintInformation &,
                                                            const KisPaintOpSettingsSP,
                                                            const KisOptimizedBrushOutline &,
                                                            const KisPaintOpSettings::OutlineMode &,
                                                            qreal,
                                                            qreal,
                                                            qreal,
                                                            bool,
                                                            qreal,
                                                            qreal) const;

    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Fetcher::fetchOutline)), Signature>);
    static_assert(std::is_same_v<decltype(std::declval<const Fetcher &>().fetchOutline(
                                     std::declval<const KisPaintInformation &>(),
                                     std::declval<KisPaintOpSettingsSP>(),
                                     std::declval<const KisOptimizedBrushOutline &>(),
                                     std::declval<const KisPaintOpSettings::OutlineMode &>(),
                                     std::declval<qreal>())),
                                 KisOptimizedBrushOutline>);
}

QTEST_GUILESS_MAIN(KisCurrentOutlineFetcherSchemaContractTest)

#include "KisCurrentOutlineFetcherSchemaContractTest.moc"
