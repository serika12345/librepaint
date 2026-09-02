/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWidgetConnectionUtils.h"

#include <QTest>

#include <tuple>
#include <type_traits>

#define ASSERT_CONNECTION_SIGNATURE(function, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisWidgetConnectionUtils::function)), signature>)

class KisWidgetConnectionStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void controlStateDefaultsAndAliasesRemainTyped();
    void controlStateConversionPreservesValueAndEnabled();
    void comboBoxStateDefaultsAndAssignedListsRemainStable();
    void spacingStateDefaultsAndConversionRoundTripRemainStable();
    void spinBoxStatesDefaultsAliasesAndConversionRemainTyped();
    void primitiveControlConnectionSignaturesRemainStable();
    void choiceActionAndTextControlConnectionSignaturesRemainStable();
    void specializedControlConnectionSignaturesRemainStable();
    void controlStateConnectionSignaturesRemainStable();
    void widgetPresentationPropertyConnectionSignaturesRemainStable();
};

void KisWidgetConnectionStateContractTest::controlStateDefaultsAndAliasesRemainTyped()
{
    using namespace KisWidgetConnectionUtils;

    static_assert(std::is_same_v<CheckBoxState, ControlState<bool>>);
    static_assert(std::is_same_v<ButtonGroupState, ControlState<int>>);

    const CheckBoxState checkBoxDefaults;
    QCOMPARE(checkBoxDefaults.value, false);
    QCOMPARE(checkBoxDefaults.enabled, true);

    const ButtonGroupState buttonGroupDefaults;
    QCOMPARE(buttonGroupDefaults.value, 0);
    QCOMPARE(buttonGroupDefaults.enabled, true);
}

void KisWidgetConnectionStateContractTest::controlStateConversionPreservesValueAndEnabled()
{
    using namespace KisWidgetConnectionUtils;

    int source = -17;
    const auto fromLvalue = ToControlState{}(source, false);
    static_assert(std::is_same_v<std::decay_t<decltype(fromLvalue)>, ControlState<int>>);
    QCOMPARE(fromLvalue.value, -17);
    QCOMPARE(fromLvalue.enabled, false);

    source = 42;
    QCOMPARE(fromLvalue.value, -17);

    const auto fromRvalue = ToControlState{}(QStringLiteral("状態🌐"), true);
    static_assert(std::is_same_v<std::decay_t<decltype(fromRvalue)>, ControlState<QString>>);
    QCOMPARE(fromRvalue.value, QStringLiteral("状態🌐"));
    QCOMPARE(fromRvalue.enabled, true);
}

void KisWidgetConnectionStateContractTest::comboBoxStateDefaultsAndAssignedListsRemainStable()
{
    using namespace KisWidgetConnectionUtils;

    const ComboBoxState defaults;
    QVERIFY(defaults.items.isEmpty());
    QCOMPARE(defaults.currentIndex, -1);
    QCOMPARE(defaults.enabled, true);
    QVERIFY(defaults.toolTips.isEmpty());

    ComboBoxState configured;
    configured.items = {QStringLiteral("first"), QStringLiteral("二番目"), QStringLiteral("first")};
    configured.currentIndex = 1;
    configured.enabled = false;
    configured.toolTips = {QStringLiteral("tip 1"), QStringLiteral("説明"), QString()};

    QCOMPARE(configured.items,
             QStringList({QStringLiteral("first"), QStringLiteral("二番目"), QStringLiteral("first")}));
    QCOMPARE(configured.currentIndex, 1);
    QCOMPARE(configured.enabled, false);
    QCOMPARE(configured.toolTips,
             QStringList({QStringLiteral("tip 1"), QStringLiteral("説明"), QString()}));
}

void KisWidgetConnectionStateContractTest::spacingStateDefaultsAndConversionRoundTripRemainStable()
{
    using namespace KisWidgetConnectionUtils;

    const SpacingState defaults;
    QCOMPARE(defaults.spacing, qreal(0.05));
    QCOMPARE(defaults.useAutoSpacing, false);
    QCOMPARE(defaults.autoSpacingCoeff, qreal(1.0));

    const SpacingState converted = ToSpacingState{}(-2.5, true, 0.125);
    QCOMPARE(converted.spacing, qreal(-2.5));
    QCOMPARE(converted.useAutoSpacing, true);
    QCOMPARE(converted.autoSpacingCoeff, qreal(0.125));

    const auto values = FromSpacingState{}(converted);
    static_assert(std::is_same_v<std::decay_t<decltype(values)>, std::tuple<qreal, bool, qreal>>);
    QCOMPARE(std::get<0>(values), qreal(-2.5));
    QCOMPARE(std::get<1>(values), true);
    QCOMPARE(std::get<2>(values), qreal(0.125));
}

void KisWidgetConnectionStateContractTest::spinBoxStatesDefaultsAliasesAndConversionRemainTyped()
{
    using namespace KisWidgetConnectionUtils;

    static_assert(std::is_same_v<IntSpinBoxState, SpinBoxState<int>>);
    static_assert(std::is_same_v<DoubleSpinBoxState, SpinBoxState<qreal>>);

    const IntSpinBoxState intDefaults;
    QCOMPARE(intDefaults.value, 0);
    QCOMPARE(intDefaults.min, 0);
    QCOMPARE(intDefaults.max, 0);
    QCOMPARE(intDefaults.enabled, true);

    const DoubleSpinBoxState realDefaults;
    QCOMPARE(realDefaults.value, qreal(0));
    QCOMPARE(realDefaults.min, qreal(0));
    QCOMPARE(realDefaults.max, qreal(0));
    QCOMPARE(realDefaults.enabled, true);

    const auto intState = ToSpinBoxState{}(7, -4, 19, false);
    static_assert(std::is_same_v<std::decay_t<decltype(intState)>, SpinBoxState<int>>);
    QCOMPARE(intState.value, 7);
    QCOMPARE(intState.min, -4);
    QCOMPARE(intState.max, 19);
    QCOMPARE(intState.enabled, false);

    qreal value = 2.75;
    qreal minimum = -1.5;
    qreal maximum = 8.25;
    const auto realState = ToSpinBoxState{}(value, minimum, maximum, true);
    static_assert(std::is_same_v<std::decay_t<decltype(realState)>, SpinBoxState<qreal>>);
    QCOMPARE(realState.value, qreal(2.75));
    QCOMPARE(realState.min, qreal(-1.5));
    QCOMPARE(realState.max, qreal(8.25));
    QCOMPARE(realState.enabled, true);

    value = 99.0;
    QCOMPARE(realState.value, qreal(2.75));
}

void KisWidgetConnectionStateContractTest::primitiveControlConnectionSignaturesRemainStable()
{
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QAbstractButton *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QCheckBox *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QSpinBox *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QDoubleSpinBox *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QSlider *, QObject *, const char *));
}

void KisWidgetConnectionStateContractTest::choiceActionAndTextControlConnectionSignaturesRemainStable()
{
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QAction *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QButtonGroup *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QComboBox *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(QLineEdit *, QObject *, const char *));
}

void KisWidgetConnectionStateContractTest::specializedControlConnectionSignaturesRemainStable()
{
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(KisAngleSelector *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(KisColorButton *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(KisFileNameRequester *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(KisMultipliersDoubleSliderSpinBox *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControl, void (*)(KisSpacingSelectionWidget *, QObject *, const char *));
}

void KisWidgetConnectionStateContractTest::controlStateConnectionSignaturesRemainStable()
{
    ASSERT_CONNECTION_SIGNATURE(connectControlState,
                                void (*)(QAbstractButton *, QObject *, const char *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControlState, void (*)(QButtonGroup *, QObject *, const char *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControlState, void (*)(QComboBox *, QObject *, const char *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControlState, void (*)(QDoubleSpinBox *, QObject *, const char *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectControlState, void (*)(QSpinBox *, QObject *, const char *, const char *));
}

void KisWidgetConnectionStateContractTest::widgetPresentationPropertyConnectionSignaturesRemainStable()
{
    ASSERT_CONNECTION_SIGNATURE(connectWidgetEnabledToProperty, void (*)(QWidget *, QObject *, const char *));
    ASSERT_CONNECTION_SIGNATURE(connectWidgetVisibleToProperty, void (*)(QWidget *, QObject *, const char *));
}

QTEST_GUILESS_MAIN(KisWidgetConnectionStateContractTest)

#include "KisWidgetConnectionStateContractTest.moc"
