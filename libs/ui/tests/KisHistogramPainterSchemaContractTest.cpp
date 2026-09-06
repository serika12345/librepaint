/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisHistogramPainter.h"
#include "widgets/KisHistogramView.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_HISTOGRAM_PAINTER_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisHistogramPainter::method)), signature>)
} // namespace

class KisHistogramPainterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void histogramPainterTypeLifetimeAndSetupSchemaRemainStable();
    void histogramPainterRenderingSignaturesRemainStable();
    void histogramPainterChannelSelectionSignaturesRemainStable();
    void histogramPainterColorAndScaleSignaturesRemainStable();
    void histogramPainterPeakAndLogarithmicPolicySignaturesRemainStable();
    void histogramViewTypeLifetimeAndSetupSchemaRemainStable();
    void histogramViewChannelQuerySignaturesRemainStable();
    void histogramViewChannelSelectionSignaturesRemainStable();
    void histogramViewColorAndScaleSignaturesRemainStable();
    void histogramViewLogarithmicPolicySignaturesRemainStable();
};

void KisHistogramPainterSchemaContractTest::histogramPainterTypeLifetimeAndSetupSchemaRemainStable()
{
    static_assert(std::is_class_v<KisHistogramPainter>);
    static_assert(std::is_default_constructible_v<KisHistogramPainter>);
    static_assert(std::is_destructible_v<KisHistogramPainter>);
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(
        setup,
        void (KisHistogramPainter::*)(KisHistogram *, const KoColorSpace *, QVector<int>));

    using DefaultChannelsResult =
        decltype(std::declval<KisHistogramPainter &>().setup(std::declval<KisHistogram *>(),
                                                             std::declval<const KoColorSpace *>()));
    static_assert(std::is_same_v<DefaultChannelsResult, void>);
}

void KisHistogramPainterSchemaContractTest::histogramPainterRenderingSignaturesRemainStable()
{
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(paint, QImage (KisHistogramPainter::*)(const QSize &));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(paint, QImage (KisHistogramPainter::*)(int, int));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(paint, void (KisHistogramPainter::*)(QPainter &, const QRect &));
}

void KisHistogramPainterSchemaContractTest::histogramPainterChannelSelectionSignaturesRemainStable()
{
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(availableChannels, QList<int> (KisHistogramPainter::*)() const);
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(channels, const QVector<int> &(KisHistogramPainter::*)() const);
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setChannel, void (KisHistogramPainter::*)(int));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setChannels, void (KisHistogramPainter::*)(const QVector<int> &));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(totalNumberOfAvailableChannels, int (KisHistogramPainter::*)() const);
}

void KisHistogramPainterSchemaContractTest::histogramPainterColorAndScaleSignaturesRemainStable()
{
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(defaultColor, QColor (KisHistogramPainter::*)() const);
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(scale, qreal (KisHistogramPainter::*)() const);
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setDefaultColor, void (KisHistogramPainter::*)(const QColor &));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setScale, void (KisHistogramPainter::*)(qreal));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setScaleToFit, void (KisHistogramPainter::*)());
}

void KisHistogramPainterSchemaContractTest::histogramPainterPeakAndLogarithmicPolicySignaturesRemainStable()
{
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(isLogarithmic, bool (KisHistogramPainter::*)() const);
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setLogarithmic, void (KisHistogramPainter::*)(bool));
    ASSERT_HISTOGRAM_PAINTER_SIGNATURE(setScaleToCutLongPeaks, void (KisHistogramPainter::*)());
}

void KisHistogramPainterSchemaContractTest::histogramViewTypeLifetimeAndSetupSchemaRemainStable()
{
    using View = KisHistogramView;
    using Setup = void (
        View::*)(const QVector<KisHistogram *> &, const QVector<const KoColorSpace *> &, const QVector<QVector<int>> &);
    static_assert(std::is_class_v<View> && std::is_base_of_v<QWidget, View>);
    static_assert(std::is_constructible_v<View, QWidget *> && std::has_virtual_destructor_v<View>);
    static_assert(std::is_same_v<decltype(&View::setup), Setup>);
    static_assert(
        std::is_same_v<decltype(std::declval<View &>().setup(std::declval<const QVector<KisHistogram *> &>(),
                                                             std::declval<const QVector<const KoColorSpace *> &>())),
                       void>);
}

void KisHistogramPainterSchemaContractTest::histogramViewChannelQuerySignaturesRemainStable()
{
    using View = KisHistogramView;
    static_assert(std::is_same_v<decltype(&View::channels), const QVector<int> &(View::*)() const>);
}

void KisHistogramPainterSchemaContractTest::histogramViewChannelSelectionSignaturesRemainStable()
{
    using View = KisHistogramView;
    static_assert(std::is_same_v<decltype(&View::setChannel), void (View::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&View::setChannels), void (View::*)(const QVector<int> &, int)>);
    static_assert(std::is_same_v<decltype(&View::clearChannels), void (View::*)()>);
    static_assert(std::is_same_v<decltype(std::declval<View &>().setChannel(int{})), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<View &>().setChannels(std::declval<const QVector<int> &>())), void>);
}

void KisHistogramPainterSchemaContractTest::histogramViewColorAndScaleSignaturesRemainStable()
{
    using View = KisHistogramView;
    static_assert(std::is_same_v<decltype(&View::defaultColor), QColor (View::*)() const>);
    static_assert(std::is_same_v<decltype(&View::scale), qreal (View::*)() const>);
    static_assert(std::is_same_v<decltype(&View::setDefaultColor), void (View::*)(const QColor &)>);
    static_assert(std::is_same_v<decltype(&View::setScale), void (View::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&View::setScaleToFit), void (View::*)()>);
    static_assert(std::is_same_v<decltype(&View::setScaleToCutLongPeaks), void (View::*)()>);
}

void KisHistogramPainterSchemaContractTest::histogramViewLogarithmicPolicySignaturesRemainStable()
{
    using View = KisHistogramView;
    static_assert(std::is_same_v<decltype(&View::isLogarithmic), bool (View::*)() const>);
    static_assert(std::is_same_v<decltype(&View::setLogarithmic), void (View::*)(bool)>);
}

QTEST_MAIN(KisHistogramPainterSchemaContractTest)

#include "KisHistogramPainterSchemaContractTest.moc"
