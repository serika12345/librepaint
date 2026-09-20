/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisStandardOptionData.h>
#include <kis_properties_configuration.h>

#include <QTest>

namespace
{
template<typename Data>
void verifyPersistedStrengthKey(const QString &key)
{
    Data saved;
    saved.strengthValue = 0.375;

    KisPropertiesConfiguration writtenSettings;
    saved.write(&writtenSettings);
    QCOMPARE(writtenSettings.getDouble(key), 0.375);

    KisPropertiesConfiguration savedPreset;
    savedPreset.setProperty(key, 0.625);
    Data restored;
    QVERIFY(restored.read(&savedPreset));
    QCOMPARE(restored.strengthValue, 0.625);
}
} // namespace

class KisStandardOptionDataCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void savedPresetKeysRestoreStandardCurveStrengths();
};

void KisStandardOptionDataCompatibilityTest::savedPresetKeysRestoreStandardCurveStrengths()
{
    // Compatibility requirement: Saved paint-op presets depend on the established curve option setting keys.
    // Consumer: Artists reopening presets that use standard opacity, color, texture, and brush dynamics options.
    // Operation: A standard curve option saves its strength and a preset containing that saved key is read.
    // Observable result: Each established setting key stores and restores the option's strength.
    // Failure impact: Existing presets lose a standard brush option value or apply it to the wrong behavior.
    verifyPersistedStrengthKey<KisOpacityOptionData>(QStringLiteral("OpacityValue"));
    verifyPersistedStrengthKey<KisFlowOptionData>(QStringLiteral("FlowValue"));
    verifyPersistedStrengthKey<KisRatioOptionData>(QStringLiteral("RatioValue"));
    verifyPersistedStrengthKey<KisSoftnessOptionData>(QStringLiteral("SoftnessValue"));
    verifyPersistedStrengthKey<KisRotationOptionData>(QStringLiteral("RotationValue"));
    verifyPersistedStrengthKey<KisDarkenOptionData>(QStringLiteral("DarkenValue"));
    verifyPersistedStrengthKey<KisMixOptionData>(QStringLiteral("MixValue"));
    verifyPersistedStrengthKey<KisHueOptionData>(QStringLiteral("hValue"));
    verifyPersistedStrengthKey<KisSaturationOptionData>(QStringLiteral("sValue"));
    verifyPersistedStrengthKey<KisValueOptionData>(QStringLiteral("vValue"));
    verifyPersistedStrengthKey<KisRateOptionData>(QStringLiteral("RateValue"));
    verifyPersistedStrengthKey<KisStrengthOptionData>(QStringLiteral("Texture/Strength/Value"));
    verifyPersistedStrengthKey<KisLightnessStrengthOptionData>(QStringLiteral("LightnessStrengthValue"));
}

QTEST_GUILESS_MAIN(KisStandardOptionDataCompatibilityTest)
#include "KisStandardOptionDataCompatibilityTest.moc"
