/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPlatformPredictionBuffer.h"

#include <QMutexLocker>

#include <algorithm>

KisPlatformPredictionBuffer &KisPlatformPredictionBuffer::instance()
{
    static KisPlatformPredictionBuffer buffer;
    return buffer;
}

void KisPlatformPredictionBuffer::push(const KisPlatformPredictionEntry &entry)
{
    QMutexLocker locker(&m_mutex);

    bool replaced = false;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).keyMs == entry.keyMs) {
            m_entries[i] = entry;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        m_entries.append(entry);
    }

    if (m_entries.isEmpty()) return;

    quint64 newestKey = m_entries.constFirst().keyMs;
    for (const KisPlatformPredictionEntry &current : m_entries) {
        newestKey = std::max(newestKey, current.keyMs);
    }

    int firstKept = 0;
    while (firstKept < m_entries.size()
           && newestKey - m_entries.at(firstKept).keyMs > keepMs) {
        ++firstKept;
    }
    if (firstKept > 0) {
        m_entries.remove(0, firstKept);
    }

    while (m_entries.size() > qsizetype(maxEntries)) {
        m_entries.removeFirst();
    }
}

bool KisPlatformPredictionBuffer::find(quint64 keyMs, KisPlatformPredictionEntry *entry) const
{
    QMutexLocker locker(&m_mutex);

    for (const KisPlatformPredictionEntry &current : m_entries) {
        if (current.keyMs == keyMs) {
            if (entry) {
                *entry = current;
            }
            return true;
        }
    }
    return false;
}

void KisPlatformPredictionBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    m_entries.clear();
}

void KisPlatformPredictionBuffer::setSupplierInstalled(bool installed)
{
    QMutexLocker locker(&m_mutex);
    m_supplierInstalled.store(installed);
}

bool KisPlatformPredictionBuffer::supplierInstalled() const
{
    QMutexLocker locker(&m_mutex);
    return m_supplierInstalled.load();
}
