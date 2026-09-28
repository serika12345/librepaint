/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_precision_option.h"

#include <kis_properties_configuration.h>

#include <QTest>

class KisPrecisionOptionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void persistenceKeysAndDefaultDataRemainStable();
    void precisionDataReadsWritesAndComparesIndependently();
    void optionConstructionAndMutatorsPreserveIndependentState();
    void effectivePrecisionUsesAutoModeAndExactThreshold();
};

void KisPrecisionOptionContractTest::persistenceKeysAndDefaultDataRemainStable()
{
    QCOMPARE(PRECISION_LEVEL, QStringLiteral("KisPrecisionOption/precisionLevel"));
    QCOMPARE(AUTO_PRECISION_ENABLED, QStringLiteral("KisPrecisionOption/AutoPrecisionEnabled"));
    QCOMPARE(STARTING_SIZE, QStringLiteral("KisPrecisionOption/SizeToStartFrom"));
    QCOMPARE(DELTA_VALUE, QStringLiteral("KisPrecisionOption/DeltaValue"));

    const KisBrushModel::PrecisionData data;
    QCOMPARE(data.precisionLevel, 5);
    QVERIFY(!data.useAutoPrecision);
}

void KisPrecisionOptionContractTest::precisionDataReadsWritesAndComparesIndependently()
{
    KisPropertiesConfiguration missingSetting;
    const KisBrushModel::PrecisionData missingData = KisBrushModel::PrecisionData::read(&missingSetting);

    QCOMPARE(missingData.precisionLevel, 5);
    QVERIFY(!missingData.useAutoPrecision);
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration populatedSetting;
    populatedSetting.setProperty(PRECISION_LEVEL, -7);
    populatedSetting.setProperty(AUTO_PRECISION_ENABLED, true);
    populatedSetting.setProperty(QString::fromUtf8("保持/備考"), QString::fromUtf8("精度・設定"));

    const KisBrushModel::PrecisionData populatedData = KisBrushModel::PrecisionData::read(&populatedSetting);
    QCOMPARE(populatedData.precisionLevel, -7);
    QVERIFY(populatedData.useAutoPrecision);
    QCOMPARE(populatedSetting.getProperties().size(), 3);

    KisPropertiesConfiguration writtenSetting;
    writtenSetting.setProperty(QString::fromUtf8("保持/備考"), QString::fromUtf8("既存値"));
    populatedData.write(&writtenSetting);

    QCOMPARE(writtenSetting.getInt(PRECISION_LEVEL), -7);
    QVERIFY(writtenSetting.getBool(AUTO_PRECISION_ENABLED));
    QCOMPARE(writtenSetting.getProperty(QString::fromUtf8("保持/備考")).toString(), QString::fromUtf8("既存値"));
    QCOMPARE(writtenSetting.getProperties().size(), 3);

    KisBrushModel::PrecisionData peer = populatedData;
    QVERIFY(populatedData == peer);

    peer.precisionLevel = 11;
    QVERIFY(!(populatedData == peer));

    peer = populatedData;
    peer.useAutoPrecision = false;
    QVERIFY(!(populatedData == peer));
}

void KisPrecisionOptionContractTest::optionConstructionAndMutatorsPreserveIndependentState()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(PRECISION_LEVEL, 9);
    setting.setProperty(AUTO_PRECISION_ENABLED, true);

    KisPrecisionOption option(&setting);
    QCOMPARE(option.precisionLevel(), 9);
    QVERIFY(option.autoPrecisionEnabled());
    QVERIFY(!option.hasImprecisePositionOptions());

    option.setPrecisionLevel(-4);
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(option.autoPrecisionEnabled());
    QVERIFY(!option.hasImprecisePositionOptions());

    option.setAutoPrecisionEnabled(0);
    QVERIFY(!option.autoPrecisionEnabled());
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(!option.hasImprecisePositionOptions());

    option.setAutoPrecisionEnabled(-7);
    QVERIFY(option.autoPrecisionEnabled());
    QCOMPARE(option.precisionLevel(), -4);

    option.setHasImprecisePositionOptions(true);
    QVERIFY(option.hasImprecisePositionOptions());
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(option.autoPrecisionEnabled());

    option.setHasImprecisePositionOptions(false);
    QVERIFY(!option.hasImprecisePositionOptions());
    QCOMPARE(option.precisionLevel(), -4);
    QVERIFY(option.autoPrecisionEnabled());
}

void KisPrecisionOptionContractTest::effectivePrecisionUsesAutoModeAndExactThreshold()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(PRECISION_LEVEL, 8);
    setting.setProperty(AUTO_PRECISION_ENABLED, false);

    KisPrecisionOption option(&setting);
    QCOMPARE(option.effectivePrecisionLevel(0.0), 8);
    QCOMPARE(option.effectivePrecisionLevel(29.999), 8);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 8);
    QCOMPARE(option.effectivePrecisionLevel(300.0), 8);

    option.setAutoPrecisionEnabled(1);
    QCOMPARE(option.effectivePrecisionLevel(29.999), 5);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 5);
    QCOMPARE(option.effectivePrecisionLevel(30.001), 5);

    option.setHasImprecisePositionOptions(true);
    QCOMPARE(option.effectivePrecisionLevel(29.999), 5);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 3);
    QCOMPARE(option.effectivePrecisionLevel(30.001), 3);

    option.setAutoPrecisionEnabled(0);
    QCOMPARE(option.effectivePrecisionLevel(30.0), 8);
}

QTEST_GUILESS_MAIN(KisPrecisionOptionContractTest)

#include "KisPrecisionOptionContractTest.moc"
