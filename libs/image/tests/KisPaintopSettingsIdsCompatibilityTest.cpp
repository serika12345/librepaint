/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintopSettingsIds.h"

#include <QByteArray>
#include <QTest>

class KisPaintopSettingsIdsCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maskingBrushIdsPreservePresetCompatibility();
};

// Compatibility requirement: Saved brush presets and paint-op plugins depend on the masking-brush setting keys and paint-op ID.
void KisPaintopSettingsIdsCompatibilityTest::maskingBrushIdsPreservePresetCompatibility()
{
    // Consumer: Users reopening brush presets with an embedded masking brush.
    // Operation: Read the masking-brush paint-op ID and stored setting keys from a preset.
    // Observable result: The embedded brush and its enabled state, blend mode, size, and options load.
    // Failure impact: Existing presets lose their masking-brush behavior or load with default settings.
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushPaintOpId), QByteArray("paintbrush"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushEnabledTag), QByteArray("MaskingBrush/Enabled"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushCompositeOpTag), QByteArray("MaskingBrush/MaskingCompositeOp"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushUseMasterSizeTag), QByteArray("MaskingBrush/UseMasterSize"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushMasterSizeCoeffTag), QByteArray("MaskingBrush/MasterSizeCoeff"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushPresetPrefix), QByteArray("MaskingBrush/Preset/"));
}

QTEST_GUILESS_MAIN(KisPaintopSettingsIdsCompatibilityTest)

#include "KisPaintopSettingsIdsCompatibilityTest.moc"
