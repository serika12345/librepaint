/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentDevice.h"
#include <QScopedValueRollback>
#include <limits>
#include <stdexcept>

KisGpuDocumentDevice::KisGpuDocumentDevice(quint64 bytes, QObject *parent)
    : QObject(parent), m_gpuBytes(bytes)
{
    if (!bytes) throw std::invalid_argument("GPU documents require a nonzero device budget");
}

KisGpuDocumentDevice::~KisGpuDocumentDevice()
{
    suspend();
}

KisGpuDocumentDevice::DocumentId KisGpuDocumentDevice::createDocument(Limits limits)
{
    return open(limits, {});
}

KisGpuDocumentDevice::DocumentId KisGpuDocumentDevice::restoreDocument(Limits limits,
    KisGpuLayerDocument::Recovery recovery)
{
    return open(limits, std::move(recovery));
}

KisGpuDocumentDevice::DocumentId KisGpuDocumentDevice::open(Limits limits,
    std::optional<KisGpuLayerDocument::Recovery> recovery)
{
    if (m_transition || m_suspended) return 0;
    if (!limits.documentBytes || !limits.recoveryBytes || !limits.maximumPending || limits.retainedEdits < 0
        || m_nextId == std::numeric_limits<DocumentId>::max()
        || (recovery && !KisGpuLayerDocument::validRecovery(*recovery, limits.retainedEdits, limits.recoveryBytes))) {
        m_error = QStringLiteral("Invalid GPU document limits or recovery values");
        return 0;
    }
    QScopedValueRollback<bool> transition(m_transition, true);
    try {
        auto candidate = m_device ? nullptr : std::make_unique<KisGpuDevice>(0, false, m_gpuBytes);
        auto gpu = m_device ? m_device.get() : candidate.get();
        if (!gpu->available()) { m_error = QStringLiteral("GPU device is unavailable"); return 0; }
        auto storage = std::make_shared<KisGpuTileStore>(*gpu, limits.documentBytes, limits.maximumPending);
        auto model = recovery
            ? std::make_shared<KisGpuLayerDocument>(storage, limits.retainedEdits, std::move(*recovery), limits.recoveryBytes)
            : std::make_shared<KisGpuLayerDocument>(storage, limits.retainedEdits, limits.recoveryBytes);
        const auto id = m_nextId;
        m_documents.emplace(id, Entry{limits, std::move(storage), std::move(model)});
        if (candidate) m_device = std::move(candidate);
        ++m_nextId;
        m_error.clear();
        Q_EMIT documentsChanged();
        return id;
    } catch (const std::exception &error) {
        m_error = QString::fromUtf8(error.what());
        return 0;
    }
}

std::shared_ptr<KisGpuLayerDocument> KisGpuDocumentDevice::document(DocumentId id) const
{
    const auto found = m_documents.find(id);
    return found == m_documents.end() ? nullptr : found->second.model;
}

std::shared_ptr<KisGpuTileStore> KisGpuDocumentDevice::store(DocumentId id) const
{
    const auto found = m_documents.find(id);
    return found == m_documents.end() ? nullptr : found->second.store;
}

bool KisGpuDocumentDevice::canSuspend(const Entry &entry) const
{
    return KisGpuLayerDocument::validRecovery(entry.model->recovery(), entry.limits.retainedEdits,
                                              entry.limits.recoveryBytes);
}

bool KisGpuDocumentDevice::closeDocument(DocumentId id)
{
    if (m_transition) return false;
    const auto found = m_documents.find(id);
    if (found == m_documents.end() || !canSuspend(found->second)) return false;
    QScopedValueRollback<bool> transition(m_transition, true);
    Q_EMIT documentAboutToClose(id);
    if (found->second.model->suspend() != KisGpuLayerDocument::Result::Accepted) return false;
    m_documents.erase(found);
    if (m_documents.empty() && m_device) {
        Q_EMIT aboutToReleaseGpu();
        m_device->destroy();
        m_device.reset();
        m_evictRequested = false;
    }
    Q_EMIT documentsChanged();
    return true;
}

bool KisGpuDocumentDevice::suspend()
{
    if (m_transition) return false;
    if (m_suspended) return true;
    for (const auto &[id, entry] : m_documents) {
        Q_UNUSED(id);
        if (!canSuspend(entry)) { m_error = QStringLiteral("Cannot preserve GPU document recovery values"); return false; }
    }
    QScopedValueRollback<bool> transition(m_transition, true);
    if (m_device) Q_EMIT aboutToReleaseGpu();
    for (auto &[id, entry] : m_documents) {
        Q_UNUSED(id);
        if (entry.model->suspend() != KisGpuLayerDocument::Result::Accepted) return false;
        entry.store.reset();
    }
    if (m_device) m_device->destroy();
    m_device.reset();
    m_suspended = true;
    m_evictRequested = false;
    m_error.clear();
    return true;
}

bool KisGpuDocumentDevice::resume()
{
    if (m_transition) return false;
    if (!m_suspended) return true;
    if (m_documents.empty()) { m_suspended = false; return true; }
    QScopedValueRollback<bool> transition(m_transition, true);
    try {
        auto device = std::make_unique<KisGpuDevice>(0, false, m_gpuBytes);
        std::map<DocumentId, std::shared_ptr<KisGpuTileStore>> stores;
        for (const auto &[id, entry] : m_documents)
            stores.emplace(id, std::make_shared<KisGpuTileStore>(*device, entry.limits.documentBytes, entry.limits.maximumPending));
        for (auto &[id, entry] : m_documents) {
            if (entry.model->resume(stores.at(id)) != KisGpuLayerDocument::Result::Accepted) {
                for (auto &[unused, rollback] : m_documents) { Q_UNUSED(unused); rollback.model->suspend(); }
                m_error = QStringLiteral("Cannot resume GPU documents on the replacement device");
                return false;
            }
        }
        m_device = std::move(device);
        for (auto &[id, entry] : m_documents) entry.store = std::move(stores.at(id));
        m_suspended = false;
        m_error.clear();
        Q_EMIT gpuRestored();
        return true;
    } catch (const std::exception &error) {
        m_error = QString::fromUtf8(error.what());
        return false;
    }
}

void KisGpuDocumentDevice::requestHistoryEviction()
{
    if (!m_suspended && !m_transition) m_evictRequested = true;
}

void KisGpuDocumentDevice::poll()
{
    if (m_transition || !m_device) return;
    if (!m_device->available()) { suspend(); return; }
    bool evicted = true;
    for (auto &[id, entry] : m_documents) {
        Q_UNUSED(id);
        entry.model->poll();
        if (m_evictRequested && entry.model->evictHistory() != KisGpuLayerDocument::Result::Accepted) evicted = false;
    }
    if (evicted) m_evictRequested = false;
}
