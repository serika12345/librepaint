/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <KisGpuPixelTolerance.h>
#include <QTest>

class KisGpuPixelToleranceTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void acceptsOneEightBitStep();
    void rejectsDifferencesOutsideTheContract();
};

void KisGpuPixelToleranceTest::acceptsOneEightBitStep()
{
    QVERIFY(pixelStepDifference(QByteArray::fromHex("01112080"), QByteArray::fromHex("00102181")).isEmpty());
    const quint16 actual[] = {257, 1000, 65535, 32768};
    const quint16 expected[] = {0, 1257, 65278, 33025};
    QVERIFY(pixelStepDifference(QByteArray(reinterpret_cast<const char *>(actual), sizeof(actual)),
        QByteArray(reinterpret_cast<const char *>(expected), sizeof(expected)), 2).isEmpty());
}

void KisGpuPixelToleranceTest::rejectsDifferencesOutsideTheContract()
{
    // #89 P6: hidden RGB and the zero/nonzero alpha boundary are part of #107.
    QVERIFY(!pixelStepDifference(QByteArray::fromHex("02000000"), QByteArray::fromHex("00000000")).isEmpty());
    QVERIFY(!pixelStepDifference(QByteArray::fromHex("00000001"), QByteArray::fromHex("00000000")).isEmpty());
    QVERIFY(!pixelStepDifference(QByteArray(3, '\0'), QByteArray(3, '\0')).isEmpty());
    QVERIFY(!pixelStepDifference({}, {}, 0).isEmpty());
    const quint16 actual[] = {258, 0, 0, 65535};
    const quint16 expected[] = {0, 0, 0, 65535};
    QVERIFY(!pixelStepDifference(QByteArray(reinterpret_cast<const char *>(actual), sizeof(actual)),
        QByteArray(reinterpret_cast<const char *>(expected), sizeof(expected)), 2).isEmpty());
}

QTEST_GUILESS_MAIN(KisGpuPixelToleranceTest)
#include "KisGpuPixelToleranceTest.moc"
