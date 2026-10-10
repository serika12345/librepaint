/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_LAYER_DOCUMENT_H
#define KIS_GPU_LAYER_DOCUMENT_H

#include "KisGpuEditSession.h"
#include <QString>

/** Ordered raster layers and one bounded document-wide Undo/Redo sequence.
 * Calls and destruction use the store's submitting thread. Editing leases keep
 * their concrete store alive; tokens become stale after cancellation or close.
 */
class KisGpuLayerDocument
{
public:
    using LayerId = quint64;
    enum class Result { Accepted, Busy, Stale, InvalidLayer, InvalidInput, RecoveryBudgetExceeded, GpuRejected };
    enum class State { Idle, Editing, Committing, Failed };
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
    /** Null store or a budget too small for the empty history throws std::invalid_argument. */
    KisGpuLayerDocument(std::shared_ptr<KisGpuTileStore> store, qsizetype retainedEdits,
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
    Result commit(const KisGpuEditSession::Token &token);
    Result cancel(const KisGpuEditSession::Token &token);
    void poll();
    bool undo();
    bool redo();
    /** Preserves an active edit while releasing the document's other retained frames. */
    void clearHistory();
    State state() const;
    Result lastPublicationResult() const { return m_lastResult; }
    QVector<Layer> layers() const;
    QVector<KisGpuTileStore::Layer> projectionLayers(bool includePreview = true) const;
    qsizetype historySize() const { return m_history.size(); }
    /** Logical CPU payload owned by history and the active editing lease. */
    quint64 recoveryBytes() const;

private:
    struct LayerState {
        Layer layer;
        KisGpuEditSession::Recovery pixels;
        std::optional<KisGpuEditSession::Recovery> mask;
    };
    using Record = std::shared_ptr<const LayerState>;
    using Frame = QVector<Record>;
    struct Active {
        LayerId id;
        Target target;
        KisGpuEditSession::Token token;
    };
    qsizetype indexOf(LayerId id) const;
    quint64 recordBytes(const LayerState &record) const;
    quint64 historyBytes(const QVector<Frame> &history) const;
    Result publish(Frame frame);
    Result readiness() const;
    std::shared_ptr<KisGpuTileStore> m_store;
    qsizetype m_retainedEdits, m_cursor = 0;
    quint64 m_maximumRecoveryBytes;
    LayerId m_nextId = 1;
    QVector<Frame> m_history;
    std::optional<Active> m_active;
    std::shared_ptr<KisGpuEditSession> m_session;
    Result m_lastResult = Result::Accepted;
};

#endif
