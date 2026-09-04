/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoUnitDoubleSpinBox.h"
#include "kis_double_parse_unit_spin_box.h"
#include "kis_spin_box_unit_manager.h"

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
    void unitDoubleSpinBoxTypeLifetimeAndValueSchemaRemainStable();
    void unitDoubleSpinBoxRangeAndStepSignaturesRemainStable();
    void unitDoubleSpinBoxUnitTextAndNotificationSignaturesRemainStable();
    void unitManagerBuilderTypeLifetimeAndBuildSchemaRemainStable();
    void unitManagerFactoryTypeAndCreationSchemaRemainStable();
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

#define ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoUnitDoubleSpinBox::method)), signature>)

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitDoubleSpinBoxTypeLifetimeAndValueSchemaRemainStable()
{
    using SpinBox = KoUnitDoubleSpinBox;

    static_assert(std::is_class_v<SpinBox>);
    static_assert(std::is_base_of_v<QDoubleSpinBox, SpinBox>);
    static_assert(std::is_default_constructible_v<SpinBox>);
    static_assert(std::is_constructible_v<SpinBox, QWidget *>);
    static_assert(std::has_virtual_destructor_v<SpinBox>);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(changeValue, void (SpinBox::*)(double));
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(value, double (SpinBox::*)() const);
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitDoubleSpinBoxRangeAndStepSignaturesRemainStable()
{
    using SpinBox = KoUnitDoubleSpinBox;
    using ScalarSetter = void (SpinBox::*)(double);

    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(setMinimum, ScalarSetter);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(setMaximum, ScalarSetter);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(setLineStep, ScalarSetter);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(setLineStepPt, ScalarSetter);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(setMinMaxStep, void (SpinBox::*)(double, double, double));
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitDoubleSpinBoxUnitTextAndNotificationSignaturesRemainStable()
{
    using SpinBox = KoUnitDoubleSpinBox;

    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(setUnit, void (SpinBox::*)(const KoUnit &));
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(validate, QValidator::State (SpinBox::*)(QString &, int &) const);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(textFromValue, QString (SpinBox::*)(double) const);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(valueFromText, double (SpinBox::*)(const QString &) const);
    ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE(valueChangedPt, void (SpinBox::*)(qreal));
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitManagerBuilderTypeLifetimeAndBuildSchemaRemainStable()
{
    using Builder = KisSpinBoxUnitManagerBuilder;

    static_assert(std::is_class_v<Builder>);
    static_assert(std::has_virtual_destructor_v<Builder>);
    static_assert(std::is_same_v<decltype(static_cast<KisSpinBoxUnitManager *(Builder::*)(QObject *)>(
                                     &Builder::buildUnitManager)),
                                 KisSpinBoxUnitManager *(Builder::*)(QObject *)>);
}

void KisDoubleParseUnitSpinBoxSchemaContractTest::unitManagerFactoryTypeAndCreationSchemaRemainStable()
{
    using Builder = KisSpinBoxUnitManagerBuilder;
    using Factory = KisSpinBoxUnitManagerFactory;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::buildDefaultUnitManager), KisSpinBoxUnitManager *(*)(QObject *)>);
    static_assert(std::is_same_v<decltype(&Factory::setDefaultUnitManagerBuilder), void (*)(Builder *)>);
    static_assert(std::is_same_v<decltype(&Factory::clearUnitManagerBuilder), void (*)()>);
}

#undef ASSERT_UNIT_DOUBLE_SPIN_BOX_SIGNATURE

#undef ASSERT_UNIT_SPIN_BOX_SIGNATURE

QTEST_APPLESS_MAIN(KisDoubleParseUnitSpinBoxSchemaContractTest)

#include "KisDoubleParseUnitSpinBoxSchemaContractTest.moc"
