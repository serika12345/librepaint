/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisLodAvailabilityModel.h>
#include <kis_properties_configuration.h>

#include <QTest>

#include <lager/state.hpp>

#include <tuple>

class KisLodAvailabilityContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void thresholdControlsInstantPreviewAvailability();
    void settingsWritePreservesUserChoices();
    void unknownPaintOpDoesNotReplaceCurrentState();
};

void KisLodAvailabilityContractTest::thresholdControlsInstantPreviewAvailability()
{
    lager::state<KisLodAvailabilityData, lager::automatic_tag> data;
    lager::state<qreal, lager::automatic_tag> effectiveBrushSize(50.0);
    lager::state<KisPaintopLodLimitations, lager::automatic_tag> limitations;
    KisLodAvailabilityModel model(data, effectiveBrushSize, limitations);

    QCOMPARE(std::get<0>(model.availabilityState()),
             KisLodAvailabilityModel::BlockedByThreshold);
    QVERIFY(!model.effectiveLodAvailable());

    QVERIFY(model.setProperty("lodSizeThreshold", 25.0));

    QCOMPARE(std::get<0>(model.availabilityState()),
             KisLodAvailabilityModel::Available);
    QVERIFY(model.effectiveLodAvailable());
}

void KisLodAvailabilityContractTest::settingsWritePreservesUserChoices()
{
    KisLodAvailabilityData data;
    data.isLodUserAllowed = false;
    data.lodSizeThreshold = 37.5;

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("unrelated"), QStringLiteral("preserve"));
    data.write(&setting);

    QVERIFY(!setting.getBool(QStringLiteral("lodUserAllowed"), true));
    QCOMPARE(setting.getDouble(QStringLiteral("lodSizeThreshold"), 0.0), 37.5);
    QCOMPARE(setting.getString(QStringLiteral("unrelated")),
             QStringLiteral("preserve"));
}

void KisLodAvailabilityContractTest::unknownPaintOpDoesNotReplaceCurrentState()
{
    KisLodAvailabilityData data;
    data.isLodUserAllowed = false;
    data.isLodSizeThresholdSupported = false;
    data.lodSizeThreshold = 37.5;

    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("paintop"),
                        QStringLiteral("missing-paint-op"));
    setting.setProperty(QStringLiteral("lodUserAllowed"), true);
    setting.setProperty(QStringLiteral("lodSizeThreshold"), 12.0);

    QVERIFY(!data.read(&setting));
    QVERIFY(!data.isLodUserAllowed);
    QVERIFY(!data.isLodSizeThresholdSupported);
    QCOMPARE(data.lodSizeThreshold, 37.5);
}

QTEST_MAIN(KisLodAvailabilityContractTest)

#include "KisLodAvailabilityContractTest.moc"
