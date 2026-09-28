/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAirbrushOptionData.h"

#include <kis_paintop_settings.h>
#include <kis_properties_configuration.h>

#include <QTest>

class KisAirbrushOptionDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsAndEqualityReflectEveryValue();
    void writePersistsOnlyAirbrushProperties();
    void readRestoresEveryPersistedProperty();
    void missingPropertiesUseReadDefaults();
};

void KisAirbrushOptionDataContractTest::defaultsAndEqualityReflectEveryValue()
{
    const KisAirbrushOptionData defaults;
    QVERIFY(!defaults.isChecked);
    QCOMPARE(defaults.airbrushRate, 50.0);
    QVERIFY(!defaults.ignoreSpacing);

    KisAirbrushOptionData same;
    QCOMPARE(defaults, same);

    same.isChecked = true;
    QVERIFY(defaults != same);
    same = defaults;
    same.airbrushRate = 75.0;
    QVERIFY(defaults != same);
    same = defaults;
    same.ignoreSpacing = true;
    QVERIFY(defaults != same);
}

void KisAirbrushOptionDataContractTest::writePersistsOnlyAirbrushProperties()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated"), 91);

    KisAirbrushOptionData data;
    data.isChecked = true;
    data.airbrushRate = 37.5;
    data.ignoreSpacing = true;
    data.write(&setting);

    QCOMPARE(setting.getProperties().size(), 4);
    QCOMPARE(setting.getBool(AIRBRUSH_ENABLED), true);
    QCOMPARE(setting.getDouble(AIRBRUSH_RATE), 37.5);
    QCOMPARE(setting.getBool(AIRBRUSH_IGNORE_SPACING), true);
    QCOMPARE(setting.getProperty(QStringLiteral("unrelated")).toInt(), 91);
}

void KisAirbrushOptionDataContractTest::readRestoresEveryPersistedProperty()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(AIRBRUSH_ENABLED, true);
    setting.setProperty(AIRBRUSH_RATE, 12.25);
    setting.setProperty(AIRBRUSH_IGNORE_SPACING, true);

    KisAirbrushOptionData data;
    QVERIFY(data.read(&setting));
    QVERIFY(data.isChecked);
    QCOMPARE(data.airbrushRate, 12.25);
    QVERIFY(data.ignoreSpacing);
    QCOMPARE(setting.getProperties().size(), 3);
}

void KisAirbrushOptionDataContractTest::missingPropertiesUseReadDefaults()
{
    KisPropertiesConfiguration setting;
    KisAirbrushOptionData data;
    data.isChecked = true;
    data.airbrushRate = 88.0;
    data.ignoreSpacing = true;

    QVERIFY(data.read(&setting));
    QVERIFY(!data.isChecked);
    QCOMPARE(data.airbrushRate, 20.0);
    QVERIFY(!data.ignoreSpacing);
    QVERIFY(setting.getProperties().isEmpty());
}

QTEST_GUILESS_MAIN(KisAirbrushOptionDataContractTest)

#include "KisAirbrushOptionDataContractTest.moc"
