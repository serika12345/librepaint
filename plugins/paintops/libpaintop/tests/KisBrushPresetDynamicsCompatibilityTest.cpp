/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisScatterOption.h>
#include <KisScatterOptionData.h>
#include <KisSharpnessOption.h>
#include <KisSharpnessOptionData.h>
#include <KisSpacingOption.h>
#include <KisSpacingOptionData.h>
#include <brushengine/kis_paint_information.h>
#include <kis_properties_configuration.h>

#include <QTest>

class KisBrushPresetDynamicsCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void savedSharpnessSettingsRestoreOutlineAlignment();
    void legacySharpnessFactorRestoresSharpnessResponse();
    void savedScatterAxesKeepDabsInPlaceWhenDisabled();
    void legacyScatterAmountRestoresScatterStrength();
    void savedSpacingSettingsRestoreSpacingBehavior();
};

void KisBrushPresetDynamicsCompatibilityTest::savedSharpnessSettingsRestoreOutlineAlignment()
{
    // Compatibility requirement: Saved paint-op presets depend on the SharpnessValue, Sharpness/alignoutline, and Sharpness/softness setting keys.
    // Consumer: Artists reopening a brush preset that aligns its outline to pixel boundaries.
    // Operation: The saved sharpness curve and outline settings are loaded for a fully pressed dab.
    // Observable result: The dab position is aligned to whole pixels and retains the saved softness setting.
    // Failure impact: Reopened presets produce blurred outlines or a different sharpness threshold.
    KisSharpnessOptionData saved;
    saved.isChecked = true;
    saved.useCurve = false;
    saved.strengthValue = 1.0;
    saved.alignOutlinePixels = true;
    saved.softness = 41;

    KisPropertiesConfiguration preset;
    saved.write(&preset);
    QCOMPARE(preset.getDouble(QStringLiteral("SharpnessValue")), 1.0);
    QVERIFY(preset.getBool(QStringLiteral("Sharpness/alignoutline")));
    QCOMPARE(preset.getInt(QStringLiteral("Sharpness/softness")), 41);

    KisSharpnessOptionData restoredData;
    QVERIFY(restoredData.read(&preset));
    QCOMPARE(restoredData.softness, 41);

    KisSharpnessOption restored(&preset);
    qint32 x = 0;
    qint32 y = 0;
    qreal xFraction = -1.0;
    qreal yFraction = -1.0;
    restored.apply(KisPaintInformation(QPointF(), 1.0), QPointF(10.4, 20.6), x, y, xFraction, yFraction);
    QCOMPARE(x, 10);
    QCOMPARE(y, 21);
    QCOMPARE(xFraction, 0.0);
    QCOMPARE(yFraction, 0.0);
}

void KisBrushPresetDynamicsCompatibilityTest::legacySharpnessFactorRestoresSharpnessResponse()
{
    // Compatibility requirement: Saved paint-op presets depend on the legacy Sharpness/factor setting key.
    // Consumer: Artists reopening a preset saved before SharpnessValue was introduced.
    // Operation: A preset containing the legacy Sharpness/factor value is loaded.
    // Observable result: The sharpness strength and threshold softness use the legacy factor.
    // Failure impact: Older presets lose their intended edge sharpness.
    KisPropertiesConfiguration legacyPreset;
    legacyPreset.setProperty(QStringLiteral("Sharpness/factor"), 0.375);

    KisSharpnessOptionData restored;
    QVERIFY(restored.read(&legacyPreset));
    QCOMPARE(restored.strengthValue, 0.375);
    QCOMPARE(restored.softness, 37);
}

void KisBrushPresetDynamicsCompatibilityTest::savedScatterAxesKeepDabsInPlaceWhenDisabled()
{
    // Compatibility requirement: Saved paint-op presets depend on the ScatterValue, Scattering/AxisX, and Scattering/AxisY setting keys.
    // Consumer: Artists reopening a preset with scattering disabled on both axes.
    // Operation: The saved scatter curve and axis settings are loaded before placing a dab.
    // Observable result: The dab remains at the input position instead of receiving a random scatter offset.
    // Failure impact: Reopened presets scatter dabs that the artist configured to remain in place.
    KisScatterOptionData saved;
    saved.isChecked = true;
    saved.useCurve = false;
    saved.strengthValue = 3.25;
    saved.axisX = false;
    saved.axisY = false;

    KisPropertiesConfiguration preset;
    saved.write(&preset);
    QCOMPARE(preset.getDouble(QStringLiteral("ScatterValue")), 3.25);
    QVERIFY(!preset.getBool(QStringLiteral("Scattering/AxisX"), true));
    QVERIFY(!preset.getBool(QStringLiteral("Scattering/AxisY"), true));

    KisScatterOption restored(&preset);
    const KisPaintInformation input(QPointF(12.5, 8.25), 1.0);
    QCOMPARE(restored.apply(input, 100.0, 50.0), input.pos());
}

void KisBrushPresetDynamicsCompatibilityTest::legacyScatterAmountRestoresScatterStrength()
{
    // Compatibility requirement: Saved paint-op presets depend on the legacy Scattering/Amount setting key.
    // Consumer: Artists reopening a preset saved before ScatterValue replaced Scattering/Amount.
    // Operation: A preset containing the legacy scatter amount is loaded.
    // Observable result: The scatter curve strength receives the legacy amount.
    // Failure impact: Older presets place dabs with a different scattering distance.
    KisPropertiesConfiguration legacyPreset;
    legacyPreset.setProperty(QStringLiteral("Scattering/Amount"), 3.25);

    KisScatterOptionData restored;
    QVERIFY(restored.read(&legacyPreset));
    QCOMPARE(restored.strengthValue, 3.25);
}

void KisBrushPresetDynamicsCompatibilityTest::savedSpacingSettingsRestoreSpacingBehavior()
{
    // Compatibility requirement: Saved paint-op presets depend on the SpacingValue, Spacing/Isotropic, and PaintOpSettings/updateSpacingBetweenDabs setting keys.
    // Consumer: Artists reopening a preset with isotropic spacing and live spacing updates enabled.
    // Operation: The saved spacing curve and its two behavior settings are loaded before a dab sequence.
    // Observable result: The restored option reports both spacing modes and applies the saved spacing multiplier.
    // Failure impact: Reopened presets space dabs inconsistently or ignore spacing updates during a stroke.
    KisSpacingOptionData saved;
    saved.isChecked = true;
    saved.useCurve = false;
    saved.strengthValue = 0.6;
    saved.isotropicSpacing = true;
    saved.useSpacingUpdates = true;

    KisPropertiesConfiguration preset;
    saved.write(&preset);
    QCOMPARE(preset.getDouble(QStringLiteral("SpacingValue")), 0.6);
    QVERIFY(preset.getBool(QStringLiteral("Spacing/Isotropic")));
    QVERIFY(preset.getBool(QStringLiteral("PaintOpSettings/updateSpacingBetweenDabs")));

    KisSpacingOption restored(&preset);
    QVERIFY(restored.isotropicSpacing());
    QVERIFY(restored.usingSpacingUpdates());
    QCOMPARE(restored.apply(KisPaintInformation(QPointF(), 1.0)), 0.6);
}

QTEST_GUILESS_MAIN(KisBrushPresetDynamicsCompatibilityTest)
#include "KisBrushPresetDynamicsCompatibilityTest.moc"
