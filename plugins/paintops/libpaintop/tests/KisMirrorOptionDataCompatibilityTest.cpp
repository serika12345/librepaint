/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisMirrorOption.h>
#include <KisMirrorOptionData.h>
#include <KisMirrorProperties.h>
#include <KisPaintopSettingsIds.h>
#include <brushengine/kis_paint_information.h>
#include <kis_properties_configuration.h>

#include <QTest>

class KisMirrorOptionDataCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void savedMirrorDirectionsRestoreDabOrientation();
    void savedMaskingMirrorDirectionsRestoreFromPresetPrefix();
};

void KisMirrorOptionDataCompatibilityTest::savedMirrorDirectionsRestoreDabOrientation()
{
    // Compatibility requirement: Saved paint-op presets depend on the HorizontalMirrorEnabled and VerticalMirrorEnabled setting keys.
    // Consumer: Artists reopening a brush preset with horizontal or vertical dab mirroring enabled.
    // Operation: The saved mirror settings are loaded and applied to a fully pressed dab.
    // Observable result: The dab receives the saved horizontal mirror direction and an inverted coordinate system.
    // Failure impact: Reopened brush presets draw dabs with the wrong orientation.
    KisMirrorOptionData saved;
    saved.isChecked = true;
    saved.useCurve = false;
    saved.enableHorizontalMirror = true;

    KisPropertiesConfiguration preset;
    saved.write(&preset);

    QVERIFY(preset.getBool(QStringLiteral("HorizontalMirrorEnabled")));
    QVERIFY(!preset.getBool(QStringLiteral("VerticalMirrorEnabled"), true));

    KisMirrorOption restored(&preset);
    const MirrorProperties mirroring = restored.apply(KisPaintInformation(QPointF(), 1.0));
    QVERIFY(mirroring.horizontalMirror);
    QVERIFY(!mirroring.verticalMirror);
    QVERIFY(mirroring.coordinateSystemFlipped);
}

void KisMirrorOptionDataCompatibilityTest::savedMaskingMirrorDirectionsRestoreFromPresetPrefix()
{
    // Consumer: Artists reopening a preset with a mirrored masking brush.
    // Operation: The masking brush saves its vertical mirror direction below the established preset prefix and is loaded again.
    // Observable result: The embedded masking configuration restores its vertical mirror direction without enabling horizontal mirroring.
    // Failure impact: Reopened masking brushes apply their dab pattern in the wrong direction.
    const QString presetPrefix = QString::fromLatin1(KisPaintOpUtils::MaskingBrushPresetPrefix);
    KisMirrorOptionData saved(presetPrefix);
    saved.enableVerticalMirror = true;

    KisPropertiesConfiguration preset;
    saved.write(&preset);
    QVERIFY(preset.getBool(presetPrefix + QStringLiteral("VerticalMirrorEnabled")));
    QVERIFY(!preset.getBool(presetPrefix + QStringLiteral("HorizontalMirrorEnabled"), true));

    KisPropertiesConfiguration embeddedPreset;
    preset.getPrefixedProperties(presetPrefix, &embeddedPreset);

    KisMirrorOptionData restored;
    QVERIFY(restored.read(&embeddedPreset));
    QVERIFY(restored.enableVerticalMirror);
    QVERIFY(!restored.enableHorizontalMirror);
}

QTEST_GUILESS_MAIN(KisMirrorOptionDataCompatibilityTest)
#include "KisMirrorOptionDataCompatibilityTest.moc"
