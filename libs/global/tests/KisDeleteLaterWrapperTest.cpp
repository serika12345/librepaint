/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDeleteLaterWrapper.h>

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QTest>
#include <QThread>

#include <utility>

namespace
{

class WorkerContext
{
public:
    WorkerContext()
    {
        m_context.moveToThread(&m_thread);
        m_thread.start();
    }

    ~WorkerContext()
    {
        const bool returnedToGuiThread = invoke([this] {
            m_context.moveToThread(qApp->thread());
        });
        Q_ASSERT(returnedToGuiThread);

        m_thread.quit();
        const bool stopped = m_thread.wait();
        Q_ASSERT(stopped);
        Q_UNUSED(stopped);
    }

    template<typename Function>
    bool invoke(Function &&function)
    {
        return QMetaObject::invokeMethod(&m_context, std::forward<Function>(function), Qt::BlockingQueuedConnection);
    }

private:
    QThread m_thread;
    QObject m_context;
};

class TrackedValue
{
public:
    explicit TrackedValue(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~TrackedValue()
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

} // namespace

class KisDeleteLaterWrapperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directMoveToGuiThread();
    void factoryMovesValueWrapperToGuiThread();
    void pointerWrapperDeletesOwnedValue();
};

void KisDeleteLaterWrapperTest::directMoveToGuiThread()
{
    WorkerContext worker;
    QObject *object = nullptr;
    QThread *creationThread = nullptr;

    const bool invoked = worker.invoke([&] {
        object = new QObject;
        creationThread = object->thread();
        KisDeleteLaterWrapperPrivate::moveToGuiThread(object);
    });

    QVERIFY(invoked);
    QVERIFY(object);
    QVERIFY(creationThread != qApp->thread());
    QCOMPARE(object->thread(), qApp->thread());

    delete object;
}

void KisDeleteLaterWrapperTest::factoryMovesValueWrapperToGuiThread()
{
    WorkerContext worker;
    KisDeleteLaterWrapper<QString> *wrapper = nullptr;

    const bool invoked = worker.invoke([&] {
        wrapper = makeKisDeleteLaterWrapper(QStringLiteral("owned value"));
    });

    QVERIFY(invoked);
    QVERIFY(wrapper);
    QCOMPARE(wrapper->thread(), qApp->thread());

    delete wrapper;
}

void KisDeleteLaterWrapperTest::pointerWrapperDeletesOwnedValue()
{
    WorkerContext worker;
    bool destroyed = false;
    KisDeleteLaterWrapper<TrackedValue *> *wrapper = nullptr;

    const bool invoked = worker.invoke([&] {
        wrapper = makeKisDeleteLaterWrapper(new TrackedValue(&destroyed));
    });

    QVERIFY(invoked);
    QVERIFY(wrapper);
    QCOMPARE(wrapper->thread(), qApp->thread());
    QVERIFY(!destroyed);

    QPointer<QObject> wrapperGuard(wrapper);
    wrapper->deleteLater();
    QCoreApplication::sendPostedEvents(wrapper, QEvent::DeferredDelete);

    QVERIFY(wrapperGuard.isNull());
    QVERIFY(destroyed);
}

QTEST_MAIN(KisDeleteLaterWrapperTest)

#include "KisDeleteLaterWrapperTest.moc"
