/*
 *  SPDX-FileCopyrightText: 2023 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTemporaryResourceStorageLock.h"
#include "KisTemporaryResourceStorageLocator.h"

#include <kis_assert.h>

KisTemporaryResourceStorageLockAdapter::KisTemporaryResourceStorageLockAdapter(const QString &temporaryStorageLocationTemplate)
    : m_temporaryStorageLocationTemplate(temporaryStorageLocationTemplate)
{
}

void KisTemporaryResourceStorageLockAdapter::lock()
{
    int counter = 0;
    QString storageLocation = m_temporaryStorageLocationTemplate;

    while (KisTemporaryResourceStorageLocator::hasStorage(storageLocation)) {
        storageLocation = QString("%1_%2").arg(m_temporaryStorageLocationTemplate).arg(counter++);
    }

    KisTemporaryResourceStorageLocator::addMemoryStorage(storageLocation);

    m_temporaryStorageLocation = storageLocation;
}

bool KisTemporaryResourceStorageLockAdapter::try_lock()
{
    lock();
    return true;
}

void KisTemporaryResourceStorageLockAdapter::unlock()
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(!m_temporaryStorageLocation.isEmpty());
    KisTemporaryResourceStorageLocator::removeStorage(m_temporaryStorageLocation);
    m_temporaryStorageLocation.clear();
}

QString KisTemporaryResourceStorageLockAdapter::storageLocation() const
{
    return m_temporaryStorageLocation;
}
