/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTextureOptionData.h"

#include <kis_paintop_lod_limitations.h>

#include <QTest>

class KisTextureOptionLodContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enabledTextureReportsInstantPreviewLimitation();
};

void KisTextureOptionLodContractTest::enabledTextureReportsInstantPreviewLimitation()
{
    KisTextureOptionData data;
    const KisPaintopLodLimitations disabledLimitations = data.lodLimitations();
    QVERIFY(disabledLimitations.limitations.isEmpty());
    QVERIFY(disabledLimitations.blockers.isEmpty());

    data.isEnabled = true;
    const KisPaintopLodLimitations enabledLimitations = data.lodLimitations();
    QCOMPARE(enabledLimitations.limitations.size(), 1);
    QVERIFY(enabledLimitations.blockers.isEmpty());
    QCOMPARE(enabledLimitations.limitations.constBegin()->id(), QStringLiteral("texture-pattern"));
    QVERIFY(!enabledLimitations.limitations.constBegin()->name().isEmpty());
}

QTEST_GUILESS_MAIN(KisTextureOptionLodContractTest)

#include "KisTextureOptionLodContractTest.moc"
