/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
};

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

QTEST_APPLESS_MAIN(KisSliderSpinBoxSchemaContractTest)

#include "KisSliderSpinBoxSchemaContractTest.moc"
