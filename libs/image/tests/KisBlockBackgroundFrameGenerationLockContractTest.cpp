/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBlockBackgroundFrameGenerationLock.h"
#include "kis_image_animation_interface.h"

#include <QTest>
#include <QVector>

namespace
{

enum class BackgroundGenerationCall {
    Block,
    Unblock
};

struct BackgroundGenerationInvocation {
    KisImageAnimationInterface *interface;
    BackgroundGenerationCall call;
};

QVector<BackgroundGenerationInvocation> invocations;

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

void KisImageAnimationInterface::blockBackgroundFrameGeneration()
{
    invocations.append({this, BackgroundGenerationCall::Block});
}

void KisImageAnimationInterface::unblockBackgroundFrameGeneration()
{
    invocations.append({this, BackgroundGenerationCall::Unblock});
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

class KisBlockBackgroundFrameGenerationLockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void retainsBorrowedAnimationInterface();
    void delegatesBackgroundGenerationControlExactlyOnce();
};

void KisBlockBackgroundFrameGenerationLockContractTest::init()
{
    invocations.clear();
}

void KisBlockBackgroundFrameGenerationLockContractTest::retainsBorrowedAnimationInterface()
{
    KisImageAnimationInterface firstInterface(nullptr);
    KisImageAnimationInterface secondInterface(nullptr);
    KisImageAnimationInterface *borrowedInterface = &firstInterface;
    KisBlockBackgroundFrameGenerationLockAdapter adapter(borrowedInterface);

    borrowedInterface = &secondInterface;
    adapter.lock();

    QCOMPARE(invocations.size(), 1);
    QCOMPARE(invocations.at(0).interface, &firstInterface);
    QCOMPARE(invocations.at(0).call, BackgroundGenerationCall::Block);
    QCOMPARE(borrowedInterface, &secondInterface);
}

void KisBlockBackgroundFrameGenerationLockContractTest::delegatesBackgroundGenerationControlExactlyOnce()
{
    KisImageAnimationInterface interface(nullptr);
    KisBlockBackgroundFrameGenerationLockAdapter adapter(&interface);

    adapter.lock();
    adapter.unlock();

    QCOMPARE(invocations.size(), 2);
    QCOMPARE(invocations.at(0).interface, &interface);
    QCOMPARE(invocations.at(0).call, BackgroundGenerationCall::Block);
    QCOMPARE(invocations.at(1).interface, &interface);
    QCOMPARE(invocations.at(1).call, BackgroundGenerationCall::Unblock);
}

QTEST_GUILESS_MAIN(KisBlockBackgroundFrameGenerationLockContractTest)

#include "KisBlockBackgroundFrameGenerationLockContractTest.moc"
