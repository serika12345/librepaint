/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/data/KoUnicodeBlockData.h"

#include <QTest>

class KoUnicodeBlockDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valueConstructionRetainsInclusiveRange();
    void equalityUsesRangeRatherThanTranslatedName();
    void factoryFindsRepresentativeBlocksAndGaps();
    void factoryInstancesOwnIndependentLookupTables();
    void noBlockSentinelCollidesWithMaximumCodepoint();
};

void KoUnicodeBlockDataContractTest::valueConstructionRetainsInclusiveRange()
{
    const KoUnicodeBlockData block(QStringLiteral("Example Block"), 0x0100, 0x017F);

    QCOMPARE(block.name, QStringLiteral("Example Block"));
    QCOMPARE(block.start, 0x0100U);
    QCOMPARE(block.end, 0x017FU);
    QVERIFY(block.match(0x0100U));
    QVERIFY(block.match(0x0140U));
    QVERIFY(block.match(0x017FU));
    QVERIFY(!block.match(0x00FFU));
    QVERIFY(!block.match(0x0180U));
}

void KoUnicodeBlockDataContractTest::equalityUsesRangeRatherThanTranslatedName()
{
    const KoUnicodeBlockData block(QStringLiteral("First Name"), 0x1000, 0x109F);
    const KoUnicodeBlockData translatedName(QStringLiteral("Translated Name"), 0x1000, 0x109F);
    const KoUnicodeBlockData differentStart(QStringLiteral("First Name"), 0x1001, 0x109F);
    const KoUnicodeBlockData differentEnd(QStringLiteral("First Name"), 0x1000, 0x109E);

    QVERIFY(block == translatedName);
    QVERIFY(!(block == differentStart));
    QVERIFY(!(block == differentEnd));
}

void KoUnicodeBlockDataContractTest::factoryFindsRepresentativeBlocksAndGaps()
{
    KoUnicodeBlockDataFactory factory;

    const KoUnicodeBlockData basicLatin = factory.blockForUCS(0x0041U);
    QVERIFY(!basicLatin.name.isEmpty());
    QCOMPARE(basicLatin.start, 0x0000U);
    QCOMPARE(basicLatin.end, 0x007FU);
    QVERIFY(factory.blockForUCS(0x007FU) == basicLatin);

    const KoUnicodeBlockData latinSupplement = factory.blockForUCS(0x0080U);
    QCOMPARE(latinSupplement.start, 0x0080U);
    QCOMPARE(latinSupplement.end, 0x00FFU);
    QVERIFY(!(latinSupplement == basicLatin));

    const KoUnicodeBlockData emoticons = factory.blockForUCS(0x1F600U);
    QVERIFY(!emoticons.name.isEmpty());
    QCOMPARE(emoticons.start, 0x1F600U);
    QCOMPARE(emoticons.end, 0x1F64FU);

    const KoUnicodeBlockData gap = factory.blockForUCS(0x2FE0U);
    QVERIFY(gap == KoUnicodeBlockDataFactory::noBlock());

    const KoUnicodeBlockData outsideUnicode = factory.blockForUCS(0x110000U);
    QVERIFY(outsideUnicode == KoUnicodeBlockDataFactory::noBlock());
}

void KoUnicodeBlockDataContractTest::factoryInstancesOwnIndependentLookupTables()
{
    KoUnicodeBlockData firstResult = [] {
        KoUnicodeBlockDataFactory factory;
        return factory.blockForUCS(0x3042U);
    }();

    firstResult.name.clear();
    firstResult.start = 0U;
    firstResult.end = 0U;

    KoUnicodeBlockDataFactory secondFactory;
    const KoUnicodeBlockData secondResult = secondFactory.blockForUCS(0x3042U);
    QVERIFY(!secondResult.name.isEmpty());
    QCOMPARE(secondResult.start, 0x3040U);
    QCOMPARE(secondResult.end, 0x309FU);
}

void KoUnicodeBlockDataContractTest::noBlockSentinelCollidesWithMaximumCodepoint()
{
    const KoUnicodeBlockData noBlock = KoUnicodeBlockDataFactory::noBlock();

    QVERIFY(!noBlock.name.isEmpty());
    QCOMPARE(noBlock.start, 0x10FFFFU);
    QCOMPARE(noBlock.end, 0x10FFFFU);
    QVERIFY(noBlock.match(0x10FFFFU));

    KoUnicodeBlockDataFactory factory;
    const KoUnicodeBlockData finalUnicodeBlock = factory.blockForUCS(0x10FFFFU);
    QCOMPARE(finalUnicodeBlock.start, 0x100000U);
    QCOMPARE(finalUnicodeBlock.end, 0x10FFFFU);
    QVERIFY(!(finalUnicodeBlock == noBlock));
}

QTEST_GUILESS_MAIN(KoUnicodeBlockDataContractTest)

#include "KoUnicodeBlockDataContractTest.moc"
