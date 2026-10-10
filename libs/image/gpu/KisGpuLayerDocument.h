/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_LAYER_DOCUMENT_H
#define KIS_GPU_LAYER_DOCUMENT_H

#include "KisGpuEditSession.h"
#include <QString>
#include <map>

/** Ordered raster layers and one bounded document-wide Undo/Redo sequence.
 * Calls and destruction use the store's submitting thread. Editing leases keep
 * their concrete store alive; tokens become stale after cancellation or close.
 */
class KisGpuLayerDocument
{
public:
    using LayerId = quint64;
    enum class Result { Accepted, Busy, Stale, InvalidLayer, InvalidInput, RecoveryBudgetExceeded, GpuRejected };
    enum class State { Idle, Editing, Committing, Failed, Restoring, Compacting, Suspended };
    enum class Target { Pixels, Mask };
    struct Layer {
        LayerId id = 0;
        QString name;
        KisGpuTileStore::Layer paint;
    };
    struct LayerResult { Result result = Result::InvalidInput; LayerId id = 0; };
    struct Editing {
        Result result = Result::Busy;
        std::shared_ptr<KisGpuEditSession> session;
        KisGpuEditSession::Token token;
    };
    using RasterRecovery = std::shared_ptr<const KisGpuEditSession::Recovery>;
    struct RecoveryLayer {
        LayerId id = 0;
        QString name;
        quint8 opacity = 255;
        KisGpuTileStore::CompositeOp operation = KisGpuTileStore::CompositeOp::Over;
        RasterRecovery pixels = std::make_shared<const KisGpuEditSession::Recovery>();
        RasterRecovery mask;
    };
    using RecoveryFrame = QVector<std::shared_ptr<const RecoveryLayer>>;
    struct RecoveryWorking {
        std::shared_ptr<const RecoveryLayer> base;
        Target target = Target::Pixels;
        KisGpuEditSession::Recovery data;
        bool completed = false;
    };
    /** CPU-only immutable layer records, global history and optional unfinished edit. */
    struct Recovery {
        QVector<RecoveryFrame> history;
        qsizetype cursor = 0;
        LayerId nextId = 1;
        std::optional<RecoveryWorking> working;
    };
    /** Null store or a budget too small for the empty history throws std::invalid_argument. */
    KisGpuLayerDocument(std::shared_ptr<KisGpuTileStore> store, qsizetype retainedEdits,
                        quint64 maximumRecoveryBytes = 64 * 1024 * 1024);
    /** Restores asynchronously; malformed history or CPU budget throws std::invalid_argument. */
    KisGpuLayerDocument(std::shared_ptr<KisGpuTileStore> store, qsizetype retainedEdits, Recovery recovery,
                        quint64 maximumRecoveryBytes = 64 * 1024 * 1024);
    ~KisGpuLayerDocument();
    KisGpuLayerDocument(const KisGpuLayerDocument &) = delete;
    KisGpuLayerDocument &operator=(const KisGpuLayerDocument &) = delete;

    LayerResult addLayer(QString name, qsizetype index = -1);
    Result removeLayer(LayerId id);
    Result moveLayer(LayerId id, qsizetype index);
    Result renameLayer(LayerId id, QString name);
    Result setLayerProperties(LayerId id, quint8 opacity, KisGpuTileStore::CompositeOp operation);
    Result clearMask(LayerId id);
    /** One edit at a time; adopts the existing GPU head and CPU record without transfer or replay. */
    Editing begin(LayerId id, Target target = Target::Pixels);
    Editing currentEditing() const;
    Recovery recovery() const;
    /** CPU-only structural validation, usable before persistence or GPU creation. */
    static bool validRecovery(const Recovery &recovery, qsizetype retainedEdits, quint64 maximumRecoveryBytes);
    /** Failure of the current restoration; None after successful adoption. */
    KisGpuTileStore::Error recoveryError() const;
    Result commit(const KisGpuEditSession::Token &token);
    Result cancel(const KisGpuEditSession::Token &token);
    void poll();
    /** Accepts a history step; evicted targets complete asynchronously through poll(). */
    bool undo();
    bool redo();
    /** Preserves an active edit while releasing the document's other retained frames. */
    bool clearHistory();
    /** Release GPU versions outside the current frame, retaining all CPU history. */
    Result evictHistory();
    /** Keep CPU history and accepted working input while releasing this document's GPU ownership. */
    Result suspend();
    /** Recreate a suspended document on a new concrete store through poll(). */
    Result resume(std::shared_ptr<KisGpuTileStore> store);
    /** Cancel a queued Undo/Redo restoration and keep the previously displayed frame. */
    bool cancelHistoryRestore();
    /** Explicit maintenance of one current target. History and GPU versions stay stable. */
    Result compactRecovery(LayerId id, Target target = Target::Pixels);
    std::optional<KisGpuTileStore::Status> recoveryCompactionStatus() const;
    KisGpuTileStore::Error recoveryCompactionError() const;
    State state() const;
    Result lastPublicationResult() const { return m_lastResult; }
    QVector<Layer> layers() const;
    QVector<KisGpuTileStore::Layer> projectionLayers(bool includePreview = true) const;
    qsizetype historySize() const { return m_history.size(); }
    /** Logical CPU payload owned by history and the active editing lease. */
    quint64 recoveryBytes() const;

private:
    using LayerState = RecoveryLayer;
    using Record = std::shared_ptr<const LayerState>;
    using Frame = QVector<Record>;
    struct Active {
        LayerId id;
        Target target;
        KisGpuEditSession::Token token;
    };
    struct Compaction {
        RasterRecovery source;
        std::shared_ptr<KisGpuEditSession> session;
        quint64 reservedBytes = 0;
    };
    struct Restoration {
        Recovery request;
        QVector<std::shared_ptr<const RecoveryLayer>> records;
        qsizetype index = 0;
        std::map<const KisGpuEditSession::Recovery *, KisGpuTileStore::Version> rasters;
        std::shared_ptr<KisGpuEditSession> session;
        bool initial = true, maskPhase = false, failed = false;
        KisGpuTileStore::Error error = KisGpuTileStore::Error::None;
    };
    void pollRecovery();
    void pollRecoveryCompaction();
    bool requestHistoryCursor(qsizetype cursor);
    static quint64 recoveryBytes(const Recovery &recovery);
    static std::optional<quint64> workingMaximum(const Recovery &recovery, quint64 maximumRecoveryBytes);
    qsizetype indexOf(LayerId id) const;
    KisGpuTileStore::Version rasterVersion(const RasterRecovery &raster) const;
    Layer layerValue(const LayerState &record) const;
    void pruneRasterVersions();
    quint64 recordMetadataBytes(const LayerState &record) const;
    quint64 historyBytes(const QVector<Frame> &history) const;
    Result publish(Frame frame);
    Result readiness() const;
    std::shared_ptr<KisGpuTileStore> m_store;
    qsizetype m_retainedEdits, m_cursor = 0;
    quint64 m_maximumRecoveryBytes;
    LayerId m_nextId = 1;
    QVector<Frame> m_history;
    std::map<const KisGpuEditSession::Recovery *, KisGpuTileStore::Version> m_rasterVersions;
    std::optional<Active> m_active;
    std::shared_ptr<KisGpuEditSession> m_session;
    std::optional<Restoration> m_restoring;
    std::optional<Compaction> m_compacting;
    std::optional<KisGpuTileStore::Status> m_compactionStatus;
    KisGpuTileStore::Error m_compactionError = KisGpuTileStore::Error::None;
    Result m_lastResult = Result::Accepted;
};

#endif
