/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <kis_update_info.h>

#include "../canvas/kis_canvas_updates_compressor.h"

namespace
{
class TestUpdateInfo final : public KisUpdateInfo
{
public:
    TestUpdateInfo(const QRect &dirtyImageRect, int levelOfDetail, bool compressible = true)
        : m_dirtyImageRect(dirtyImageRect)
        , m_levelOfDetail(levelOfDetail)
        , m_compressible(compressible)
    {
    }

    QRect dirtyImageRect() const override
    {
        return m_dirtyImageRect;
    }

    int levelOfDetail() const override
    {
        return m_levelOfDetail;
    }

    bool canBeCompressed() const override
    {
        return m_compressible;
    }

private:
    QRect m_dirtyImageRect;
    int m_levelOfDetail;
    bool m_compressible;
};

KisUpdateInfoSP update(const QRect &dirtyImageRect, int levelOfDetail)
{
    return new TestUpdateInfo(dirtyImageRect, levelOfDetail);
}
} // namespace

class KisCanvasUpdatesCompressorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsOnlyForFirstPendingUpdate();
    void preservesProjectionUpdateOrdering();
};

void KisCanvasUpdatesCompressorContractTest::startsOnlyForFirstPendingUpdate()
{
    KisCanvasUpdatesCompressor compressor;
    const KisUpdateInfoSP empty = update(QRect(), 0);
    const KisUpdateInfoSP first = update(QRect(0, 0, 4, 4), 0);
    const KisUpdateInfoSP second = update(QRect(8, 8, 4, 4), 0);

    QVERIFY(!compressor.putUpdateInfo(empty));
    QVERIFY(compressor.putUpdateInfo(first));
    QVERIFY(!compressor.putUpdateInfo(second));

    KisUpdateInfoList drained;
    compressor.takeUpdateInfo(drained);
    QCOMPARE(drained.size(), 2);
    QCOMPARE(drained.at(0).data(), first.data());
    QCOMPARE(drained.at(1).data(), second.data());

    KisUpdateInfoList emptyDrain;
    compressor.takeUpdateInfo(emptyDrain);
    QVERIFY(emptyDrain.isEmpty());

    const KisUpdateInfoSP afterDrain = update(QRect(16, 16, 4, 4), 0);
    QVERIFY(compressor.putUpdateInfo(afterDrain));
}

void KisCanvasUpdatesCompressorContractTest::preservesProjectionUpdateOrdering()
{
    KisCanvasUpdatesCompressor compressor;
    const KisUpdateInfoSP covered = update(QRect(4, 4, 4, 4), 0);
    const KisUpdateInfoSP partialOverlap = update(QRect(10, 4, 4, 4), 0);
    const KisUpdateInfoSP batchStart = new KisMarkerUpdateInfo(KisMarkerUpdateInfo::StartBatch, QRect(0, 0, 64, 64));
    const KisUpdateInfoSP otherLevel = update(QRect(4, 4, 4, 4), 1);
    const KisUpdateInfoSP covering = update(QRect(2, 2, 10, 10), 0);

    QVERIFY(compressor.putUpdateInfo(covered));
    QVERIFY(!compressor.putUpdateInfo(partialOverlap));
    QVERIFY(!compressor.putUpdateInfo(batchStart));
    QVERIFY(!compressor.putUpdateInfo(otherLevel));
    QVERIFY(!compressor.putUpdateInfo(covering));

    KisUpdateInfoList drained;
    compressor.takeUpdateInfo(drained);
    QCOMPARE(drained.size(), 4);
    QCOMPARE(drained.at(0).data(), partialOverlap.data());
    QCOMPARE(drained.at(1).data(), batchStart.data());
    QCOMPARE(drained.at(2).data(), otherLevel.data());
    QCOMPARE(drained.at(3).data(), covering.data());
}

QTEST_MAIN(KisCanvasUpdatesCompressorContractTest)

#include "KisCanvasUpdatesCompressorContractTest.moc"
