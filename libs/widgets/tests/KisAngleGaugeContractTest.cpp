/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAngleGauge.h"

#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

class KisAngleGaugeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateEnumsAndParentLifetime()
    {
        QCOMPARE(static_cast<int>(KisAngleGauge::IncreasingDirection_CounterClockwise), 0);
        QCOMPARE(static_cast<int>(KisAngleGauge::IncreasingDirection_Clockwise), 1);

        auto *parent = new QWidget;
        QPointer<KisAngleGauge> gauge = new KisAngleGauge(parent);

        QCOMPARE(gauge->parentWidget(), parent);
        QCOMPARE(gauge->angle(), 0.0);
        QCOMPARE(gauge->snapAngle(), 15.0);
        QCOMPARE(gauge->resetAngle(), 0.0);
        QCOMPARE(gauge->increasingDirection(), KisAngleGauge::IncreasingDirection_CounterClockwise);
        QCOMPARE(gauge->focusPolicy(), Qt::WheelFocus);

        delete parent;
        QVERIFY(gauge.isNull());
    }

    void settersPreserveFiniteConfiguration()
    {
        KisAngleGauge gauge;

        gauge.setSnapAngle(22.5);
        gauge.setResetAngle(-45.0);
        gauge.setIncreasingDirection(KisAngleGauge::IncreasingDirection_Clockwise);

        QCOMPARE(gauge.snapAngle(), 22.5);
        QCOMPARE(gauge.resetAngle(), -45.0);
        QCOMPARE(gauge.increasingDirection(), KisAngleGauge::IncreasingDirection_Clockwise);
    }

    void setAngleEmitsOnlyForMeaningfulChanges()
    {
        KisAngleGauge gauge;
        QSignalSpy angleChangedSpy(&gauge, &KisAngleGauge::angleChanged);
        QVERIFY(angleChangedSpy.isValid());

        gauge.setAngle(37.5);
        QCOMPARE(gauge.angle(), 37.5);
        QCOMPARE(angleChangedSpy.count(), 1);
        QCOMPARE(angleChangedSpy.at(0).at(0).toDouble(), 37.5);

        gauge.setAngle(37.5);
        QCOMPARE(angleChangedSpy.count(), 1);

        gauge.setAngle(-12.25);
        QCOMPARE(gauge.angle(), -12.25);
        QCOMPARE(angleChangedSpy.count(), 2);
        QCOMPARE(angleChangedSpy.at(1).at(0).toDouble(), -12.25);
    }

    void resetUsesConfiguredAngle()
    {
        KisAngleGauge gauge;
        gauge.setResetAngle(90.0);
        gauge.setAngle(12.0);

        QSignalSpy angleChangedSpy(&gauge, &KisAngleGauge::angleChanged);
        QVERIFY(angleChangedSpy.isValid());

        gauge.reset();
        QCOMPARE(gauge.angle(), 90.0);
        QCOMPARE(angleChangedSpy.count(), 1);
        QCOMPARE(angleChangedSpy.at(0).at(0).toDouble(), 90.0);

        gauge.reset();
        QCOMPARE(angleChangedSpy.count(), 1);
    }
};

QTEST_MAIN(KisAngleGaugeContractTest)

#include "KisAngleGaugeContractTest.moc"
