/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "color/KisOcioConfiguration.h"

#include <QTest>

namespace
{
KisOcioConfiguration populatedConfiguration()
{
    KisOcioConfiguration configuration;
    configuration.mode = KisOcioConfiguration::OCIO_ENVIRONMENT;
    configuration.configurationPath = QStringLiteral("/設定/映画.ocio");
    configuration.lutPath = QStringLiteral("/色変換/昼.cube");
    configuration.inputColorSpace = QStringLiteral("線形 Rec.2020");
    configuration.displayDevice = QStringLiteral("表示装置 P3");
    configuration.displayView = QStringLiteral("高輝度");
    configuration.look = QStringLiteral("映画調");
    return configuration;
}

void compareConfiguration(const KisOcioConfiguration &actual, const KisOcioConfiguration &expected)
{
    QCOMPARE(actual.mode, expected.mode);
    QCOMPARE(actual.configurationPath, expected.configurationPath);
    QCOMPARE(actual.lutPath, expected.lutPath);
    QCOMPARE(actual.inputColorSpace, expected.inputColorSpace);
    QCOMPARE(actual.displayDevice, expected.displayDevice);
    QCOMPARE(actual.displayView, expected.displayView);
    QCOMPARE(actual.look, expected.look);
}
} // namespace

class KisOcioConfigurationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsAreInternalAndEmpty();
    void modeValuesRemainStable();
    void stringFieldsPreserveIndependentUnicodeValues();
    void copiesPreserveEveryValue();
};

void KisOcioConfigurationContractTest::defaultsAreInternalAndEmpty()
{
    const KisOcioConfiguration configuration;

    QCOMPARE(configuration.mode, KisOcioConfiguration::INTERNAL);
    QVERIFY(configuration.configurationPath.isEmpty());
    QVERIFY(configuration.lutPath.isEmpty());
    QVERIFY(configuration.inputColorSpace.isEmpty());
    QVERIFY(configuration.displayDevice.isEmpty());
    QVERIFY(configuration.displayView.isEmpty());
    QVERIFY(configuration.look.isEmpty());
}

void KisOcioConfigurationContractTest::modeValuesRemainStable()
{
    QCOMPARE(int(KisOcioConfiguration::INTERNAL), 0);
    QCOMPARE(int(KisOcioConfiguration::OCIO_CONFIG), 1);
    QCOMPARE(int(KisOcioConfiguration::OCIO_ENVIRONMENT), 2);

    KisOcioConfiguration configuration;
    const KisOcioConfiguration::Mode modes[]{KisOcioConfiguration::INTERNAL,
                                             KisOcioConfiguration::OCIO_CONFIG,
                                             KisOcioConfiguration::OCIO_ENVIRONMENT};
    for (KisOcioConfiguration::Mode mode : modes) {
        configuration.mode = mode;
        QCOMPARE(configuration.mode, mode);
    }
}

void KisOcioConfigurationContractTest::stringFieldsPreserveIndependentUnicodeValues()
{
    const KisOcioConfiguration configuration = populatedConfiguration();

    QCOMPARE(configuration.mode, KisOcioConfiguration::OCIO_ENVIRONMENT);
    QCOMPARE(configuration.configurationPath, QStringLiteral("/設定/映画.ocio"));
    QCOMPARE(configuration.lutPath, QStringLiteral("/色変換/昼.cube"));
    QCOMPARE(configuration.inputColorSpace, QStringLiteral("線形 Rec.2020"));
    QCOMPARE(configuration.displayDevice, QStringLiteral("表示装置 P3"));
    QCOMPARE(configuration.displayView, QStringLiteral("高輝度"));
    QCOMPARE(configuration.look, QStringLiteral("映画調"));
}

void KisOcioConfigurationContractTest::copiesPreserveEveryValue()
{
    const KisOcioConfiguration expected = populatedConfiguration();
    KisOcioConfiguration source = expected;
    const KisOcioConfiguration copied(source);
    KisOcioConfiguration assigned;
    assigned = source;

    source.mode = KisOcioConfiguration::INTERNAL;
    source.configurationPath = QStringLiteral("変更済み");
    source.lutPath.clear();
    source.inputColorSpace.clear();
    source.displayDevice.clear();
    source.displayView.clear();
    source.look.clear();

    compareConfiguration(copied, expected);
    compareConfiguration(assigned, expected);
}

QTEST_GUILESS_MAIN(KisOcioConfigurationContractTest)

#include "KisOcioConfigurationContractTest.moc"
