/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisRectsGrid.h>
#include <KisUsageLogger.h>
#include <kis_assert.h>

#include <QTest>

namespace
{

int recoverableAssertCount = 0;
int safeAssertCount = 0;
QStringList usageMessages;

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
    ++recoverableAssertCount;
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

void KisUsageLogger::log(const QString &message)
{
    usageMessages.append(message);
}

class KisRectsGridContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void constructionAlignsAndInvalidSizeFallsBack();
    void addRectReturnsOnlyNewCellsAndContainsCoverage();
    void removeRectRemovesOnlyFullyContainedCells();
    void alignedOperationsReportChangedCells();
    void boundingRectRetainsAllocatedAreaAfterRemoval();
};

void KisRectsGridContractTest::init()
{
    recoverableAssertCount = 0;
    safeAssertCount = 0;
    usageMessages.clear();
}

void KisRectsGridContractTest::constructionAlignsAndInvalidSizeFallsBack()
{
    KisRectsGrid grid(8);
    QCOMPARE(grid.alignRect(QRect(5, 5, 10, 10)), QRect(0, 0, 16, 16));
    QVERIFY(usageMessages.isEmpty());

    KisRectsGrid invalidGrid(3);
    QCOMPARE(invalidGrid.alignRect(QRect(5, 5, 10, 10)), QRect(0, 0, 64, 64));
    QCOMPARE(usageMessages.size(), 1);
    QVERIFY(usageMessages.constFirst().contains(QStringLiteral("Grid size: 3")));
    QCOMPARE(recoverableAssertCount, 0);
    QCOMPARE(safeAssertCount, 0);
}

void KisRectsGridContractTest::addRectReturnsOnlyNewCellsAndContainsCoverage()
{
    KisRectsGrid grid(8);

    const QVector<QRect> added = grid.addRect(QRect(1, 1, 10, 4));
    QCOMPARE(added, QVector<QRect>({QRect(0, 0, 8, 8), QRect(8, 0, 8, 8)}));
    QVERIFY(grid.contains(QRect(1, 1, 10, 4)));
    QVERIFY(!grid.contains(QRect(16, 0, 1, 1)));
    QVERIFY(grid.addRect(QRect(1, 1, 10, 4)).isEmpty());
    QCOMPARE(recoverableAssertCount, 0);
    QCOMPARE(safeAssertCount, 0);
}

void KisRectsGridContractTest::removeRectRemovesOnlyFullyContainedCells()
{
    KisRectsGrid grid(8);
    grid.addAlignedRect(QRect(0, 0, 16, 8));

    const QVector<QRect> removed = grid.removeRect(QRect(0, 0, 9, 8));
    QCOMPARE(removed, QVector<QRect>({QRect(0, 0, 8, 8)}));
    QVERIFY(!grid.contains(QRect(0, 0, 8, 8)));
    QVERIFY(grid.contains(QRect(8, 0, 8, 8)));

    QVERIFY(grid.removeRect(QRect(9, 1, 2, 2)).isEmpty());
    QVERIFY(grid.contains(QRect(8, 0, 8, 8)));
    QCOMPARE(recoverableAssertCount, 0);
    QCOMPARE(safeAssertCount, 0);
}

void KisRectsGridContractTest::alignedOperationsReportChangedCells()
{
    KisRectsGrid grid(8);

    const QVector<QRect> added = grid.addAlignedRect(QRect(-8, 0, 24, 8));
    QCOMPARE(added, QVector<QRect>({QRect(-8, 0, 8, 8), QRect(0, 0, 8, 8), QRect(8, 0, 8, 8)}));

    const QVector<QRect> removed = grid.removeAlignedRect(QRect(0, 0, 8, 8));
    QCOMPARE(removed, QVector<QRect>({QRect(0, 0, 8, 8)}));
    QVERIFY(grid.removeAlignedRect(QRect(0, 0, 8, 8)).isEmpty());
    QCOMPARE(recoverableAssertCount, 0);
    QCOMPARE(safeAssertCount, 0);
}

void KisRectsGridContractTest::boundingRectRetainsAllocatedAreaAfterRemoval()
{
    KisRectsGrid grid(8);
    QCOMPARE(grid.boundingRect(), QRect());

    grid.addAlignedRect(QRect(0, 0, 8, 8));
    QCOMPARE(grid.boundingRect(), QRect(0, 0, 8, 8));

    grid.removeAlignedRect(QRect(0, 0, 8, 8));
    QVERIFY(!grid.contains(QRect(0, 0, 8, 8)));
    QCOMPARE(grid.boundingRect(), QRect(0, 0, 8, 8));
    QCOMPARE(recoverableAssertCount, 0);
    QCOMPARE(safeAssertCount, 0);
}

QTEST_GUILESS_MAIN(KisRectsGridContractTest)

#include "KisRectsGridContractTest.moc"
