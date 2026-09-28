/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintingModeOptionData.h"

#include <kis_properties_configuration.h>

#include <QTest>

class KisPaintingModeOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultValuesAndEnumeratorsRemainStable();
    void readMapsConfigurationState();
    void writeMapsModesAndPreservesOtherProperties();
    void equalityDependsOnlyOnPaintingMode();
};

void KisPaintingModeOptionDataContractTest::defaultValuesAndEnumeratorsRemainStable()
{
    QCOMPARE(static_cast<int>(enumPaintingMode::BUILDUP), 0);
    QCOMPARE(static_cast<int>(enumPaintingMode::WASH), 1);

    const KisPaintingModeOptionData data;
    QCOMPARE(data.paintingMode, enumPaintingMode::BUILDUP);
    QVERIFY(!data.hasPaintingModeProperty);
}

void KisPaintingModeOptionDataContractTest::readMapsConfigurationState()
{
    KisPropertiesConfiguration missingSetting;
    KisPaintingModeOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QCOMPARE(missingData.paintingMode, enumPaintingMode::WASH);
    QVERIFY(!missingData.hasPaintingModeProperty);
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration buildUpSetting;
    buildUpSetting.setProperty(QStringLiteral("PaintOpAction"), 1);
    KisPaintingModeOptionData buildUpData;

    QVERIFY(buildUpData.read(&buildUpSetting));
    QCOMPARE(buildUpData.paintingMode, enumPaintingMode::BUILDUP);
    QVERIFY(buildUpData.hasPaintingModeProperty);

    for (const int persistedValue : {0, 2, 17, -1}) {
        KisPropertiesConfiguration washSetting;
        washSetting.setProperty(QStringLiteral("PaintOpAction"), persistedValue);
        KisPaintingModeOptionData washData;

        QVERIFY(washData.read(&washSetting));
        QCOMPARE(washData.paintingMode, enumPaintingMode::WASH);
        QVERIFY(washData.hasPaintingModeProperty);
        QCOMPARE(washSetting.getInt(QStringLiteral("PaintOpAction")), persistedValue);
    }
}

void KisPaintingModeOptionDataContractTest::writeMapsModesAndPreservesOtherProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), 73);

    KisPaintingModeOptionData data;
    data.hasPaintingModeProperty = true;
    data.paintingMode = enumPaintingMode::BUILDUP;
    data.write(&setting);

    QCOMPARE(setting.getInt(QStringLiteral("PaintOpAction")), 1);
    QCOMPARE(setting.getInt(QStringLiteral("unrelated/保持")), 73);
    QVERIFY(data.hasPaintingModeProperty);

    data.hasPaintingModeProperty = false;
    data.paintingMode = enumPaintingMode::WASH;
    data.write(&setting);

    QCOMPARE(setting.getInt(QStringLiteral("PaintOpAction")), 2);
    QCOMPARE(setting.getInt(QStringLiteral("unrelated/保持")), 73);
    QVERIFY(!data.hasPaintingModeProperty);
    QCOMPARE(setting.getProperties().size(), 2);
}

void KisPaintingModeOptionDataContractTest::equalityDependsOnlyOnPaintingMode()
{
    KisPaintingModeOptionData lhs;
    KisPaintingModeOptionData rhs;
    rhs.hasPaintingModeProperty = true;

    QVERIFY(lhs == rhs);
    QVERIFY(!(lhs != rhs));

    rhs.paintingMode = enumPaintingMode::WASH;
    QVERIFY(!(lhs == rhs));
    QVERIFY(lhs != rhs);

    lhs.paintingMode = enumPaintingMode::WASH;
    lhs.hasPaintingModeProperty = false;
    QVERIFY(lhs == rhs);
}

QTEST_GUILESS_MAIN(KisPaintingModeOptionDataContractTest)

#include "KisPaintingModeOptionDataContractTest.moc"
