/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_thread_safe_signal_compressor.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTest>

#include <thread>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisThreadSafeSignalCompressorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void controlMethodsEmitRequestsAndManagePendingState();
    void workerThreadStartDeliversOnTheApplicationThread();
};

void KisThreadSafeSignalCompressorContractTest::controlMethodsEmitRequestsAndManagePendingState()
{
    KisThreadSafeSignalCompressor compressor(20, KisSignalCompressor::FIRST_ACTIVE);
    QCOMPARE(compressor.objectName(), QStringLiteral("KisThreadSafeSignalCompressor"));
    QCOMPARE(compressor.thread(), QApplication::instance()->thread());
    QVERIFY(!compressor.isActive());

    QSignalSpy requestSpy(&compressor, &KisThreadSafeSignalCompressor::internalRequestSignal);
    QSignalSpy stopSpy(&compressor, &KisThreadSafeSignalCompressor::internalStopSignal);
    QSignalSpy delaySpy(&compressor, &KisThreadSafeSignalCompressor::internalSetDelay);
    QSignalSpy timeoutSpy(&compressor, &KisThreadSafeSignalCompressor::timeout);

    compressor.setDelay(25);
    QCOMPARE(delaySpy.count(), 1);
    QCOMPARE(delaySpy.constFirst().constFirst().toInt(), 25);

    compressor.start();
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(timeoutSpy.count(), 1);
    QVERIFY(!compressor.isActive());

    compressor.start();
    QCOMPARE(requestSpy.count(), 2);
    QVERIFY(compressor.isActive());

    compressor.stop();
    QCOMPARE(stopSpy.count(), 1);
    QVERIFY(!compressor.isActive());
}

void KisThreadSafeSignalCompressorContractTest::workerThreadStartDeliversOnTheApplicationThread()
{
    KisThreadSafeSignalCompressor compressor(10, KisSignalCompressor::FIRST_ACTIVE);
    QSignalSpy timeoutSpy(&compressor, &KisThreadSafeSignalCompressor::timeout);
    QThread *deliveryThread = nullptr;
    connect(&compressor, &KisThreadSafeSignalCompressor::timeout, this, [&deliveryThread] {
        deliveryThread = QThread::currentThread();
    });

    std::thread worker([&compressor] {
        compressor.start();
    });
    worker.join();

    QTRY_COMPARE_WITH_TIMEOUT(timeoutSpy.count(), 1, 100);
    QCOMPARE(deliveryThread, QApplication::instance()->thread());
    compressor.stop();
}

QTEST_MAIN(KisThreadSafeSignalCompressorContractTest)

#include "KisThreadSafeSignalCompressorContractTest.moc"
