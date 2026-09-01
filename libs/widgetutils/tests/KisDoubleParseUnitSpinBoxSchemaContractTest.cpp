/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_double_parse_unit_spin_box.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisDoubleParseUnitSpinBoxSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unitSpinBoxTypeLifetimeAndManagerPolicySignaturesRemainStable();
    void unitSpinBoxUnitSelectionSignaturesRemainStable();
    void unitSpinBoxValueConversionSignaturesRemainStable();
    void unitSpinBoxRangeStepAndPrecisionSignaturesRemainStable();
    void unitSpinBoxTextConversionSignaturesRemainStable();
};

#define ASSERT_UNIT_SPIN_BOX_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDoubleParseUnitSpinBox::method)), signature>)

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitSpinBoxTypeLifetimeAndManagerPolicySignaturesRemainStable()
{
    using ManagerSetter = void (KisDoubleParseUnitSpinBox::*)(KisSpinBoxUnitManager *);
    using IntegerSetter = void (KisDoubleParseUnitSpinBox::*)(int);
    using BooleanSetter = void (KisDoubleParseUnitSpinBox::*)(bool);

    static_assert(std::is_class_v<KisDoubleParseUnitSpinBox>);
    static_assert(std::is_base_of_v<KisDoubleParseSpinBox, KisDoubleParseUnitSpinBox>);
    static_assert(std::is_default_constructible_v<KisDoubleParseUnitSpinBox>);
    static_assert(std::has_virtual_destructor_v<KisDoubleParseUnitSpinBox>);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setUnitManager, ManagerSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setDimensionType, IntegerSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setUnitChangeFromOutsideBehavior, BooleanSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setDisplayUnit, BooleanSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(preventDecimalsChangeFromUnitManager, BooleanSetter);
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitSpinBoxUnitSelectionSignaturesRemainStable()
{
    using UnitSetter = void (KisDoubleParseUnitSpinBox::*)(const KoUnit &);
    using SymbolSetter = void (KisDoubleParseUnitSpinBox::*)(const QString &);
    using SymbolGetter = QString (KisDoubleParseUnitSpinBox::*)() const;

    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setUnit, UnitSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setUnit, SymbolSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setReturnUnit, SymbolSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(returnUnit, SymbolGetter);
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitSpinBoxValueConversionSignaturesRemainStable()
{
    using ValueSetter = void (KisDoubleParseUnitSpinBox::*)(double);
    using ValueGetter = double (KisDoubleParseUnitSpinBox::*)() const;
    using PointValueSetter = void (KisDoubleParseUnitSpinBox::*)(double, bool);
    using PointValueSignal = void (KisDoubleParseUnitSpinBox::*)(qreal);

    ASSERT_UNIT_SPIN_BOX_SIGNATURE(changeValue, ValueSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(changeValuePt, ValueSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(value, ValueGetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(valuePt, ValueGetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setValuePt, PointValueSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(valueChangedPt, PointValueSignal);
    static_assert(std::is_same_v<decltype(std::declval<KisDoubleParseUnitSpinBox &>().setValuePt(0.0)), void>);
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitSpinBoxRangeStepAndPrecisionSignaturesRemainStable()
{
    using ScalarSetter = void (KisDoubleParseUnitSpinBox::*)(double);
    using RangeSetter = void (KisDoubleParseUnitSpinBox::*)(double, double, double);
    using PrecisionSetter = void (KisDoubleParseUnitSpinBox::*)(int);

    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setMinimum, ScalarSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setMinimumPt, ScalarSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setMaximum, ScalarSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setMaximumPt, ScalarSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setLineStep, ScalarSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setLineStepPt, ScalarSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setMinMaxStep, RangeSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setMinMaxStepPt, RangeSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setDecimals, PrecisionSetter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(setSingleStep, ScalarSetter);
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitSpinBoxTextConversionSignaturesRemainStable()
{
    using TextFormatter = QString (KisDoubleParseUnitSpinBox::*)(double) const;
    using TextParser = double (KisDoubleParseUnitSpinBox::*)(const QString &) const;
    using CleanTextGetter = QString (KisDoubleParseUnitSpinBox::*)() const;

    ASSERT_UNIT_SPIN_BOX_SIGNATURE(textFromValue, TextFormatter);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(valueFromText, TextParser);
    ASSERT_UNIT_SPIN_BOX_SIGNATURE(veryCleanText, CleanTextGetter);
}

#undef ASSERT_UNIT_SPIN_BOX_SIGNATURE

QTEST_APPLESS_MAIN(KisDoubleParseUnitSpinBoxSchemaContractTest)

#include "KisDoubleParseUnitSpinBoxSchemaContractTest.moc"
