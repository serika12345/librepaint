/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <animation/kis_animation_frame_cache_index.h>
#include <kis_time_span.h>

class KisAnimationFrameCacheIndexTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void insertionAndInvalidationPreserveUnaffectedRanges();
    void glueReportsStorageChanges();
};

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
    QCOMPARE(changes.storageOperations[0].type,
             KisAnimationFrameCacheIndex::StorageOperation::Move);
    QCOMPARE(changes.storageOperations[0].sourceFrameId, 10);
    QCOMPARE(changes.storageOperations[0].destinationFrameId, 13);
    QCOMPARE(index.frameIdAtTime(12), -1);
    QCOMPARE(index.frameIdAtTime(13), 13);
    QCOMPARE(index.frameIdAtTime(16), 13);
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
    QCOMPARE(changes.storageOperations[0].type,
             KisAnimationFrameCacheIndex::StorageOperation::Forget);
    QCOMPARE(changes.storageOperations[0].sourceFrameId, 5);
    QCOMPARE(changes.storageOperations[1].type,
             KisAnimationFrameCacheIndex::StorageOperation::Move);
    QCOMPARE(changes.storageOperations[1].sourceFrameId, 8);
    QCOMPARE(changes.storageOperations[1].destinationFrameId, 9);
    QCOMPARE(index.frameIdAtTime(0), 0);
    QCOMPARE(index.frameIdAtTime(8), 0);
    QCOMPARE(index.frameIdAtTime(9), 9);
    QCOMPARE(index.frameIdAtTime(10), 9);
    QCOMPARE(index.frameIdAtTime(11), -1);
    QCOMPARE(index.frameIdAtTime(16), 16);
}

QTEST_GUILESS_MAIN(KisAnimationFrameCacheIndexTest)

#include "kis_animation_frame_cache_index_test.moc"
