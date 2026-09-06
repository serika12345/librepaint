/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lazybrush/kis_lazy_fill_tools.h"

#include <QTest>

#include <type_traits>

class KisLazyFillToolsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void alphaNormalizationSignaturesRemainStable();
    void componentSegmentationSignaturesRemainStable();
    void keyStrokeTypeAndConstructionSchemaRemainStable();
    void keyStrokeValueAndEqualitySchemaRemainStable();
    void filteringOptionsValuesAndEqualityRemainStable();
};

void KisLazyFillToolsContractTest::alphaNormalizationSignaturesRemainStable()
{
    using NormalizeSignature = void (*)(KisPaintDeviceSP, const QRect &);

    static_assert(std::is_same_v<decltype(&KisLazyFillTools::normalizeAlpha8Device), NormalizeSignature>);
    static_assert(std::is_same_v<decltype(&KisLazyFillTools::normalizeAndInvertAlpha8Device), NormalizeSignature>);
}

void KisLazyFillToolsContractTest::componentSegmentationSignaturesRemainStable()
{
    using CutSignature = void (*)(const KoColor &,
                                  KisPaintDeviceSP,
                                  KisPaintDeviceSP,
                                  KisPaintDeviceSP,
                                  KisPaintDeviceSP,
                                  KisPaintDeviceSP,
                                  const QRect &);
    using SplitSignature = QVector<QPoint> (*)(KisPaintDeviceSP, const QRect &);

    static_assert(std::is_same_v<decltype(&KisLazyFillTools::cutOneWay), CutSignature>);
    static_assert(std::is_same_v<decltype(&KisLazyFillTools::splitIntoConnectedComponents), SplitSignature>);
}

void KisLazyFillToolsContractTest::keyStrokeTypeAndConstructionSchemaRemainStable()
{
    using KeyStroke = KisLazyFillTools::KeyStroke;

    static_assert(std::is_class_v<KeyStroke>);
    static_assert(std::is_default_constructible_v<KeyStroke>);
    static_assert(std::is_constructible_v<KeyStroke, KisPaintDeviceSP, const KoColor &, bool>);
}

void KisLazyFillToolsContractTest::keyStrokeValueAndEqualitySchemaRemainStable()
{
    using KeyStroke = KisLazyFillTools::KeyStroke;

    static_assert(std::is_same_v<decltype(KeyStroke::dev), KisPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(KeyStroke::color), KoColor>);
    static_assert(std::is_same_v<decltype(KeyStroke::isTransparent), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KeyStroke &>() == std::declval<const KeyStroke &>()), bool>);
}

void KisLazyFillToolsContractTest::filteringOptionsValuesAndEqualityRemainStable()
{
    using FilteringOptions = KisLazyFillTools::FilteringOptions;

    static_assert(std::is_class_v<FilteringOptions>);
    static_assert(std::is_default_constructible_v<FilteringOptions>);
    static_assert(std::is_constructible_v<FilteringOptions, bool, qreal, qreal, qreal>);
    static_assert(std::is_same_v<decltype(FilteringOptions::useEdgeDetection), bool>);
    static_assert(std::is_same_v<decltype(FilteringOptions::edgeDetectionSize), qreal>);
    static_assert(std::is_same_v<decltype(FilteringOptions::fuzzyRadius), qreal>);
    static_assert(std::is_same_v<decltype(FilteringOptions::cleanUpAmount), qreal>);
    static_assert(
        std::is_same_v<decltype(std::declval<const FilteringOptions &>() == std::declval<const FilteringOptions &>()),
                       bool>);

    const FilteringOptions defaults;
    QCOMPARE(defaults.useEdgeDetection, false);
    QCOMPARE(defaults.edgeDetectionSize, 4.0);
    QCOMPARE(defaults.fuzzyRadius, 0.0);
    QCOMPARE(defaults.cleanUpAmount, 0.0);

    const FilteringOptions configured(true, 8.0, 2.0, 1.0);
    const FilteringOptions same(true, 8.0, 2.0, 1.0);
    QCOMPARE(configured.useEdgeDetection, true);
    QCOMPARE(configured.edgeDetectionSize, 8.0);
    QCOMPARE(configured.fuzzyRadius, 2.0);
    QCOMPARE(configured.cleanUpAmount, 1.0);
    QVERIFY(configured == same);

    FilteringOptions changed = same;
    changed.useEdgeDetection = false;
    QVERIFY(!(configured == changed));
    changed = same;
    changed.edgeDetectionSize = 9.0;
    QVERIFY(!(configured == changed));
    changed = same;
    changed.fuzzyRadius = 3.0;
    QVERIFY(!(configured == changed));
    changed = same;
    changed.cleanUpAmount = 2.0;
    QVERIFY(!(configured == changed));
}

QTEST_APPLESS_MAIN(KisLazyFillToolsContractTest)

#include "KisLazyFillToolsContractTest.moc"
