/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISTEMPORARYRESOURCESTORAGELOCATOR_H
#define KISTEMPORARYRESOURCESTORAGELOCATOR_H

#include <QString>

namespace KisTemporaryResourceStorageLocator
{
bool hasStorage(const QString &location);
void addMemoryStorage(const QString &location);
void removeStorage(const QString &location);
}

#endif // KISTEMPORARYRESOURCESTORAGELOCATOR_H
