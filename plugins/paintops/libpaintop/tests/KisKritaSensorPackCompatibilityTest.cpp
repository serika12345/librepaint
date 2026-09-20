/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCurveOptionData.h>
#include <kis_properties_configuration.h>

#include <QTest>

#include <algorithm>
#include <array>

class KisKritaSensorPackCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void legacySensorIdentifiersRestoreSelectedInput();
};

void KisKritaSensorPackCompatibilityTest::legacySensorIdentifiersRestoreSelectedInput()
{
    // Compatibility requirement: Saved paint-op presets depend on the legacy SizeSensor XML identifiers.
    // Consumer: Artists reopening presets that select a dynamic input sensor for a size curve.
    // Operation: A size option reads each supported legacy single-sensor XML definition.
    // Observable result: The selected sensor becomes active and restores its curve.
    // Failure impact: Existing presets lose dynamics or apply their curve to a different input source.
    const std::array<QString, 16> sensorIds = {QStringLiteral("pressure"),
                                               QStringLiteral("pressurein"),
                                               QStringLiteral("tangentialpressure"),
                                               QStringLiteral("drawingangle"),
                                               QStringLiteral("xtilt"),
                                               QStringLiteral("ytilt"),
                                               QStringLiteral("ascension"),
                                               QStringLiteral("declination"),
                                               QStringLiteral("rotation"),
                                               QStringLiteral("fuzzy"),
                                               QStringLiteral("fuzzystroke"),
                                               QStringLiteral("speed"),
                                               QStringLiteral("fade"),
                                               QStringLiteral("distance"),
                                               QStringLiteral("time"),
                                               QStringLiteral("perspective")};

    for (const QString &sensorId : sensorIds) {
        KisPropertiesConfiguration legacyPreset;
        legacyPreset.setProperty(
            QStringLiteral("SizeSensor"),
            QStringLiteral("<sensor id=\"%1\"><curve>0,0;0.5,0.8;1,1;</curve></sensor>").arg(sensorId));

        KisCurveOptionData restored(KoID(QStringLiteral("Size"), QStringLiteral("Size")),
                                    KisCurveOptionData::Checkability::Checkable);
        QVERIFY2(restored.read(&legacyPreset), qPrintable(sensorId));

        const std::vector<KisSensorData *> sensors = restored.sensors();
        const auto activeSensor = std::find_if(sensors.cbegin(), sensors.cend(), [](const KisSensorData *sensor) {
            return sensor->isActive;
        });
        QVERIFY2(activeSensor != sensors.cend(), qPrintable(sensorId));
        QCOMPARE((*activeSensor)->id.id(), sensorId);
        QCOMPARE((*activeSensor)->curve, QStringLiteral("0,0;0.5,0.8;1,1;"));
    }
}

QTEST_GUILESS_MAIN(KisKritaSensorPackCompatibilityTest)
#include "KisKritaSensorPackCompatibilityTest.moc"
