/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include <set>

KisGpuLayerDocument::Result KisGpuLayerDocument::evictHistory()
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    std::set<const KisGpuEditSession::Recovery *> retained;
    for (const auto &record : m_history[m_cursor]) {
        retained.insert(record->pixels.get());
        if (record->mask) retained.insert(record->mask.get());
    }
    for (auto i = m_rasterVersions.begin(); i != m_rasterVersions.end();) {
        if (!retained.count(i->first)) i = m_rasterVersions.erase(i);
        else ++i;
    }
    return Result::Accepted;
}

bool KisGpuLayerDocument::requestHistoryCursor(qsizetype cursor)
{
    if (readiness() != Result::Accepted || cursor < 0 || cursor >= m_history.size()) return false;
    Restoration restore;
    restore.initial = false;
    restore.records = m_history[cursor];
    bool complete = true;
    for (const auto &record : restore.records) for (const auto &raster : {record->pixels, record->mask}) {
        if (!raster) continue;
        const auto found = m_rasterVersions.find(raster.get());
        if (found == m_rasterVersions.end()) complete = false;
        else restore.rasters.emplace(found->first, found->second);
    }
    if (complete) {
        m_cursor = cursor;
    } else {
        restore.request = recovery();
        restore.request.cursor = cursor;
        m_restoring = std::move(restore);
    }
    return true;
}

bool KisGpuLayerDocument::undo()
{
    return requestHistoryCursor(m_cursor - 1);
}

bool KisGpuLayerDocument::redo()
{
    return requestHistoryCursor(m_cursor + 1);
}

bool KisGpuLayerDocument::cancelHistoryRestore()
{
    if (!m_restoring || m_restoring->initial) return false;
    m_restoring.reset();
    return true;
}
