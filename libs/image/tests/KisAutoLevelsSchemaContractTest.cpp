/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAutoLevels.h"

#include <QTest>

#include <type_traits>

#define ASSERT_AUTO_LEVELS_FUNCTION(function, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAutoLevels::function)), signature>)

class KisAutoLevelsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void adjustmentMethodEnumSchemaRemainsStable();
    void channelHistogramValueSchemaRemainsStable();
    void inputPointAndGammaSignaturesRemainStable();
    void darkestAndWhitestColorSignatureRemainsStable();
    void channelAdjustmentSignaturesRemainStable();
};

void KisAutoLevelsSchemaContractTest::adjustmentMethodEnumSchemaRemainsStable()
{
    using namespace KisAutoLevels;

    static_assert(std::is_enum_v<ShadowsAndHighlightsAdjustmentMethod>);
    static_assert(ShadowsAndHighlightsAdjustmentMethod_MonochromaticContrast == 0);
    static_assert(ShadowsAndHighlightsAdjustmentMethod_PerChannelContrast == 1);
    static_assert(std::is_enum_v<MidtonesAdjustmentMethod>);
    static_assert(MidtonesAdjustmentMethod_None == 0);
    static_assert(MidtonesAdjustmentMethod_UseMedian == 1);
    static_assert(MidtonesAdjustmentMethod_UseMean == 2);
}

void KisAutoLevelsSchemaContractTest::channelHistogramValueSchemaRemainsStable()
{
    using Histogram = KisAutoLevels::ChannelHistogram;

    static_assert(std::is_class_v<Histogram>);
    static_assert(std::is_same_v<decltype(&Histogram::histogram), KisHistogram * Histogram::*>);
    static_assert(std::is_same_v<decltype(&Histogram::channel), int Histogram::*>);
}

void KisAutoLevelsSchemaContractTest::inputPointAndGammaSignaturesRemainStable()
{
    using Histogram = KisAutoLevels::ChannelHistogram;
    using PointPair = QPair<qreal, qreal>;

    ASSERT_AUTO_LEVELS_FUNCTION(getInputBlackAndWhitePoints, PointPair (*)(Histogram, qreal, qreal));
    ASSERT_AUTO_LEVELS_FUNCTION(getGamma, qreal (*)(qreal, qreal, qreal, qreal));
}

void KisAutoLevelsSchemaContractTest::darkestAndWhitestColorSignatureRemainsStable()
{
    using ColorPair = QPair<KoColor, KoColor>;

    ASSERT_AUTO_LEVELS_FUNCTION(getDarkestAndWhitestColors, ColorPair (*)(const KisPaintDeviceSP, qreal, qreal));
}

void KisAutoLevelsSchemaContractTest::channelAdjustmentSignaturesRemainStable()
{
    using Histogram = KisAutoLevels::ChannelHistogram;
    using Method = KisAutoLevels::MidtonesAdjustmentMethod;
    using Values = QVector<qreal>;
    using Result = QVector<KisLevelsCurve>;

    ASSERT_AUTO_LEVELS_FUNCTION(adjustMonochromaticContrast,
                                Result (*)(Histogram,
                                           QVector<Histogram> &,
                                           qreal,
                                           qreal,
                                           qreal,
                                           Method,
                                           qreal,
                                           const Values &,
                                           const Values &,
                                           const Values &));
    ASSERT_AUTO_LEVELS_FUNCTION(adjustPerChannelContrast,
                                Result (*)(QVector<Histogram> &,
                                           qreal,
                                           qreal,
                                           qreal,
                                           Method,
                                           qreal,
                                           const Values &,
                                           const Values &,
                                           const Values &));
}

QTEST_APPLESS_MAIN(KisAutoLevelsSchemaContractTest)

#include "KisAutoLevelsSchemaContractTest.moc"
