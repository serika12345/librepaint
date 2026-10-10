/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DOCUMENT_READBACK_H
#define KIS_GPU_DOCUMENT_READBACK_H

#include "KisGpuLayerDocument.h"

/** Explicit asynchronous CPU snapshot of a frozen, committed layer column.
 * The job retains its concrete store and versions. Calls and destruction use
 * the submitting thread; the completed CPU value may outlive every GPU owner.
 * Destruction cancels pending publication. No partial result is exposed.
 */
class KisGpuDocumentReadback
{
public:
    using Status = KisGpuTileStore::Status;
    enum class Error { None, InvalidDocument, InvalidVersion, BudgetExceeded, DeviceLost, ReadFailed };
    KisGpuDocumentReadback(std::shared_ptr<KisGpuTileStore> store,
                          QVector<KisGpuLayerDocument::Layer> layers,
                          KisGpuLayerDocument::LayerId nextId,
                          quint64 maximumRecoveryBytes = 64 * 1024 * 1024);
    KisGpuDocumentReadback(const KisGpuDocumentReadback &) = delete;
    KisGpuDocumentReadback &operator=(const KisGpuDocumentReadback &) = delete;
    void poll();
    Status status() const { return m_status; }
    Error error() const { return m_error; }
    std::optional<KisGpuLayerDocument::Recovery> result() const { return m_result; }
    /** Logical CPU checkpoint capacity reserved for this snapshot. */
    quint64 reservedRecoveryBytes() const { return m_reservedBytes; }

private:
    struct Source {
        KisGpuTileStore::Version version;
        std::shared_ptr<KisGpuEditSession::Recovery> checkpoint;
        QVector<QPoint> coordinates;
        qsizetype cursor = 0;
    };
    void fail(Error error);
    std::shared_ptr<KisGpuTileStore> m_store;
    QVector<Source> m_sources;
    KisGpuLayerDocument::Recovery m_staging;
    std::optional<KisGpuLayerDocument::Recovery> m_result;
    KisGpuTileStore::Readback m_latest;
    qsizetype m_index = 0;
    quint64 m_reservedBytes = 0;
    bool m_waiting = false;
    Status m_status = Status::Pending;
    Error m_error = Error::None;
};

#endif
