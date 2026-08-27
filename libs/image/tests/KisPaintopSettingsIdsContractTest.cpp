/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintopSettingsIds.h"

#include <QByteArray>
#include <QTest>

class KisPaintopSettingsIdsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maskingBrushIdsPreservePresetCompatibility();
};

void KisPaintopSettingsIdsContractTest::maskingBrushIdsPreservePresetCompatibility()
{
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushPaintOpId), QByteArray("paintbrush"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushEnabledTag), QByteArray("MaskingBrush/Enabled"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushCompositeOpTag), QByteArray("MaskingBrush/MaskingCompositeOp"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushUseMasterSizeTag), QByteArray("MaskingBrush/UseMasterSize"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushMasterSizeCoeffTag), QByteArray("MaskingBrush/MasterSizeCoeff"));
    QCOMPARE(QByteArray(KisPaintOpUtils::MaskingBrushPresetPrefix), QByteArray("MaskingBrush/Preset/"));
}

QTEST_GUILESS_MAIN(KisPaintopSettingsIdsContractTest)

#include "KisPaintopSettingsIdsContractTest.moc"
