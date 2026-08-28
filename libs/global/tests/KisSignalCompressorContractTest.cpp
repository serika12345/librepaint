/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_signal_compressor.h"
#include "kis_signal_compressor_with_param.h"

#include <QSignalSpy>
#include <QTest>

#include <type_traits>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisSignalCompressorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionConfigurationAndImmediateMode();
    void deferredModesAndIdleCallbackDeliver();
    void functionAndSignalProxiesForwardCalls();
    void parameterCompressorDeliversLatestValueAndStops();
};

void KisSignalCompressorContractTest::constructionConfigurationAndImmediateMode()
{
    static_assert(KisSignalCompressor::POSTPONE != KisSignalCompressor::FIRST_ACTIVE_POSTPONE_NEXT);
    static_assert(KisSignalCompressor::FIRST_ACTIVE_POSTPONE_NEXT != KisSignalCompressor::FIRST_ACTIVE);
    static_assert(KisSignalCompressor::FIRST_ACTIVE != KisSignalCompressor::FIRST_INACTIVE);
    static_assert(KisSignalCompressor::FIRST_INACTIVE != KisSignalCompressor::UNDEFINED);
    static_assert(KisSignalCompressor::PRECISE_INTERVAL != KisSignalCompressor::ADDITIVE_INTERVAL);

    KisSignalCompressor compressor;
    QCOMPARE(compressor.delay(), 0);
    QVERIFY(!compressor.isActive());
    compressor.setMode(KisSignalCompressor::FIRST_ACTIVE);
    compressor.setDelay(20);
    QCOMPARE(compressor.delay(), 20);

    QSignalSpy spy(&compressor, &KisSignalCompressor::timeout);
    compressor.start();
    QCOMPARE(spy.count(), 1);
    QVERIFY(!compressor.isActive());

    compressor.start();
    QVERIFY(compressor.isActive());
    compressor.stop();
    QVERIFY(!compressor.isActive());

    QObject parent;
    KisSignalCompressor parented(25, KisSignalCompressor::FIRST_INACTIVE, &parent);
    QCOMPARE(parented.parent(), &parent);
    QCOMPARE(parented.delay(), 25);

    KisSignalCompressor additive(30,
                                 KisSignalCompressor::FIRST_ACTIVE,
                                 KisSignalCompressor::ADDITIVE_INTERVAL);
    QCOMPARE(additive.delay(), 30);
}

void KisSignalCompressorContractTest::deferredModesAndIdleCallbackDeliver()
{
    KisSignalCompressor postponed(10, KisSignalCompressor::POSTPONE);
    QSignalSpy postponedSpy(&postponed, &KisSignalCompressor::timeout);
    postponed.start();
    QVERIFY(postponed.isActive());
    QTRY_COMPARE_WITH_TIMEOUT(postponedSpy.count(), 1, 100);
    postponed.stop();

    KisSignalCompressor inactive(10, KisSignalCompressor::FIRST_INACTIVE);
    QSignalSpy inactiveSpy(&inactive, &KisSignalCompressor::timeout);
    inactive.start();
    QCOMPARE(inactiveSpy.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(inactiveSpy.count(), 1, 100);
    inactive.stop();

    KisSignalCompressor firstAndPostponed(10,
                                          KisSignalCompressor::FIRST_ACTIVE_POSTPONE_NEXT);
    QSignalSpy firstAndPostponedSpy(&firstAndPostponed, &KisSignalCompressor::timeout);
    firstAndPostponed.start();
    QCOMPARE(firstAndPostponedSpy.count(), 1);
    firstAndPostponed.start();
    QTRY_COMPARE_WITH_TIMEOUT(firstAndPostponedSpy.count(), 2, 100);
    firstAndPostponed.stop();

    int idleChecks = 0;
    KisSignalCompressor idle(1000, KisSignalCompressor::POSTPONE);
    idle.setDelay([&idleChecks] {
        ++idleChecks;
        return true;
    }, 1, 1000);
    QSignalSpy idleSpy(&idle, &KisSignalCompressor::timeout);
    idle.start();
    QTRY_COMPARE_WITH_TIMEOUT(idleSpy.count(), 1, 100);
    QVERIFY(idleChecks > 0);
    idle.stop();
}

void KisSignalCompressorContractTest::functionAndSignalProxiesForwardCalls()
{
    static_assert(std::is_same_v<SignalToFunctionProxy::TrivialFunction,
                                 std::function<void()>>);

    int calls = 0;
    SignalToFunctionProxy functionProxy([&calls] {
        ++calls;
    });
    functionProxy.start();
    QCOMPARE(calls, 1);

    FunctionToSignalProxy signalProxy;
    QSignalSpy spy(&signalProxy, &FunctionToSignalProxy::timeout);
    signalProxy.start();
    QCOMPARE(spy.count(), 1);
}

void KisSignalCompressorContractTest::parameterCompressorDeliversLatestValueAndStops()
{
    using Compressor = KisSignalCompressorWithParam<int>;
    static_assert(std::is_same_v<Compressor::CallbackFunction, std::function<void(int)>>);

    QVector<int> delivered;
    {
        Compressor compressor(10,
                              [&delivered](int value) {
                                  delivered.append(value);
                              },
                              KisSignalCompressor::POSTPONE);
        compressor.setDelay(10);
        compressor.start(3);
        compressor.start(7);
        QVERIFY(compressor.isActive());
        QTRY_COMPARE_WITH_TIMEOUT(delivered, QVector<int>({7}), 100);
        compressor.stop();
        QVERIFY(!compressor.isActive());
    }

    QTest::qWait(20);
    QCOMPARE(delivered, QVector<int>({7}));

    int immediateValue = 0;
    Compressor immediate(10, [&immediateValue](int value) {
        immediateValue = value;
    });
    immediate.start(11);
    QCOMPARE(immediateValue, 11);
    immediate.stop();
}

QTEST_GUILESS_MAIN(KisSignalCompressorContractTest)

#include "KisSignalCompressorContractTest.moc"
