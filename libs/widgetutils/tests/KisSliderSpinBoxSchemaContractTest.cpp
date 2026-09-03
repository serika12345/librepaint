/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_multipliers_double_slider_spinbox.h"
#include "kis_slider_spin_box.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisSliderSpinBoxSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sliderSpinValueAndRangeSignaturesRemainStable();
    void sliderSpinSoftRangeReadSignaturesRemainStable();
    void sliderSpinSoftRangeWriteSignaturesRemainStable();
    void sliderSpinStepSignaturesRemainStable();
    void sliderSpinExponentSignaturesRemainStable();
    void sliderLifetimeAndSizeSchemaRemainStable();
    void sliderDragSchemaRemainStable();
    void multipliersSliderTypeAndSizeSchemaRemainStable();
    void multipliersSliderRangeAndStepSchemaRemainStable();
    void multipliersSliderDisplayAndDragSchemaRemainStable();
};

#define ASSERT_SLIDER_SIGNATURE(type, method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

void KisSliderSpinBoxSchemaContractTest::sliderSpinValueAndRangeSignaturesRemainStable()
{
    using IntegerSpinBox = KisSliderSpinBox;
    using RealSpinBox = KisDoubleSliderSpinBox;
    using IntegerValueSetter = void (IntegerSpinBox::*)(int);
    using IntegerRangeSetter = void (IntegerSpinBox::*)(int, int, bool);
    using IntegerLimitSetter = void (IntegerSpinBox::*)(int, bool);
    using RealValueSetter = void (RealSpinBox::*)(qreal);
    using RealRangeSetter = void (RealSpinBox::*)(qreal, qreal, int, bool);
    using RealLimitSetter = void (RealSpinBox::*)(qreal, bool);

    static_assert(std::is_class_v<IntegerSpinBox>);
    static_assert(std::is_base_of_v<KisIntParseSpinBox, IntegerSpinBox>);
    static_assert(std::is_class_v<RealSpinBox>);
    static_assert(std::is_base_of_v<KisDoubleParseSpinBox, RealSpinBox>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerValueSetter>(&IntegerSpinBox::setValue)), IntegerValueSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerRangeSetter>(&IntegerSpinBox::setRange)), IntegerRangeSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerLimitSetter>(&IntegerSpinBox::setMinimum)), IntegerLimitSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerLimitSetter>(&IntegerSpinBox::setMaximum)), IntegerLimitSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealValueSetter>(&RealSpinBox::setValue)), RealValueSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealRangeSetter>(&RealSpinBox::setRange)), RealRangeSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealLimitSetter>(&RealSpinBox::setMinimum)), RealLimitSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealLimitSetter>(&RealSpinBox::setMaximum)), RealLimitSetter>);

    static_assert(std::is_same_v<decltype(std::declval<IntegerSpinBox &>().setRange(0, 1)), void>);
    static_assert(std::is_same_v<decltype(std::declval<IntegerSpinBox &>().setMinimum(0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<IntegerSpinBox &>().setMaximum(1)), void>);
    static_assert(std::is_same_v<decltype(std::declval<RealSpinBox &>().setRange(qreal{}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<RealSpinBox &>().setMinimum(qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<RealSpinBox &>().setMaximum(qreal{})), void>);
}

void KisSliderSpinBoxSchemaContractTest::sliderSpinSoftRangeReadSignaturesRemainStable()
{
    using IntegerSpinBox = KisSliderSpinBox;
    using RealSpinBox = KisDoubleSliderSpinBox;
    using IntegerGetter = int (IntegerSpinBox::*)() const;
    using RealGetter = qreal (RealSpinBox::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<IntegerGetter>(&IntegerSpinBox::softMinimum)), IntegerGetter>);
    static_assert(std::is_same_v<decltype(static_cast<IntegerGetter>(&IntegerSpinBox::softMaximum)), IntegerGetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealGetter>(&RealSpinBox::softMinimum)), RealGetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealGetter>(&RealSpinBox::softMaximum)), RealGetter>);
}

void KisSliderSpinBoxSchemaContractTest::sliderSpinSoftRangeWriteSignaturesRemainStable()
{
    using IntegerSpinBox = KisSliderSpinBox;
    using RealSpinBox = KisDoubleSliderSpinBox;
    using IntegerRangeSetter = void (IntegerSpinBox::*)(int, int);
    using IntegerLimitSetter = void (IntegerSpinBox::*)(int);
    using RealRangeSetter = void (RealSpinBox::*)(qreal, qreal);
    using RealLimitSetter = void (RealSpinBox::*)(qreal);

    static_assert(
        std::is_same_v<decltype(static_cast<IntegerRangeSetter>(&IntegerSpinBox::setSoftRange)), IntegerRangeSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerLimitSetter>(&IntegerSpinBox::setSoftMinimum)), IntegerLimitSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerLimitSetter>(&IntegerSpinBox::setSoftMaximum)), IntegerLimitSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealRangeSetter>(&RealSpinBox::setSoftRange)), RealRangeSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<RealLimitSetter>(&RealSpinBox::setSoftMinimum)), RealLimitSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<RealLimitSetter>(&RealSpinBox::setSoftMaximum)), RealLimitSetter>);
}

void KisSliderSpinBoxSchemaContractTest::sliderSpinStepSignaturesRemainStable()
{
    using IntegerSpinBox = KisSliderSpinBox;
    using RealSpinBox = KisDoubleSliderSpinBox;
    using IntegerGetter = int (IntegerSpinBox::*)() const;
    using IntegerSetter = void (IntegerSpinBox::*)(int);
    using RealGetter = qreal (RealSpinBox::*)() const;
    using RealSetter = void (RealSpinBox::*)(qreal);

    static_assert(std::is_same_v<decltype(static_cast<IntegerGetter>(&IntegerSpinBox::fastSliderStep)), IntegerGetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<IntegerSetter>(&IntegerSpinBox::setFastSliderStep)), IntegerSetter>);
    static_assert(std::is_same_v<decltype(static_cast<IntegerSetter>(&IntegerSpinBox::setPageStep)), IntegerSetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealGetter>(&RealSpinBox::fastSliderStep)), RealGetter>);
    static_assert(std::is_same_v<decltype(static_cast<RealSetter>(&RealSpinBox::setFastSliderStep)), RealSetter>);
}

void KisSliderSpinBoxSchemaContractTest::sliderSpinExponentSignaturesRemainStable()
{
    using IntegerSetter = void (KisSliderSpinBox::*)(qreal);
    using RealSetter = void (KisDoubleSliderSpinBox::*)(qreal);

    static_assert(
        std::is_same_v<decltype(static_cast<IntegerSetter>(&KisSliderSpinBox::setExponentRatio)), IntegerSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<RealSetter>(&KisDoubleSliderSpinBox::setExponentRatio)), RealSetter>);
}

void KisSliderSpinBoxSchemaContractTest::sliderLifetimeAndSizeSchemaRemainStable()
{
    using IntegerSpinBox = KisSliderSpinBox;
    using RealSpinBox = KisDoubleSliderSpinBox;
    using IntegerSizeGetter = QSize (IntegerSpinBox::*)() const;
    using RealSizeGetter = QSize (RealSpinBox::*)() const;

    static_assert(std::is_default_constructible_v<IntegerSpinBox>);
    static_assert(std::is_constructible_v<IntegerSpinBox, QWidget *>);
    static_assert(std::has_virtual_destructor_v<IntegerSpinBox>);
    ASSERT_SLIDER_SIGNATURE(IntegerSpinBox, sizeHint, IntegerSizeGetter);
    ASSERT_SLIDER_SIGNATURE(IntegerSpinBox, minimumSizeHint, IntegerSizeGetter);

    static_assert(std::is_default_constructible_v<RealSpinBox>);
    static_assert(std::is_constructible_v<RealSpinBox, QWidget *>);
    static_assert(std::has_virtual_destructor_v<RealSpinBox>);
    ASSERT_SLIDER_SIGNATURE(RealSpinBox, sizeHint, RealSizeGetter);
    ASSERT_SLIDER_SIGNATURE(RealSpinBox, minimumSizeHint, RealSizeGetter);
}

void KisSliderSpinBoxSchemaContractTest::sliderDragSchemaRemainStable()
{
    using IntegerSpinBox = KisSliderSpinBox;
    using RealSpinBox = KisDoubleSliderSpinBox;
    using IntegerDraggingGetter = bool (IntegerSpinBox::*)() const;
    using IntegerPolicySetter = void (IntegerSpinBox::*)(bool);
    using IntegerFinishedSignal = void (IntegerSpinBox::*)();
    using RealDraggingGetter = bool (RealSpinBox::*)() const;
    using RealPolicySetter = void (RealSpinBox::*)(bool);
    using RealFinishedSignal = void (RealSpinBox::*)();

    ASSERT_SLIDER_SIGNATURE(IntegerSpinBox, isDragging, IntegerDraggingGetter);
    ASSERT_SLIDER_SIGNATURE(IntegerSpinBox, setBlockUpdateSignalOnDrag, IntegerPolicySetter);
    ASSERT_SLIDER_SIGNATURE(IntegerSpinBox, draggingFinished, IntegerFinishedSignal);
    ASSERT_SLIDER_SIGNATURE(RealSpinBox, isDragging, RealDraggingGetter);
    ASSERT_SLIDER_SIGNATURE(RealSpinBox, setBlockUpdateSignalOnDrag, RealPolicySetter);
    ASSERT_SLIDER_SIGNATURE(RealSpinBox, draggingFinished, RealFinishedSignal);
}

void KisSliderSpinBoxSchemaContractTest::multipliersSliderTypeAndSizeSchemaRemainStable()
{
    using MultipliersSlider = KisMultipliersDoubleSliderSpinBox;
    using SizeGetter = QSize (MultipliersSlider::*)() const;

    static_assert(std::is_class_v<MultipliersSlider>);
    static_assert(std::is_base_of_v<QWidget, MultipliersSlider>);
    static_assert(std::is_default_constructible_v<MultipliersSlider>);
    static_assert(std::is_constructible_v<MultipliersSlider, QWidget *>);
    static_assert(std::has_virtual_destructor_v<MultipliersSlider>);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, sizeHint, SizeGetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, minimumSizeHint, SizeGetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, minimumSize, SizeGetter);
}

void KisSliderSpinBoxSchemaContractTest::multipliersSliderRangeAndStepSchemaRemainStable()
{
    using MultipliersSlider = KisMultipliersDoubleSliderSpinBox;
    using ValueGetter = qreal (MultipliersSlider::*)();
    using ValueSetter = void (MultipliersSlider::*)(qreal);
    using RangeSetter = void (MultipliersSlider::*)(qreal, qreal, int);
    using StepSetter = void (MultipliersSlider::*)(qreal);
    using MultiplierAdder = void (MultipliersSlider::*)(double);

    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, value, ValueGetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setValue, ValueSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setRange, RangeSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setSingleStep, StepSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, addMultiplier, MultiplierAdder);
    static_assert(std::is_same_v<decltype(std::declval<MultipliersSlider &>().setRange(qreal{}, qreal{})), void>);
}

void KisSliderSpinBoxSchemaContractTest::multipliersSliderDisplayAndDragSchemaRemainStable()
{
    using MultipliersSlider = KisMultipliersDoubleSliderSpinBox;
    using RealSetter = void (MultipliersSlider::*)(qreal);
    using TextSetter = void (MultipliersSlider::*)(const QString &);
    using BooleanSetter = void (MultipliersSlider::*)(bool);
    using ValueSignal = void (MultipliersSlider::*)(qreal);

    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setExponentRatio, RealSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setPrefix, TextSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setSuffix, TextSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, setBlockUpdateSignalOnDrag, BooleanSetter);
    ASSERT_SLIDER_SIGNATURE(MultipliersSlider, valueChanged, ValueSignal);
}

#undef ASSERT_SLIDER_SIGNATURE

QTEST_APPLESS_MAIN(KisSliderSpinBoxSchemaContractTest)

#include "KisSliderSpinBoxSchemaContractTest.moc"
