/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBusyWaitBroker.h"
#include "KisSafeBlockingQueueConnectionProxy.h"

#include <QApplication>
#include <QSemaphore>
#include <QTest>
#include <QThread>
#include <QVector>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <type_traits>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{
class GeneralWaitScope
{
public:
    GeneralWaitScope()
    {
        KisBusyWaitBroker::instance()->notifyGeneralWaitStarted();
    }

    ~GeneralWaitScope()
    {
        KisBusyWaitBroker::instance()->notifyGeneralWaitEnded();
    }
};
} // namespace

class KisSafeBlockingQueueConnectionProxyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initMovesBorrowedObjectToApplicationThread();
    void typedAndVoidStartsDispatchSynchronouslyOnGuiThread();
    void workerStartBlocksUntilGuiCallbacksComplete();
    void guiWaitLetsWorkerDispatchDirectlyExactlyOnce();
};

void KisSafeBlockingQueueConnectionProxyContractTest::initMovesBorrowedObjectToApplicationThread()
{
    QThread workerThread;
    workerThread.start();

    QObject object;
    object.moveToThread(&workerThread);
    QVERIFY(QMetaObject::invokeMethod(
        &object,
        [&object] {
            KisSafeBlockingQueueConnectionProxyPrivate::initProxyObject(&object);
        },
        Qt::BlockingQueuedConnection));

    QCOMPARE(object.thread(), QApplication::instance()->thread());
    workerThread.quit();
    QVERIFY(workerThread.wait(500));
}

void KisSafeBlockingQueueConnectionProxyContractTest::typedAndVoidStartsDispatchSynchronouslyOnGuiThread()
{
    using TypedProxy = KisSafeBlockingQueueConnectionProxy<QString>;
    using VoidProxy = KisSafeBlockingQueueConnectionProxy<void>;
    static_assert(std::is_constructible_v<TypedProxy, std::function<void(QString)>>);
    static_assert(std::is_constructible_v<VoidProxy, std::function<void()>>);

    QVector<QString> values;
    QVector<QThread *> callbackThreads;
    TypedProxy typedProxy([&](const QString &value) {
        values.append(value);
        callbackThreads.append(QThread::currentThread());
    });

    int voidCalls = 0;
    VoidProxy voidProxy([&] {
        ++voidCalls;
        callbackThreads.append(QThread::currentThread());
    });

    typedProxy.start(QStringLiteral("first"));
    typedProxy.start(QStringLiteral("second"));
    voidProxy.start();

    QCOMPARE(values, QVector<QString>({QStringLiteral("first"), QStringLiteral("second")}));
    QCOMPARE(voidCalls, 1);
    QCOMPARE(callbackThreads.size(), 3);
    for (QThread *thread : callbackThreads) {
        QCOMPARE(thread, QApplication::instance()->thread());
    }
}

void KisSafeBlockingQueueConnectionProxyContractTest::workerStartBlocksUntilGuiCallbacksComplete()
{
    QVector<int> values;
    QVector<QThread *> callbackThreads;
    KisSafeBlockingQueueConnectionProxy<int> proxy([&](int value) {
        values.append(value);
        callbackThreads.append(QThread::currentThread());
    });

    QSemaphore entered;
    std::atomic_bool returned = false;
    std::thread worker([&] {
        entered.release();
        proxy.start(13);
        proxy.start(29);
        returned.store(true);
    });

    QVERIFY(entered.tryAcquire(1, 500));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    QVERIFY(!returned.load());

    QTRY_COMPARE_WITH_TIMEOUT(values.size(), 2, 500);
    QTRY_VERIFY_WITH_TIMEOUT(returned.load(), 500);
    worker.join();

    QCOMPARE(values, QVector<int>({13, 29}));
    QCOMPARE(callbackThreads.size(), 2);
    for (QThread *thread : callbackThreads) {
        QCOMPARE(thread, QApplication::instance()->thread());
    }
}

void KisSafeBlockingQueueConnectionProxyContractTest::guiWaitLetsWorkerDispatchDirectlyExactlyOnce()
{
    std::atomic_int calls = 0;
    std::thread::id callbackThread;
    KisSafeBlockingQueueConnectionProxy<void> proxy([&] {
        callbackThread = std::this_thread::get_id();
        calls.fetch_add(1);
    });

    GeneralWaitScope generalWait;
    QSemaphore finished;
    std::thread::id workerThread;
    std::thread worker([&] {
        workerThread = std::this_thread::get_id();
        proxy.start();
        finished.release();
    });

    const bool completedWithoutGuiEvents = finished.tryAcquire(1, 500);
    worker.join();

    QVERIFY(completedWithoutGuiEvents);
    QCOMPARE(calls.load(), 1);
    QCOMPARE(callbackThread, workerThread);
}

QTEST_MAIN(KisSafeBlockingQueueConnectionProxyContractTest)

#include "KisSafeBlockingQueueConnectionProxyContractTest.moc"
