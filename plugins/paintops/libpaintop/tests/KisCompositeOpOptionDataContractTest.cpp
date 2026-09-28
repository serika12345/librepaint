/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCompositeOpOptionData.h"

#include <kis_properties_configuration.h>

#include <QTest>

class KisCompositeOpOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultValues();
    void readMapsMissingAndUnicodeConfiguration();
    void writeMapsBothMembersAndPreservesOtherProperties();
    void equalityDependsOnBothMembersIndependently();
};

void KisCompositeOpOptionDataContractTest::defaultValues()
{
    const KisCompositeOpOptionData data;
    QCOMPARE(data.compositeOpId, QStringLiteral("normal"));
    QVERIFY(!data.eraserMode);
}

void KisCompositeOpOptionDataContractTest::readMapsMissingAndUnicodeConfiguration()
{
    KisPropertiesConfiguration missingSetting;
    missingSetting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("既存値"));
    KisCompositeOpOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QCOMPARE(missingData.compositeOpId, QStringLiteral("normal"));
    QVERIFY(!missingData.eraserMode);
    QCOMPARE(missingSetting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("既存値"));
    QCOMPARE(missingSetting.getProperties().size(), 1);

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("CompositeOp"), QStringLiteral("合成/🌿"));
    setting.setProperty(QStringLiteral("EraserMode"), true);
    setting.setProperty(QStringLiteral("unrelated/保持"), 73);
    KisCompositeOpOptionData data;

    QVERIFY(data.read(&setting));
    QCOMPARE(data.compositeOpId, QStringLiteral("合成/🌿"));
    QVERIFY(data.eraserMode);
    QCOMPARE(setting.getProperties().size(), 3);
}

void KisCompositeOpOptionDataContractTest::writeMapsBothMembersAndPreservesOtherProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));

    KisCompositeOpOptionData data;
    data.compositeOpId = QStringLiteral("描画/γ");
    data.eraserMode = true;
    data.write(&setting);

    QCOMPARE(setting.getString(QStringLiteral("CompositeOp")), QStringLiteral("描画/γ"));
    QVERIFY(setting.getBool(QStringLiteral("EraserMode")));
    QCOMPARE(setting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("残す"));
    QCOMPARE(setting.getProperties().size(), 3);
    QCOMPARE(data.compositeOpId, QStringLiteral("描画/γ"));
    QVERIFY(data.eraserMode);
}

void KisCompositeOpOptionDataContractTest::equalityDependsOnBothMembersIndependently()
{
    KisCompositeOpOptionData baseline;
    baseline.compositeOpId = QStringLiteral("合成/α");
    baseline.eraserMode = false;

    KisCompositeOpOptionData peer = baseline;
    QVERIFY(baseline == peer);
    QVERIFY(!(baseline != peer));

    peer.compositeOpId = QStringLiteral("合成/β");
    QVERIFY(!(baseline == peer));
    QVERIFY(baseline != peer);

    peer = baseline;
    peer.eraserMode = true;
    QVERIFY(!(baseline == peer));
    QVERIFY(baseline != peer);
}

QTEST_GUILESS_MAIN(KisCompositeOpOptionDataContractTest)

#include "KisCompositeOpOptionDataContractTest.moc"
