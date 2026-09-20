/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisCurveOptionDataTest.h"

#include <KisCurveOptionData.h>
#include <kis_properties_configuration.h>

void KisCurveOptionDataTest::savedCurveRestoresBrushResponse()
{
    // Consumer: Artists reopening a brush preset with pressure and rotation dynamics.
    // Operation: A preset saves a shared curve, strength, and active rotation response, then is loaded again.
    // Observable result: The restored brush keeps the enabled curve controls and produces the configured sensor response.
    // Failure impact: Reopened presets paint with a different pressure, rotation, or strength response.
    KisCurveOptionData saved(KoID("Opacity"),
                               KisCurveOptionData::Checkability::NotCheckable);
    saved.strengthValue = 0.35;
    saved.useCurve = true;
    saved.useSameCurve = true;
    saved.commonCurve = "0.0,0.2;1,0.8;";
    saved.sensorStruct().sensorPressure.curve = saved.commonCurve;
    saved.sensorStruct().sensorRotation.isActive = true;
    saved.sensorStruct().sensorRotation.curve = "0.0,1.0;1,0.0;";

    KisPropertiesConfiguration config;
    saved.write(&config);

    KisCurveOptionData restored(KoID("Opacity"),
                                 KisCurveOptionData::Checkability::NotCheckable);
    QVERIFY(restored.read(&config));
    QVERIFY(restored.isChecked);
    QVERIFY(restored.useCurve);
    QVERIFY(restored.useSameCurve);
    QCOMPARE(restored.strengthValue, 0.35);
    QCOMPARE(restored.commonCurve, QStringLiteral("0.0,0.2;1,0.8;"));
    QVERIFY(restored.sensorStruct().sensorPressure.isActive);
    QCOMPARE(restored.sensorStruct().sensorPressure.curve, QStringLiteral("0.0,0.2;1,0.8;"));
    QVERIFY(restored.sensorStruct().sensorRotation.isActive);
    QCOMPARE(restored.sensorStruct().sensorRotation.curve, QStringLiteral("0.0,1.0;1,0.0;"));
}

void KisCurveOptionDataTest::prefixedCurveSettingsRemainSeparate()
{
    // Consumer: Paint-op presets that store several curve options in one configuration.
    // Operation: Two prefixed curve options and an unrelated preset value are saved, then one option is restored.
    // Observable result: The restored option receives its own curve settings and unrelated preset data remains available.
    // Failure impact: Saving one brush option overwrites another option or unrelated preset configuration.
    KisCurveOptionData saved(QStringLiteral("masking/"),
                             KoID("Opacity"),
                             KisCurveOptionData::Checkability::CheckableIfHasPrefix);
    saved.isChecked = true;
    saved.strengthValue = 0.6;
    saved.commonCurve = "0,0;0.5,0.25;1,1;";

    KisCurveOptionData other(QStringLiteral("texture/"),
                             KoID("Opacity"),
                             KisCurveOptionData::Checkability::CheckableIfHasPrefix);
    other.isChecked = true;
    other.strengthValue = 0.2;
    other.commonCurve = "0,1;1,0;";

    KisPropertiesConfiguration config;
    config.setProperty(QStringLiteral("preset/name"), QStringLiteral("Unchanged"));
    saved.write(&config);
    other.write(&config);

    KisCurveOptionData restored(QStringLiteral("masking/"),
                                KoID("Opacity"),
                                KisCurveOptionData::Checkability::CheckableIfHasPrefix);
    QVERIFY(restored.read(&config));
    QVERIFY(restored.isCheckable);
    QVERIFY(restored.isChecked);
    QCOMPARE(restored.strengthValue, 0.6);
    QCOMPARE(restored.commonCurve, QStringLiteral("0,0;0.5,0.25;1,1;"));
    QCOMPARE(config.getProperty(QStringLiteral("preset/name")).toString(), QStringLiteral("Unchanged"));
}

void KisCurveOptionDataTest::disabledSensorsStayDisabledAfterReload()
{
    // Consumer: Artists who disable a sensor while keeping another sensor-driven curve active.
    // Operation: A preset disables pressure and enables rotation before it is saved and loaded.
    // Observable result: Pressure remains disabled and the active rotation curve is restored.
    // Failure impact: Reopened brushes respond to an input the artist disabled or lose the active response.
    KisCurveOptionData saved(KoID("Opacity"),
                             KisCurveOptionData::Checkability::NotCheckable);
    saved.sensorStruct().sensorPressure.isActive = false;
    saved.sensorStruct().sensorPressure.curve = "0.0,0.5;1,1;";
    saved.sensorStruct().sensorRotation.isActive = true;
    saved.sensorStruct().sensorRotation.curve = "0.0,0.5;1,1;";

    KisCurveOptionData restored(KoID("Opacity"),
                                KisCurveOptionData::Checkability::NotCheckable);

    KisPropertiesConfiguration config;
    saved.write(&config);
    QVERIFY(restored.read(&config));

    QVERIFY(!restored.sensorStruct().sensorPressure.isActive);
    QVERIFY(restored.sensorStruct().sensorRotation.isActive);
    QCOMPARE(restored.sensorStruct().sensorRotation.curve, QStringLiteral("0.0,0.5;1,1;"));
}

void KisCurveOptionDataTest::missingSensorsUsePressureDefault()
{
    // Consumer: Artists loading a preset that has no valid dynamic sensor selection.
    // Operation: A curve option is initialized without a saved sensor list.
    // Observable result: The pressure sensor is active with the default curve.
    // Failure impact: The preset has no responsive input curve and paints with unexpected dynamics.
    const KisCurveOptionData data(KoID("Opacity"),
                                  KisCurveOptionData::Checkability::NotCheckable);

    QVERIFY(data.sensorStruct().sensorPressure.isActive);
    QCOMPARE(data.sensorStruct().sensorPressure.curve, DEFAULT_CURVE_STRING);
}

SIMPLE_TEST_MAIN(KisCurveOptionDataTest)
