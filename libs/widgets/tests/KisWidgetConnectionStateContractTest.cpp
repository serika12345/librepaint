/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWidgetConnectionUtils.h"

#include <QTest>

#include <tuple>

class KisWidgetConnectionStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void controlStateDefaults();
    void controlStateConversionPreservesValueAndEnabled();
    void comboBoxStateDefaultsAndAssignedListsRemainStable();
    void spacingStateDefaultsAndConversionRoundTripRemainStable();
    void spinBoxStatesDefaultsAndConversion();
};

void KisWidgetConnectionStateContractTest::controlStateDefaults()
{
    using namespace KisWidgetConnectionUtils;

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
    QCOMPARE(fromLvalue.value, -17);
    QCOMPARE(fromLvalue.enabled, false);

    source = 42;
    QCOMPARE(fromLvalue.value, -17);

    const auto fromRvalue = ToControlState{}(QStringLiteral("状態🌐"), true);
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
    QCOMPARE(configured.toolTips, QStringList({QStringLiteral("tip 1"), QStringLiteral("説明"), QString()}));
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
    QCOMPARE(std::get<0>(values), qreal(-2.5));
    QCOMPARE(std::get<1>(values), true);
    QCOMPARE(std::get<2>(values), qreal(0.125));
}

void KisWidgetConnectionStateContractTest::spinBoxStatesDefaultsAndConversion()
{
    using namespace KisWidgetConnectionUtils;

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
    QCOMPARE(intState.value, 7);
    QCOMPARE(intState.min, -4);
    QCOMPARE(intState.max, 19);
    QCOMPARE(intState.enabled, false);

    qreal value = 2.75;
    qreal minimum = -1.5;
    qreal maximum = 8.25;
    const auto realState = ToSpinBoxState{}(value, minimum, maximum, true);
    QCOMPARE(realState.value, qreal(2.75));
    QCOMPARE(realState.min, qreal(-1.5));
    QCOMPARE(realState.max, qreal(8.25));
    QCOMPARE(realState.enabled, true);

    value = 99.0;
    QCOMPARE(realState.value, qreal(2.75));
}

QTEST_GUILESS_MAIN(KisWidgetConnectionStateContractTest)

#include "KisWidgetConnectionStateContractTest.moc"
