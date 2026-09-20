/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <animation/kis_animation_frame_cache_index.h>
#include <kis_assert.h>
#include <kis_time_span.h>

namespace
{
int safeAssertCount = 0;
QByteArray lastSafeAssertion;
} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    ++safeAssertCount;
    lastSafeAssertion = assertion;
}

class KisAnimationFrameCacheIndexTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultRecordsDescribeNoChange();
    void emptyInvalidRangesAndClearAreStable();
    void finiteAndInfiniteRangesDriveLookupAndUpload();
    void insertionAndInvalidationPreserveUnaffectedRanges();
    void invalidationReportsOrderedStorageOperations();
    void glueReportsStorageChanges();
    void intersectionsRemovalAndSafetyDiagnostics();
};

void KisAnimationFrameCacheIndexTest::defaultRecordsDescribeNoChange()
{
    using Operation = KisAnimationFrameCacheIndex::StorageOperation;

    QCOMPARE(static_cast<int>(Operation::Move), 0);
    QCOMPARE(static_cast<int>(Operation::Forget), 1);

    const Operation operation;
    QCOMPARE(operation.type, Operation::Forget);
    QCOMPARE(operation.sourceFrameId, -1);
    QCOMPARE(operation.destinationFrameId, -1);

    const Operation move{Operation::Move, 3, 7};
    QCOMPARE(move.type, Operation::Move);
    QCOMPARE(move.sourceFrameId, 3);
    QCOMPARE(move.destinationFrameId, 7);

    const KisAnimationFrameCacheIndex::ChangeSet changes;
    QVERIFY(!changes.changed);
    QVERIFY(changes.storageOperations.isEmpty());
}

void KisAnimationFrameCacheIndexTest::emptyInvalidRangesAndClearAreStable()
{
    KisAnimationFrameCacheIndex index;
    const KisTimeSpan validRange = KisTimeSpan::fromTimeToTime(4, 6);
    const KisTimeSpan invalidRange = KisTimeSpan::fromTimeToTime(6, 4);

    QVERIFY(index.isEmpty());
    QCOMPARE(index.frameIdAtTime(4), -1);
    QVERIFY(!index.contains(4));
    QVERIFY(index.shouldUploadNewFrame(4, -1));
    QVERIFY(index.rangesIntersecting(validRange).isEmpty());
    QVERIFY(!index.invalidate(validRange).changed);
    QVERIFY(!index.glue(validRange).changed);
    QVERIFY(!index.removeFrame(4));

    const auto invalidInsert = index.insert(invalidRange);
    QVERIFY(!invalidInsert.changed);
    QVERIFY(invalidInsert.storageOperations.isEmpty());
    QVERIFY(!index.invalidate(invalidRange).changed);
    QVERIFY(!index.glue(invalidRange).changed);
    QVERIFY(index.rangesIntersecting(invalidRange).isEmpty());
    QVERIFY(index.isEmpty());

    QVERIFY(index.insert(validRange).changed);
    QVERIFY(!index.isEmpty());
    index.clear();
    QVERIFY(index.isEmpty());
}

void KisAnimationFrameCacheIndexTest::finiteAndInfiniteRangesDriveLookupAndUpload()
{
    KisAnimationFrameCacheIndex index;

    const auto finiteChanges = index.insert(KisTimeSpan::fromTimeWithDuration(4, 3));
    QVERIFY(finiteChanges.changed);
    QVERIFY(finiteChanges.storageOperations.isEmpty());
    QCOMPARE(index.frameIdAtTime(3), -1);
    QCOMPARE(index.frameIdAtTime(4), 4);
    QCOMPARE(index.frameIdAtTime(6), 4);
    QCOMPARE(index.frameIdAtTime(7), -1);
    QVERIFY(index.contains(4));
    QVERIFY(index.contains(6));
    QVERIFY(!index.contains(7));
    QCOMPARE(index.frameLength(4), 3);
    QVERIFY(!index.shouldUploadNewFrame(6, 4));
    QVERIFY(index.shouldUploadNewFrame(8, 4));
    QVERIFY(index.shouldUploadNewFrame(5, 8));

    const auto infiniteChanges = index.insert(KisTimeSpan::infinite(10));
    QVERIFY(infiniteChanges.changed);
    QVERIFY(infiniteChanges.storageOperations.isEmpty());
    QCOMPARE(index.frameIdAtTime(10), 10);
    QCOMPARE(index.frameIdAtTime(100), 10);
    QCOMPARE(index.frameLength(10), -1);
    QVERIFY(index.contains(100));
    QVERIFY(index.shouldUploadNewFrame(10, 4));
    QVERIFY(!index.shouldUploadNewFrame(100, 10));
}

void KisAnimationFrameCacheIndexTest::insertionAndInvalidationPreserveUnaffectedRanges()
{
    KisAnimationFrameCacheIndex index;

    QCOMPARE(index.insert(KisTimeSpan::fromTimeWithDuration(10, 7)).changed, true);
    QCOMPARE(index.frameIdAtTime(9), -1);
    QCOMPARE(index.frameIdAtTime(10), 10);
    QCOMPARE(index.frameIdAtTime(16), 10);
    QCOMPARE(index.frameIdAtTime(17), -1);

    const auto changes = index.invalidate(KisTimeSpan::fromTimeToTime(10, 12));
    QVERIFY(changes.changed);
    QCOMPARE(changes.storageOperations.size(), 1);
    QCOMPARE(changes.storageOperations[0].type, KisAnimationFrameCacheIndex::StorageOperation::Move);
    QCOMPARE(changes.storageOperations[0].sourceFrameId, 10);
    QCOMPARE(changes.storageOperations[0].destinationFrameId, 13);
    QCOMPARE(index.frameIdAtTime(12), -1);
    QCOMPARE(index.frameIdAtTime(13), 13);
    QCOMPARE(index.frameIdAtTime(16), 13);
}

void KisAnimationFrameCacheIndexTest::invalidationReportsOrderedStorageOperations()
{
    using Operation = KisAnimationFrameCacheIndex::StorageOperation;

    KisAnimationFrameCacheIndex index;
    index.insert(KisTimeSpan::fromTimeWithDuration(0, 3));
    index.insert(KisTimeSpan::fromTimeWithDuration(5, 3));
    index.insert(KisTimeSpan::fromTimeWithDuration(8, 3));
    index.insert(KisTimeSpan::infinite(16));

    const auto changes = index.invalidate(KisTimeSpan::fromTimeToTime(5, 9));

    QVERIFY(changes.changed);
    QCOMPARE(changes.storageOperations.size(), 2);
    QCOMPARE(changes.storageOperations[0].type, Operation::Forget);
    QCOMPARE(changes.storageOperations[0].sourceFrameId, 5);
    QCOMPARE(changes.storageOperations[0].destinationFrameId, -1);
    QCOMPARE(changes.storageOperations[1].type, Operation::Move);
    QCOMPARE(changes.storageOperations[1].sourceFrameId, 8);
    QCOMPARE(changes.storageOperations[1].destinationFrameId, 10);
    QCOMPARE(index.frameIdAtTime(2), 0);
    QCOMPARE(index.frameIdAtTime(5), -1);
    QCOMPARE(index.frameIdAtTime(9), -1);
    QCOMPARE(index.frameIdAtTime(10), 10);
    QCOMPARE(index.frameLength(10), 1);
    QCOMPARE(index.frameIdAtTime(16), 16);
}

void KisAnimationFrameCacheIndexTest::glueReportsStorageChanges()
{
    KisAnimationFrameCacheIndex index;
    index.insert(KisTimeSpan::fromTimeWithDuration(0, 3));
    index.insert(KisTimeSpan::fromTimeWithDuration(5, 3));
    index.insert(KisTimeSpan::fromTimeWithDuration(8, 3));
    index.insert(KisTimeSpan::infinite(16));

    const auto changes = index.glue(KisTimeSpan::fromTimeWithDuration(2, 7));

    QVERIFY(changes.changed);
    QCOMPARE(changes.storageOperations.size(), 2);
    QCOMPARE(changes.storageOperations[0].type, KisAnimationFrameCacheIndex::StorageOperation::Forget);
    QCOMPARE(changes.storageOperations[0].sourceFrameId, 5);
    QCOMPARE(changes.storageOperations[1].type, KisAnimationFrameCacheIndex::StorageOperation::Move);
    QCOMPARE(changes.storageOperations[1].sourceFrameId, 8);
    QCOMPARE(changes.storageOperations[1].destinationFrameId, 9);
    QCOMPARE(index.frameIdAtTime(0), 0);
    QCOMPARE(index.frameIdAtTime(8), 0);
    QCOMPARE(index.frameIdAtTime(9), 9);
    QCOMPARE(index.frameIdAtTime(10), 9);
    QCOMPARE(index.frameIdAtTime(11), -1);
    QCOMPARE(index.frameIdAtTime(16), 16);
    QCOMPARE(index.frameLength(0), 9);
    QCOMPARE(index.frameLength(9), 2);

    const auto infiniteChanges = index.glue(KisTimeSpan::infinite(9));
    QVERIFY(infiniteChanges.changed);
    QCOMPARE(infiniteChanges.storageOperations.size(), 1);
    QCOMPARE(infiniteChanges.storageOperations[0].type, KisAnimationFrameCacheIndex::StorageOperation::Forget);
    QCOMPARE(infiniteChanges.storageOperations[0].sourceFrameId, 16);
    QCOMPARE(infiniteChanges.storageOperations[0].destinationFrameId, -1);
    QCOMPARE(index.frameLength(9), -1);
    QCOMPARE(index.frameIdAtTime(100), 9);
}

void KisAnimationFrameCacheIndexTest::intersectionsRemovalAndSafetyDiagnostics()
{
    KisAnimationFrameCacheIndex index;
    index.insert(KisTimeSpan::fromTimeWithDuration(0, 3));
    index.insert(KisTimeSpan::fromTimeWithDuration(5, 3));
    index.insert(KisTimeSpan::infinite(10));

    const QList<QPair<int, int>> finiteIntersections{{0, 3}, {5, 3}};
    QCOMPARE(index.rangesIntersecting(KisTimeSpan::fromTimeToTime(2, 5)), finiteIntersections);

    const QList<QPair<int, int>> infiniteIntersections{{5, 3}, {10, -1}};
    QCOMPARE(index.rangesIntersecting(KisTimeSpan::infinite(6)), infiniteIntersections);
    QVERIFY(index.rangesIntersecting(KisTimeSpan::fromTimeToTime(8, 9)).isEmpty());

    QVERIFY(index.removeFrame(5));
    QVERIFY(!index.removeFrame(5));
    QVERIFY(!index.contains(5));

    safeAssertCount = 0;
    lastSafeAssertion.clear();
    QCOMPARE(index.frameLength(99), 0);
    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(lastSafeAssertion, QByteArray("m_frames.contains(frameId)"));
}

QTEST_GUILESS_MAIN(KisAnimationFrameCacheIndexTest)

#include "kis_animation_frame_cache_index_test.moc"
