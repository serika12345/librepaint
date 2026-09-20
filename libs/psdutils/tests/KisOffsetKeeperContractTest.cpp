/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "asl/kis_offset_keeper.h"

#include <QBuffer>
#include <QTest>

namespace
{

class SeekTrackingBuffer : public QBuffer
{
public:
    using QBuffer::QBuffer;

    bool seek(qint64 position) override
    {
        ++m_seekCount;
        m_lastSeekPosition = position;
        return QBuffer::seek(position);
    }

    void resetSeekObservations()
    {
        m_seekCount = 0;
        m_lastSeekPosition = -1;
    }

    int seekCount() const
    {
        return m_seekCount;
    }

    qint64 lastSeekPosition() const
    {
        return m_lastSeekPosition;
    }

private:
    int m_seekCount{0};
    qint64 m_lastSeekPosition{-1};
};

} // namespace

class KisOffsetKeeperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void changedPositionIsRestoredOnDestruction();
    void unchangedPositionDoesNotSeekOnDestruction();
};

void KisOffsetKeeperContractTest::changedPositionIsRestoredOnDestruction()
{
    QByteArray bytes("0123456789");
    SeekTrackingBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    QVERIFY(buffer.seek(3));

    {
        KisOffsetKeeper keeper(buffer);
        QVERIFY(buffer.seek(8));
        buffer.resetSeekObservations();
    }

    QCOMPARE(buffer.pos(), 3);
    QCOMPARE(buffer.seekCount(), 1);
    QCOMPARE(buffer.lastSeekPosition(), 3);
}

void KisOffsetKeeperContractTest::unchangedPositionDoesNotSeekOnDestruction()
{
    QByteArray bytes("0123456789");
    SeekTrackingBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    QVERIFY(buffer.seek(5));
    buffer.resetSeekObservations();

    {
        KisOffsetKeeper keeper(buffer);
        QCOMPARE(buffer.pos(), 5);
    }

    QCOMPARE(buffer.pos(), 5);
    QCOMPARE(buffer.seekCount(), 0);
    QCOMPARE(buffer.lastSeekPosition(), -1);
}

QTEST_GUILESS_MAIN(KisOffsetKeeperContractTest)

#include "KisOffsetKeeperContractTest.moc"
