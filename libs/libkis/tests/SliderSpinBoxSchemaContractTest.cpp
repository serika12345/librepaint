/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SliderSpinBox.h"

#include <QTest>

#include <type_traits>
#include <utility>

class SliderSpinBoxSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sliderSpinBoxTypeLifetimeAndViewSchemaRemainStable();
    void sliderSpinBoxHardRangeSignaturesRemainStable();
    void sliderSpinBoxSoftRangeAndStepSignaturesRemainStable();
    void sliderSpinBoxValueAndScalingSignaturesRemainStable();
    void sliderSpinBoxDraggingPolicySignaturesRemainStable();
    void parseSpinBoxTypeAndLifetimeSchemaRemainStable();
    void parseSpinBoxWidgetAndSteppingSignaturesRemainStable();
    void parseSpinBoxValueAndValidationSignaturesRemainStable();
    void parseSpinBoxExpressionTextSignaturesRemainStable();
    void parseSpinBoxParsingNotificationSignaturesRemainStable();
};

#define ASSERT_SLIDER_SPINBOX_SIGNATURE(type, method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

void SliderSpinBoxSchemaContractTest::parseSpinBoxTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<IntParseSpinBox>);
    static_assert(std::is_base_of_v<QObject, IntParseSpinBox>);
    static_assert(std::is_default_constructible_v<IntParseSpinBox>);
    static_assert(!std::is_copy_constructible_v<IntParseSpinBox>);
    static_assert(!std::is_copy_assignable_v<IntParseSpinBox>);
    static_assert(std::has_virtual_destructor_v<IntParseSpinBox>);

    static_assert(std::is_class_v<DoubleParseSpinBox>);
    static_assert(std::is_base_of_v<QObject, DoubleParseSpinBox>);
    static_assert(std::is_default_constructible_v<DoubleParseSpinBox>);
    static_assert(!std::is_copy_constructible_v<DoubleParseSpinBox>);
    static_assert(!std::is_copy_assignable_v<DoubleParseSpinBox>);
    static_assert(std::has_virtual_destructor_v<DoubleParseSpinBox>);
}

void SliderSpinBoxSchemaContractTest::parseSpinBoxWidgetAndSteppingSignaturesRemainStable()
{
    using IntegerWidgetGetter = QSpinBox *(IntParseSpinBox::*)() const;
    using IntegerStep = void (IntParseSpinBox::*)(int);
    using RealWidgetGetter = QDoubleSpinBox *(DoubleParseSpinBox::*)() const;
    using RealStep = void (DoubleParseSpinBox::*)(int);

    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, widget, IntegerWidgetGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, stepBy, IntegerStep);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, widget, RealWidgetGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, stepBy, RealStep);
}

void SliderSpinBoxSchemaContractTest::parseSpinBoxValueAndValidationSignaturesRemainStable()
{
    using IntegerValueSetter = void (IntParseSpinBox::*)(int, bool);
    using IntegerValidityGetter = bool (IntParseSpinBox::*)() const;
    using RealValueSetter = void (DoubleParseSpinBox::*)(double, bool);
    using RealValidityGetter = bool (DoubleParseSpinBox::*)() const;

    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, setValue, IntegerValueSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, isLastValid, IntegerValidityGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, setValue, RealValueSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, isLastValid, RealValidityGetter);

    static_assert(std::is_same_v<decltype(std::declval<IntParseSpinBox &>().setValue(0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<DoubleParseSpinBox &>().setValue(0.0)), void>);
}

void SliderSpinBoxSchemaContractTest::parseSpinBoxExpressionTextSignaturesRemainStable()
{
    using IntegerTextGetter = QString (IntParseSpinBox::*)() const;
    using RealTextGetter = QString (DoubleParseSpinBox::*)() const;

    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, veryCleanText, IntegerTextGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, veryCleanText, RealTextGetter);
}

void SliderSpinBoxSchemaContractTest::parseSpinBoxParsingNotificationSignaturesRemainStable()
{
    using IntegerParsingErrorSignal = void (IntParseSpinBox::*)(const QString &) const;
    using IntegerParsingRecoveredSignal = void (IntParseSpinBox::*)() const;
    using RealParsingErrorSignal = void (DoubleParseSpinBox::*)(const QString &) const;
    using RealParsingRecoveredSignal = void (DoubleParseSpinBox::*)() const;

    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, errorWhileParsing, IntegerParsingErrorSignal);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(IntParseSpinBox, noMoreParsingError, IntegerParsingRecoveredSignal);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, errorWhileParsing, RealParsingErrorSignal);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleParseSpinBox, noMoreParsingError, RealParsingRecoveredSignal);
}

void SliderSpinBoxSchemaContractTest::sliderSpinBoxTypeLifetimeAndViewSchemaRemainStable()
{
    using IntegerViewGetter = QWidget *(SliderSpinBox::*)() const;
    using RealViewGetter = QWidget *(DoubleSliderSpinBox::*)() const;

    static_assert(std::is_class_v<SliderSpinBox>);
    static_assert(std::is_base_of_v<IntParseSpinBox, SliderSpinBox>);
    static_assert(std::is_default_constructible_v<SliderSpinBox>);
    static_assert(!std::is_copy_constructible_v<SliderSpinBox>);
    static_assert(!std::is_copy_assignable_v<SliderSpinBox>);
    static_assert(std::has_virtual_destructor_v<SliderSpinBox>);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, widget, IntegerViewGetter);

    static_assert(std::is_class_v<DoubleSliderSpinBox>);
    static_assert(std::is_base_of_v<DoubleParseSpinBox, DoubleSliderSpinBox>);
    static_assert(std::is_default_constructible_v<DoubleSliderSpinBox>);
    static_assert(!std::is_copy_constructible_v<DoubleSliderSpinBox>);
    static_assert(!std::is_copy_assignable_v<DoubleSliderSpinBox>);
    static_assert(std::has_virtual_destructor_v<DoubleSliderSpinBox>);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, widget, RealViewGetter);
}

void SliderSpinBoxSchemaContractTest::sliderSpinBoxHardRangeSignaturesRemainStable()
{
    using IntegerRangeSetter = void (SliderSpinBox::*)(int, int, bool);
    using IntegerLimitSetter = void (SliderSpinBox::*)(int, bool);
    using RealRangeSetter = void (DoubleSliderSpinBox::*)(qreal, qreal, int, bool);
    using RealLimitSetter = void (DoubleSliderSpinBox::*)(qreal, bool);

    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setRange, IntegerRangeSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setMinimum, IntegerLimitSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setMaximum, IntegerLimitSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setRange, RealRangeSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setMinimum, RealLimitSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setMaximum, RealLimitSetter);

    static_assert(std::is_same_v<decltype(std::declval<SliderSpinBox &>().setRange(0, 1)), void>);
    static_assert(std::is_same_v<decltype(std::declval<SliderSpinBox &>().setMinimum(0)), void>);
    static_assert(std::is_same_v<decltype(std::declval<SliderSpinBox &>().setMaximum(1)), void>);
    static_assert(std::is_same_v<decltype(std::declval<DoubleSliderSpinBox &>().setRange(qreal{}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<DoubleSliderSpinBox &>().setRange(qreal{}, qreal{}, 2)), void>);
    static_assert(std::is_same_v<decltype(std::declval<DoubleSliderSpinBox &>().setMinimum(qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<DoubleSliderSpinBox &>().setMaximum(qreal{})), void>);
}

void SliderSpinBoxSchemaContractTest::sliderSpinBoxSoftRangeAndStepSignaturesRemainStable()
{
    using IntegerGetter = int (SliderSpinBox::*)() const;
    using IntegerSetter = void (SliderSpinBox::*)(int);
    using IntegerRangeSetter = void (SliderSpinBox::*)(int, int);
    using RealGetter = qreal (DoubleSliderSpinBox::*)() const;
    using RealSetter = void (DoubleSliderSpinBox::*)(qreal);
    using RealRangeSetter = void (DoubleSliderSpinBox::*)(qreal, qreal);

    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, fastSliderStep, IntegerGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, softMinimum, IntegerGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, softMaximum, IntegerGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setFastSliderStep, IntegerSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setSoftRange, IntegerRangeSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setSoftMinimum, IntegerSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setSoftMaximum, IntegerSetter);

    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, fastSliderStep, RealGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, softMinimum, RealGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, softMaximum, RealGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setFastSliderStep, RealSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setSoftRange, RealRangeSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setSoftMinimum, RealSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setSoftMaximum, RealSetter);
}

void SliderSpinBoxSchemaContractTest::sliderSpinBoxValueAndScalingSignaturesRemainStable()
{
    using IntegerValueSetter = void (SliderSpinBox::*)(int);
    using IntegerScalingSetter = void (SliderSpinBox::*)(qreal);
    using RealValueSetter = void (DoubleSliderSpinBox::*)(qreal);
    using RealScalingSetter = void (DoubleSliderSpinBox::*)(qreal);

    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setValue, IntegerValueSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setExponentRatio, IntegerScalingSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setValue, RealValueSetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setExponentRatio, RealScalingSetter);
}

void SliderSpinBoxSchemaContractTest::sliderSpinBoxDraggingPolicySignaturesRemainStable()
{
    using IntegerDraggingGetter = bool (SliderSpinBox::*)() const;
    using IntegerPolicySetter = void (SliderSpinBox::*)(bool);
    using IntegerFinishedSignal = void (SliderSpinBox::*)();
    using RealDraggingGetter = bool (DoubleSliderSpinBox::*)() const;
    using RealPolicySetter = void (DoubleSliderSpinBox::*)(bool);
    using RealFinishedSignal = void (DoubleSliderSpinBox::*)();

    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, isDragging, IntegerDraggingGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, setBlockUpdateSignalOnDrag, IntegerPolicySetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(SliderSpinBox, draggingFinished, IntegerFinishedSignal);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, isDragging, RealDraggingGetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, setBlockUpdateSignalOnDrag, RealPolicySetter);
    ASSERT_SLIDER_SPINBOX_SIGNATURE(DoubleSliderSpinBox, draggingFinished, RealFinishedSignal);
}

#undef ASSERT_SLIDER_SPINBOX_SIGNATURE

QTEST_APPLESS_MAIN(SliderSpinBoxSchemaContractTest)

#include "SliderSpinBoxSchemaContractTest.moc"
