/*
 *  SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisSafeBlockingQueueConnectionProxy.h"

#include <QThread>
#include <QApplication>
#include <QSemaphore>
#include <KisBusyWaitBroker.h>
#include <atomic>
#include <memory>

namespace {
struct BlockingCallState
{
    std::atomic_bool claimed = false;
    QSemaphore completed;
};
}

void KisSafeBlockingQueueConnectionProxyPrivate::passBlockingSignalSafely(SignalToFunctionProxy &destination)
{
    if (QThread::currentThread() == qApp->thread() ||
        KisBusyWaitBroker::instance()->guiThreadIsWaitingForBetterWeather()) {

        destination.start();
        return;
    }

    const auto state = std::make_shared<BlockingCallState>();
    QMetaObject::invokeMethod(&destination, [&destination, state] {
        bool expected = false;
        if (state->claimed.compare_exchange_strong(expected, true)) {
            destination.start();
            state->completed.release();
        }
    }, Qt::QueuedConnection);

    while (!state->completed.tryAcquire(1, 1)) {
        bool expected = false;
        if (KisBusyWaitBroker::instance()->guiThreadIsWaitingForBetterWeather() &&
            state->claimed.compare_exchange_strong(expected, true)) {

            destination.start();
            return;
        }
    }
}

void KisSafeBlockingQueueConnectionProxyPrivate::initProxyObject(QObject *object)
{
    object->moveToThread(qApp->thread());
}
