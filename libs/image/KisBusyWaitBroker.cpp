/*
 *  SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisBusyWaitBroker.h"

#include <QGlobalStatic>
#include <QMutex>
#include <QMutexLocker>

#include <QThread>
#include <QCoreApplication>
#include <functional>
#include <qhash.h>

#include "kis_assert.h"
#include "kis_image.h"
#include "kis_types.h"


Q_GLOBAL_STATIC(KisBusyWaitBroker, s_instance)


struct KisBusyWaitBroker::Private
{
    QMutex lock;
    QHash<KisImage*, int> waitingOnImages;
    int guiThreadLockCount = 0;

    std::function<void(KisImageSP)> feedbackCallback;
};


KisBusyWaitBroker::KisBusyWaitBroker()
    : m_d(new Private)
{
}

KisBusyWaitBroker::~KisBusyWaitBroker()
{
}

KisBusyWaitBroker *KisBusyWaitBroker::instance()
{
    return s_instance;
}

void KisBusyWaitBroker::notifyWaitOnImageStarted(KisImage* image)
{
    const auto *application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread()) return;

    bool needsStartCallback = false;

    {
        QMutexLocker l(&m_d->lock);

        m_d->guiThreadLockCount++;
        m_d->waitingOnImages[image]++;

        needsStartCallback = m_d->waitingOnImages[image] == 1;
    }

    if (m_d->feedbackCallback && needsStartCallback && image->refCount()) {
        m_d->feedbackCallback(image);
    }
}

void KisBusyWaitBroker::notifyWaitOnImageEnded(KisImage* image)
{
    const auto *application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread()) return;

    {
        QMutexLocker l(&m_d->lock);
        m_d->guiThreadLockCount--;

        m_d->waitingOnImages[image]--;
        KIS_SAFE_ASSERT_RECOVER_NOOP(m_d->waitingOnImages[image] >= 0);

        if (m_d->waitingOnImages[image] == 0) {
            m_d->waitingOnImages.remove(image);
        }
    }
}

void KisBusyWaitBroker::notifyGeneralWaitStarted()
{
    const auto *application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread()) return;

    QMutexLocker l(&m_d->lock);
    m_d->guiThreadLockCount++;
}

void KisBusyWaitBroker::notifyGeneralWaitEnded()
{
    const auto *application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread()) return;

    QMutexLocker l(&m_d->lock);
    m_d->guiThreadLockCount--;
}

void KisBusyWaitBroker::setFeedbackCallback(std::function<void (KisImageSP)> callback)
{
    m_d->feedbackCallback = callback;
}

bool KisBusyWaitBroker::guiThreadIsWaitingForBetterWeather() const
{
    QMutexLocker l(&m_d->lock);
    return m_d->guiThreadLockCount;
}

