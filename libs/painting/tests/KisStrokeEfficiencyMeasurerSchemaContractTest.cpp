/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "strokes/KisStrokeEfficiencyMeasurer.h"

#include <QTest>

#include <type_traits>

#define ASSERT_STROKE_EFFICIENCY_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisStrokeEfficiencyMeasurer::method)), signature>)

class KisStrokeEfficiencyMeasurerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strokeEfficiencyMeasurerTypeAndLifetimeSchemaRemainStable();
    void strokeEfficiencyMeasurerStateControlSignaturesRemainStable();
    void strokeEfficiencyMeasurerCursorSamplingSignaturesRemainStable();
    void strokeEfficiencyMeasurerRenderingNotificationSignaturesRemainStable();
    void strokeEfficiencyMeasurerMetricQuerySignaturesRemainStable();
};

void KisStrokeEfficiencyMeasurerSchemaContractTest::strokeEfficiencyMeasurerTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisStrokeEfficiencyMeasurer>);
    static_assert(std::is_default_constructible_v<KisStrokeEfficiencyMeasurer>);
    static_assert(std::is_destructible_v<KisStrokeEfficiencyMeasurer>);
}

void KisStrokeEfficiencyMeasurerSchemaContractTest::strokeEfficiencyMeasurerStateControlSignaturesRemainStable()
{
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(isEnabled, bool (KisStrokeEfficiencyMeasurer::*)() const);
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(reset, void (KisStrokeEfficiencyMeasurer::*)());
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(setEnabled, void (KisStrokeEfficiencyMeasurer::*)(bool));
}

void KisStrokeEfficiencyMeasurerSchemaContractTest::strokeEfficiencyMeasurerCursorSamplingSignaturesRemainStable()
{
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(addSample, void (KisStrokeEfficiencyMeasurer::*)(const QPointF &));
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(addSamples, void (KisStrokeEfficiencyMeasurer::*)(const QVector<QPointF> &));
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(notifyCursorMoveFinished, void (KisStrokeEfficiencyMeasurer::*)());
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(notifyCursorMoveStarted, void (KisStrokeEfficiencyMeasurer::*)());
}

// clang-format off
void KisStrokeEfficiencyMeasurerSchemaContractTest::strokeEfficiencyMeasurerRenderingNotificationSignaturesRemainStable()
// clang-format on
{
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(notifyFrameRenderingStarted, void (KisStrokeEfficiencyMeasurer::*)());
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(notifyRenderingFinished, void (KisStrokeEfficiencyMeasurer::*)());
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(notifyRenderingStarted, void (KisStrokeEfficiencyMeasurer::*)());
}

void KisStrokeEfficiencyMeasurerSchemaContractTest::strokeEfficiencyMeasurerMetricQuerySignaturesRemainStable()
{
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(averageCursorSpeed, qreal (KisStrokeEfficiencyMeasurer::*)() const);
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(averageFps, qreal (KisStrokeEfficiencyMeasurer::*)() const);
    ASSERT_STROKE_EFFICIENCY_SIGNATURE(averageRenderingSpeed, qreal (KisStrokeEfficiencyMeasurer::*)() const);
}

#undef ASSERT_STROKE_EFFICIENCY_SIGNATURE

QTEST_GUILESS_MAIN(KisStrokeEfficiencyMeasurerSchemaContractTest)
#include "KisStrokeEfficiencyMeasurerSchemaContractTest.moc"
