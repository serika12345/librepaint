/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_STROKE_INPUT_H
#define KIS_GPU_STROKE_INPUT_H
#include "KisGpuBrushStroke.h"
#include "KisGpuLayerDocument.h"
#include <deque>

/** One tool operation with bounded CPU input and replaceable GPU resources.
 * Calls and destruction share the document's submitting thread. The document
 * is retained; releaseGpu() runs in the shared owner's direct release signal.
 */
class KisGpuStrokeInput
{
public:
    using Token = KisGpuLayerDocument::EditLease;
    enum class Result { Accepted, Busy, Stale, InvalidInput, QueueFull, GpuRejected, RecoveryBudgetExceeded, LayerNotEditable };
    enum class State { Idle, Drawing, Finishing, Finished, Cancelled };
    struct Configuration {
        KisGpuBrushStroke::Settings brush;
        QRect clip;
        std::shared_ptr<const KisGpuEditSession::ReplayTexture> texture;
        std::shared_ptr<const KisGpuEditSession::ReplaySelection> selection;
    };
    struct Started { Result result = Result::Busy; Token token; };
    /** Null document or zero limits throw std::invalid_argument. */
    explicit KisGpuStrokeInput(std::shared_ptr<KisGpuLayerDocument> document,
                               qsizetype maximumSamples = 1024, quint64 maximumResourceBytes = 4 * 1024 * 1024);
    ~KisGpuStrokeInput();
    KisGpuStrokeInput(const KisGpuStrokeInput &) = delete;
    KisGpuStrokeInput &operator=(const KisGpuStrokeInput &) = delete;
    Started begin(KisGpuLayerDocument::LayerId layer, Configuration configuration,
                  KisGpuLayerDocument::Target target = KisGpuLayerDocument::Target::Pixels);
    /** Acceptance retains normalized CPU input; poll() performs bounded GPU work. */
    Result append(const Token &token, const QVector<KisGpuBrushStroke::Sample> &samples);
    /** Supersede queued input and stage a whole shape from the committed base.
     * The previous preview stays visible until all replacement batches succeed.
     * Retained replacement samples count against the CPU queue limit until publication.
     */
    Result replace(const Token &token, const QVector<KisGpuBrushStroke::Sample> &samples);
    /** Drain accepted input before committing; later append/replace calls are stale. */
    Result commit(const Token &token);
    Result cancel(const Token &token);
    /** Keep CPU progress, resources and queued input while releasing every GPU reference. */
    void releaseGpu();
    /** Advance restoration and at most one generated batch; refused input remains queued. */
    Result poll();
    State state() const { return m_state; }
    qsizetype pendingSamples() const { return qsizetype(m_queue.size()); }
    KisGpuTileStore::Error lastGpuError() const { return m_gpuError; }
    /** CPU-only configuration and resource-size validation before retaining GUI input. */
    bool validConfiguration(const Configuration &configuration) const;
private:
    bool matches(const Token &token) const;
    Result prepare();
    void finish(State state);
    std::shared_ptr<KisGpuLayerDocument> m_document;
    const qsizetype m_maximumSamples;
    const quint64 m_maximumResourceBytes;
    Configuration m_configuration;
    Token m_token;
    KisGpuBrushStroke::Progress m_progress;
    KisGpuLayerDocument::Editing m_edit;
    std::optional<KisGpuEditSession::Texture> m_texture;
    std::optional<KisGpuEditSession::Selection> m_selection;
    std::unique_ptr<KisGpuBrushStroke> m_brush;
    std::deque<KisGpuBrushStroke::Sample> m_queue;
    qsizetype m_replacementSamples = 0, m_stagedSamples = 0;
    State m_state = State::Idle;
    bool m_replace = false, m_replacementStarted = false, m_commitSent = false;
    KisGpuTileStore::Error m_gpuError = KisGpuTileStore::Error::None;
};
#endif
