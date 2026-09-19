/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCurveOption.h"
#include "KisCurveOptionWidget.h"
#include "KisCurveRangeModel.h"
#include "KisStandardOptionData.h"
#include "KisStandardOptions.h"

#include <QTest>

#include <utility>

namespace
{


template<template<typename> class Option>
struct StandardOptionTemplateProbe {
};

} // namespace

class KisCurveOptionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void curveOptionValueComponentStorageRemainsStable();
    void curveOptionWidgetTypeAndFlagSchemaRemainStable();
};

void KisCurveOptionSchemaContractTest::curveOptionValueComponentStorageRemainsStable()
{
    using Components = KisCurveOption::ValueComponents;


    const Components defaults;
    QCOMPARE(defaults.constant, 1.0);
    QCOMPARE(defaults.scaling, 1.0);
    QCOMPARE(defaults.additive, 0.0);
    QCOMPARE(defaults.absoluteOffset, 0.0);
    QVERIFY(!defaults.hasAbsoluteOffset);
    QVERIFY(!defaults.hasScaling);
    QVERIFY(!defaults.hasAdditive);
    QCOMPARE(defaults.minSizeLikeValue, 0.0);
    QCOMPARE(defaults.maxSizeLikeValue, 0.0);
}

void KisCurveOptionSchemaContractTest::curveOptionWidgetTypeAndFlagSchemaRemainStable()
{
    using Widget = KisCurveOptionWidget;

    QCOMPARE(int(Widget::None), 0x0);
    QCOMPARE(int(Widget::SupportsCommonCurve), 0x1);
    QCOMPARE(int(Widget::SupportsCurveMode), 0x2);
    QCOMPARE(int(Widget::UseFloatingPointStrength), 0x4);
}

QTEST_GUILESS_MAIN(KisCurveOptionSchemaContractTest)

#include "KisCurveOptionSchemaContractTest.moc"
