/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "asl/kis_offset_on_exit_verifier.h"

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

class KisOffsetOnExitVerifierContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void exitPositionPolicy_data();
    void exitPositionPolicy();
};

void KisOffsetOnExitVerifierContractTest::exitPositionPolicy_data()
{
    QTest::addColumn<qint64>("exitPosition");
    QTest::addColumn<qint64>("expectedFinalPosition");
    QTest::addColumn<int>("expectedSeekCount");

    QTest::newRow("expected-position") << qint64(12) << qint64(12) << 0;
    QTest::newRow("inside-padding") << qint64(10) << qint64(10) << 0;
    QTest::newRow("padding-lower-bound") << qint64(9) << qint64(9) << 0;
    QTest::newRow("below-padding") << qint64(8) << qint64(12) << 1;
    QTest::newRow("past-expected-position") << qint64(13) << qint64(12) << 1;
}

void KisOffsetOnExitVerifierContractTest::exitPositionPolicy()
{
    QFETCH(qint64, exitPosition);
    QFETCH(qint64, expectedFinalPosition);
    QFETCH(int, expectedSeekCount);

    QByteArray bytes(20, 'x');
    SeekTrackingBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    QVERIFY(buffer.seek(2));

    {
        KisOffsetOnExitVerifier verifier(buffer, 10, 3, QStringLiteral("layer"), QStringLiteral("psd"));
        QVERIFY(buffer.seek(exitPosition));
        buffer.resetSeekObservations();
    }

    QCOMPARE(buffer.pos(), expectedFinalPosition);
    QCOMPARE(buffer.seekCount(), expectedSeekCount);
    QCOMPARE(buffer.lastSeekPosition(), expectedSeekCount == 1 ? qint64(12) : qint64(-1));
}

QTEST_GUILESS_MAIN(KisOffsetOnExitVerifierContractTest)

#include "KisOffsetOnExitVerifierContractTest.moc"
