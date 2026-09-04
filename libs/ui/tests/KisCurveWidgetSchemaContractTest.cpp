/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_curve_widget.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_CURVE_WIDGET_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCurveWidget::method)), signature>)
} // namespace

class KisCurveWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void curveWidgetTypeLifetimeAndConstraintSchemaRemainStable();
    void curveWidgetCurveAndBackgroundSignaturesRemainStable();
    void curveWidgetPointSelectionAndEditingSignaturesRemainStable();
    void curveWidgetInputOutputControlSignaturesRemainStable();
    void curveWidgetNotificationSignaturesRemainStable();
};

void KisCurveWidgetSchemaContractTest::curveWidgetTypeLifetimeAndConstraintSchemaRemainStable()
{
    static_assert(std::is_class_v<KisCurveWidget>);
    static_assert(std::is_base_of_v<QWidget, KisCurveWidget>);
    static_assert(std::is_constructible_v<KisCurveWidget>);
    static_assert(std::is_constructible_v<KisCurveWidget, QWidget *>);
    static_assert(std::is_constructible_v<KisCurveWidget, QWidget *, Qt::WindowFlags>);
    static_assert(std::has_virtual_destructor_v<KisCurveWidget>);
    static_assert(std::is_enum_v<KisCurveWidget::PointConstrain>);
    static_assert(static_cast<int>(KisCurveWidget::PointConstrain_None) == 0);
    static_assert(static_cast<int>(KisCurveWidget::PointConstrain_AlwaysCorner) == 1);
    static_assert(static_cast<int>(KisCurveWidget::PointConstrain_AlwaysSmooth) == 2);
    ASSERT_CURVE_WIDGET_SIGNATURE(globalPointConstrain, KisCurveWidget::PointConstrain (KisCurveWidget::*)() const);
    ASSERT_CURVE_WIDGET_SIGNATURE(setGlobalPointConstrain, void (KisCurveWidget::*)(KisCurveWidget::PointConstrain));
    static_assert(std::is_same_v<decltype(KisCurveWidget()), KisCurveWidget>);
    static_assert(std::is_same_v<decltype(KisCurveWidget(std::declval<QWidget *>())), KisCurveWidget>);
}

void KisCurveWidgetSchemaContractTest::curveWidgetCurveAndBackgroundSignaturesRemainStable()
{
    ASSERT_CURVE_WIDGET_SIGNATURE(curve, KisCubicCurve (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(getPixmap, QPixmap (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(reset, void (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(setCurve, void (KisCurveWidget::*)(KisCubicCurve));
    ASSERT_CURVE_WIDGET_SIGNATURE(setPixmap, void (KisCurveWidget::*)(const QPixmap &));
}

void KisCurveWidgetSchemaContractTest::curveWidgetPointSelectionAndEditingSignaturesRemainStable()
{
    ASSERT_CURVE_WIDGET_SIGNATURE(addPointInTheMiddle, void (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(currentPoint, std::optional<KisCubicCurvePoint> (KisCurveWidget::*)() const);
    ASSERT_CURVE_WIDGET_SIGNATURE(currentPointPosition, std::optional<QPointF> (KisCurveWidget::*)() const);
    ASSERT_CURVE_WIDGET_SIGNATURE(isCurrentPointSetAsCorner, std::optional<bool> (KisCurveWidget::*)() const);
    ASSERT_CURVE_WIDGET_SIGNATURE(pointSelected, bool (KisCurveWidget::*)() const);
    ASSERT_CURVE_WIDGET_SIGNATURE(setCurrentPoint, bool (KisCurveWidget::*)(const QPointF &, bool));
    ASSERT_CURVE_WIDGET_SIGNATURE(setCurrentPointAsCorner, bool (KisCurveWidget::*)(bool));
    ASSERT_CURVE_WIDGET_SIGNATURE(setCurrentPointPosition, bool (KisCurveWidget::*)(const QPointF &));
}

void KisCurveWidgetSchemaContractTest::curveWidgetInputOutputControlSignaturesRemainStable()
{
    ASSERT_CURVE_WIDGET_SIGNATURE(dropInOutControls, void (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(setupInOutControls,
                                  void (KisCurveWidget::*)(QSpinBox *, QSpinBox *, int, int, int, int));
    ASSERT_CURVE_WIDGET_SIGNATURE(shouldFocusIOControls, void (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(shouldSyncIOControls, void (KisCurveWidget::*)());
}

void KisCurveWidgetSchemaContractTest::curveWidgetNotificationSignaturesRemainStable()
{
    ASSERT_CURVE_WIDGET_SIGNATURE(compressorShouldEmitModified, void (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(curveChanged, void (KisCurveWidget::*)(const KisCubicCurve &));
    ASSERT_CURVE_WIDGET_SIGNATURE(modified, void (KisCurveWidget::*)());
    ASSERT_CURVE_WIDGET_SIGNATURE(pointSelectedChanged, void (KisCurveWidget::*)());
}

QTEST_APPLESS_MAIN(KisCurveWidgetSchemaContractTest)

#include "KisCurveWidgetSchemaContractTest.moc"
