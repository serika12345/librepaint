/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QTest>

#include <kis_painting_utils.h>

class KisPaintingUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicFunctionSignaturesRemainStable();
};

void KisPaintingUtilsSchemaContractTest::publicFunctionSignaturesRemainStable()
{
    using SampleColor = bool (*)(KoColor &, KisPaintDeviceSP, const QPoint &, const KoColor *, int, int, bool);
    using FindNode = KisNodeSP (*)(KisNodeSP, const QPoint &, bool, bool);
    using FindNodes = KisNodeList (*)(KisNodeSP, const QPoint &, bool, bool, bool);

    static_assert(std::is_same_v<decltype(&KisPaintingUtils::sampleColor), SampleColor>);
    static_assert(std::is_same_v<decltype(&KisPaintingUtils::findNode), FindNode>);
    static_assert(std::is_same_v<decltype(&KisPaintingUtils::findNodes), FindNodes>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisPaintingUtilsSchemaContractTest)

#include "KisPaintingUtilsSchemaContractTest.moc"
