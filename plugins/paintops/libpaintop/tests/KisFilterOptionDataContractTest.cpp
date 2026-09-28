/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisFilterOptionData.h"

#include <kis_properties_configuration.h>

#include <QTest>

class KisFilterOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultValuesAndTagsRemainStable();
    void readMapsUnicodeConfigurationState();
    void writeMapsAllMembersAndPreservesOtherProperties();
    void equalityDependsOnEveryMember();
};

void KisFilterOptionDataContractTest::defaultValuesAndTagsRemainStable()
{
    const KisFilterOptionData data;
    QVERIFY(data.filterId.isEmpty());
    QVERIFY(data.filterConfig.isEmpty());
    QVERIFY(!data.smudgeMode);
    QCOMPARE(KisFilterOptionData::filterIdTag(), QStringLiteral("Filter/id"));
    QCOMPARE(KisFilterOptionData::filterConfigTag(), QStringLiteral("Filter/configuration"));
}

void KisFilterOptionDataContractTest::readMapsUnicodeConfigurationState()
{
    KisPropertiesConfiguration missingSetting;
    KisFilterOptionData missingData;

    QVERIFY(missingData.read(&missingSetting));
    QVERIFY(missingData.filterId.isEmpty());
    QVERIFY(missingData.filterConfig.isEmpty());
    QVERIFY(!missingData.smudgeMode);
    QVERIFY(missingSetting.getProperties().isEmpty());

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("Filter/id"), QStringLiteral("ぼかし/β"));
    setting.setProperty(QStringLiteral("Filter/configuration"), QStringLiteral("<設定 mode=\"β\">値</設定>"));
    setting.setProperty(QStringLiteral("Filter/smudgeMode"), true);
    KisFilterOptionData data;

    QVERIFY(data.read(&setting));
    QCOMPARE(data.filterId, QStringLiteral("ぼかし/β"));
    QCOMPARE(data.filterConfig, QStringLiteral("<設定 mode=\"β\">値</設定>"));
    QVERIFY(data.smudgeMode);
    QCOMPARE(setting.getProperties().size(), 3);
}

void KisFilterOptionDataContractTest::writeMapsAllMembersAndPreservesOtherProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated/保持"), QStringLiteral("残す"));

    KisFilterOptionData data;
    data.filterId = QStringLiteral("輪郭/γ");
    data.filterConfig = QStringLiteral("<設定 strength=\"42\"/>");
    data.smudgeMode = true;
    data.write(&setting);

    QCOMPARE(setting.getString(QStringLiteral("Filter/id")), QStringLiteral("輪郭/γ"));
    QCOMPARE(setting.getString(QStringLiteral("Filter/configuration")), QStringLiteral("<設定 strength=\"42\"/>"));
    QVERIFY(setting.getBool(QStringLiteral("Filter/smudgeMode")));
    QCOMPARE(setting.getString(QStringLiteral("unrelated/保持")), QStringLiteral("残す"));
    QCOMPARE(setting.getProperties().size(), 4);

    QCOMPARE(data.filterId, QStringLiteral("輪郭/γ"));
    QCOMPARE(data.filterConfig, QStringLiteral("<設定 strength=\"42\"/>"));
    QVERIFY(data.smudgeMode);
}

void KisFilterOptionDataContractTest::equalityDependsOnEveryMember()
{
    KisFilterOptionData baseline;
    baseline.filterId = QStringLiteral("filter/α");
    baseline.filterConfig = QStringLiteral("<config>値</config>");
    baseline.smudgeMode = false;

    KisFilterOptionData same = baseline;
    QVERIFY(baseline == same);
    QVERIFY(!(baseline != same));

    same.filterId = QStringLiteral("filter/β");
    QVERIFY(baseline != same);

    same = baseline;
    same.filterConfig = QStringLiteral("<config>別</config>");
    QVERIFY(baseline != same);

    same = baseline;
    same.smudgeMode = true;
    QVERIFY(baseline != same);
}

QTEST_GUILESS_MAIN(KisFilterOptionDataContractTest)

#include "KisFilterOptionDataContractTest.moc"
