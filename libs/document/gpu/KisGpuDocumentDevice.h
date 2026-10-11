/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DOCUMENT_DEVICE_H
#define KIS_GPU_DOCUMENT_DEVICE_H
#include "KisGpuDevice.h"
#include "KisGpuLayerDocument.h"
#include <QObject>
#include <map>
#include <optional>

/** One shared GPU lifetime for independently owned document models.
 * Calls, signals and destruction use this object's submitting thread.
 * Consumers release renderers, projections, editing leases and images in the
 * direct aboutToReleaseGpu notification, before device() becomes invalid.
 * Document models survive suspension and may outlive this owner as CPU values.
 */
class KisGpuDocumentDevice : public QObject
{
    Q_OBJECT
public:
    using DocumentId = quint64;
    struct Limits {
        quint64 documentBytes = 64 * 1024 * 1024;
        quint64 recoveryBytes = 64 * 1024 * 1024;
        qsizetype retainedEdits = 32;
        quint32 maximumPending = 256;
    };
    explicit KisGpuDocumentDevice(quint64 gpuBytes = 512 * 1024 * 1024, QObject *parent = nullptr);
    ~KisGpuDocumentDevice() override;
    /** Zero denotes refusal; existing documents and identifiers remain unchanged. */
    DocumentId createDocument(Limits limits);
    DocumentId restoreDocument(Limits limits, KisGpuLayerDocument::Recovery recovery);
    bool closeDocument(DocumentId id);
    std::shared_ptr<KisGpuLayerDocument> document(DocumentId id) const;
    /** Release borrowed GPU ownership before suspension; old stores become unavailable. */
    std::shared_ptr<KisGpuTileStore> store(DocumentId id) const;
    KisGpuDevice *device() const { return m_device.get(); }
    bool isSuspended() const { return m_suspended; }
    QString lastError() const { return m_error; }
    /** CPU history and accepted working commands are preserved for every document. */
    bool suspend();
    /** Creates one replacement GPU and restores each document asynchronously through poll(). */
    bool resume();
    /** Queues history eviction until every current document can accept it. */
    void requestHistoryEviction();
    bool historyEvictionPending() const { return m_evictRequested; }
    /** Nonblocking completion collection; loss suspends every attached document. */
    void poll();
Q_SIGNALS:
    void aboutToReleaseGpu();
    void documentAboutToClose(DocumentId id);
    void gpuRestored();
private:
    struct Entry {
        Limits limits;
        std::shared_ptr<KisGpuTileStore> store;
        std::shared_ptr<KisGpuLayerDocument> model;
    };
    DocumentId open(Limits limits, std::optional<KisGpuLayerDocument::Recovery> recovery);
    bool canSuspend(const Entry &entry) const;
    const quint64 m_gpuBytes;
    std::unique_ptr<KisGpuDevice> m_device;
    std::map<DocumentId, Entry> m_documents;
    DocumentId m_nextId = 1;
    QString m_error;
    bool m_suspended = false, m_transition = false, m_evictRequested = false;
};
#endif
