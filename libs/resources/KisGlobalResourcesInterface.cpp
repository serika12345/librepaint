/*
 *  SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGlobalResourcesInterface.h"

#include <QGlobalStatic>
#include <QBasicMutex>
#include <QMutexLocker>

#include <kis_assert.h>

namespace KisGlobalResourcesInterfaceDetail
{
KisResourcesInterface::ResourceSourceAdapter *createModelBackedSource(const QString &type);
}

KisResourcesInterfaceSP KisGlobalResourcesInterface::instance()
{
    // TODO: implement general macro like Q_GLOBAL_STATIC()

    static QBasicAtomicInt guard = Q_BASIC_ATOMIC_INITIALIZER(QtGlobalStatic::Uninitialized);
    static KisResourcesInterfaceSP d;
    static QBasicMutex mutex;
    int x = guard.loadAcquire();
    if (Q_UNLIKELY(x >= QtGlobalStatic::Uninitialized)) {
        QMutexLocker locker(&mutex);
        if (guard.loadRelaxed() == QtGlobalStatic::Uninitialized) {
            d.reset(new KisGlobalResourcesInterface());
            static struct Cleanup {
                ~Cleanup() {
                    d.reset();
                    guard.storeRelaxed(QtGlobalStatic::Destroyed);
                }
            } cleanup;
            guard.storeRelease(QtGlobalStatic::Initialized);
        }
    }

    return d;
}

KisResourcesInterface::ResourceSourceAdapter *
KisGlobalResourcesInterface::createSourceImpl(const QString &type) const
{
    KisResourcesInterface::ResourceSourceAdapter *source =
        KisGlobalResourcesInterfaceDetail::createModelBackedSource(type);

    KIS_ASSERT(source);
    return source;
}
