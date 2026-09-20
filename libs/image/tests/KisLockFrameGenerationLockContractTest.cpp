/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLockFrameGenerationLock.h"
#include "kis_image_animation_interface.h"

#include <QTest>
#include <QVector>

namespace
{

enum class FrameGenerationCall {
    TryLock,
    Lock,
    Unlock
};

struct FrameGenerationInvocation {
    KisImageAnimationInterface *interface;
    FrameGenerationCall call;
};

QVector<FrameGenerationInvocation> invocations;
bool tryLockResult = false;

} // namespace

struct KisImageAnimationInterface::Private {
};

KisImageAnimationInterface::KisImageAnimationInterface(KisImage *image)
    : QObject(nullptr)
    , m_d(new Private)
{
    Q_UNUSED(image);
}

KisImageAnimationInterface::~KisImageAnimationInterface() = default;

bool KisImageAnimationInterface::tryLockFrameGeneration()
{
    invocations.append({this, FrameGenerationCall::TryLock});
    return tryLockResult;
}

void KisImageAnimationInterface::lockFrameGeneration()
{
    invocations.append({this, FrameGenerationCall::Lock});
}

void KisImageAnimationInterface::unlockFrameGeneration()
{
    invocations.append({this, FrameGenerationCall::Unlock});
}

void KisImageAnimationInterface::switchCurrentTimeAsync(int frameId, SwitchTimeAsyncFlags options)
{
    Q_UNUSED(frameId);
    Q_UNUSED(options);
}

void KisImageAnimationInterface::setDocumentRangeStartFrame(int column)
{
    Q_UNUSED(column);
}

void KisImageAnimationInterface::setDocumentRangeEndFrame(int column)
{
    Q_UNUSED(column);
}

void KisImageAnimationInterface::setFramerate(int fps)
{
    Q_UNUSED(fps);
}

class KisLockFrameGenerationLockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void retainsBorrowedAnimationInterface();
    void returnsTryLockResultUnchanged();
    void delegatesExclusiveFrameGenerationControlExactlyOnce();
};

void KisLockFrameGenerationLockContractTest::init()
{
    invocations.clear();
    tryLockResult = false;
}

void KisLockFrameGenerationLockContractTest::retainsBorrowedAnimationInterface()
{
    KisImageAnimationInterface firstInterface(nullptr);
    KisImageAnimationInterface secondInterface(nullptr);
    KisImageAnimationInterface *borrowedInterface = &firstInterface;
    KisLockFrameGenerationLockAdapter adapter(borrowedInterface);

    borrowedInterface = &secondInterface;
    adapter.lock();

    QCOMPARE(invocations.size(), 1);
    QCOMPARE(invocations.at(0).interface, &firstInterface);
    QCOMPARE(invocations.at(0).call, FrameGenerationCall::Lock);
    QCOMPARE(borrowedInterface, &secondInterface);
}

void KisLockFrameGenerationLockContractTest::returnsTryLockResultUnchanged()
{
    KisImageAnimationInterface interface(nullptr);
    KisLockFrameGenerationLockAdapter adapter(&interface);

    QVERIFY(!adapter.try_lock());
    tryLockResult = true;
    QVERIFY(adapter.try_lock());

    QCOMPARE(invocations.size(), 2);
    QCOMPARE(invocations.at(0).interface, &interface);
    QCOMPARE(invocations.at(0).call, FrameGenerationCall::TryLock);
    QCOMPARE(invocations.at(1).interface, &interface);
    QCOMPARE(invocations.at(1).call, FrameGenerationCall::TryLock);
}

void KisLockFrameGenerationLockContractTest::delegatesExclusiveFrameGenerationControlExactlyOnce()
{
    KisImageAnimationInterface interface(nullptr);
    KisLockFrameGenerationLockAdapter adapter(&interface);

    adapter.lock();
    adapter.unlock();

    QCOMPARE(invocations.size(), 2);
    QCOMPARE(invocations.at(0).interface, &interface);
    QCOMPARE(invocations.at(0).call, FrameGenerationCall::Lock);
    QCOMPARE(invocations.at(1).interface, &interface);
    QCOMPARE(invocations.at(1).call, FrameGenerationCall::Unlock);
}

QTEST_GUILESS_MAIN(KisLockFrameGenerationLockContractTest)

#include "KisLockFrameGenerationLockContractTest.moc"
