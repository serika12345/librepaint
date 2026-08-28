/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTemporaryResourceStorageLocator.h"

#include <KisResourceLocator.h>

namespace KisTemporaryResourceStorageLocator
{
bool hasStorage(const QString &location)
{
    return KisResourceLocator::instance()->hasStorage(location);
}

void addMemoryStorage(const QString &location)
{
    KisResourceStorageSP storage(
        new KisResourceStorage(location, KisResourceStorage::StorageType::Memory));
    KisResourceLocator::instance()->addStorage(location, storage);
}

void removeStorage(const QString &location)
{
    KisResourceLocator::instance()->removeStorage(location);
}
}
