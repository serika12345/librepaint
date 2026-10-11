/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuStrokeInput.h"
#include <algorithm>
#include <stdexcept>

namespace {
using Document = KisGpuLayerDocument;
using Brush = KisGpuBrushStroke;
using Session = KisGpuEditSession;
using Result = KisGpuStrokeInput::Result;
Result translated(Document::Result result)
{
    switch (result) {
    case Document::Result::Accepted: return Result::Accepted;
    case Document::Result::Busy: return Result::Busy;
    case Document::Result::Stale: return Result::Stale;
    case Document::Result::GpuRejected: return Result::GpuRejected;
    case Document::Result::RecoveryBudgetExceeded: return Result::RecoveryBudgetExceeded;
    case Document::Result::LayerNotEditable: return Result::LayerNotEditable;
    default: return Result::InvalidInput;
    }
}
}

KisGpuStrokeInput::KisGpuStrokeInput(std::shared_ptr<Document> document, qsizetype samples, quint64 resources)
    : m_document(std::move(document)), m_maximumSamples(samples), m_maximumResourceBytes(resources)
{
    if (!m_document || samples <= 0 || !resources)
        throw std::invalid_argument("GPU tool input requires a document and positive CPU limits");
}

KisGpuStrokeInput::~KisGpuStrokeInput()
{
    if (m_token && !m_commitSent) cancel(m_token);
    releaseGpu();
}

bool KisGpuStrokeInput::matches(const Token &token) const
{
    return token && token == m_token && token == m_document->currentEditLease();
}

bool KisGpuStrokeInput::validConfiguration(const Configuration &config) const
{
    if (!config.clip.isValid() || config.brush.texture || !Brush::validSettings(config.brush)
        || (config.texture && config.brush.smudgeRate)) return false;
    quint64 bytes = 0;
    if (config.texture) {
        const auto &texture = *config.texture;
        if (texture.size.width() <= 0 || texture.size.height() <= 0
            || quint64(texture.alpha.size()) != quint64(texture.size.width()) * quint64(texture.size.height())) return false;
        bytes = quint64(texture.alpha.size());
    }
    if (bytes > m_maximumResourceBytes) return false;
    if (config.selection && (!config.selection->clip.isValid()
        || quint64(config.selection->commands.size()) > (m_maximumResourceBytes - bytes) / sizeof(KisGpuTileStore::DabCommand))) return false;
    return true;
}

KisGpuStrokeInput::Started KisGpuStrokeInput::begin(Document::LayerId layer, Configuration config, Document::Target target)
{
    if (m_state == State::Drawing || m_state == State::Finishing) return {Result::Busy, {}};
    if (!validConfiguration(config)) return {Result::InvalidInput, {}};
    auto edit = m_document->begin(layer, target);
    if (edit.result != Document::Result::Accepted) return {translated(edit.result), {}};
    m_edit = std::move(edit);
    m_token = m_document->currentEditLease();
    m_configuration = std::move(config);
    m_progress = {};
    m_replace = m_commitSent = false;
    m_replacementStarted = false;
    m_replacementSamples = m_stagedSamples = 0;
    m_gpuError = KisGpuTileStore::Error::None;
    m_state = State::Drawing;
    const auto result = prepare();
    if (result != Result::Accepted) { cancel(m_token); return {result, {}}; }
    return {Result::Accepted, m_token};
}

KisGpuStrokeInput::Result KisGpuStrokeInput::append(const Token &token, const QVector<Brush::Sample> &samples)
{
    if (m_state != State::Drawing || !matches(token)) return Result::Stale;
    if (samples.size() > m_maximumSamples - pendingSamples()) return Result::QueueFull;
    for (const auto &sample : samples) if (!Brush::validSample(sample)) return Result::InvalidInput;
    auto &queue = m_nextReplacement ? m_nextReplacement->samples : m_queue;
    queue.insert(queue.end(), samples.begin(), samples.end());
    return Result::Accepted;
}

KisGpuStrokeInput::Result KisGpuStrokeInput::replace(const Token &token, const QVector<Brush::Sample> &samples)
{
    if (m_state != State::Drawing || !matches(token)) return Result::Stale;
    if (samples.size() > m_maximumSamples) return Result::QueueFull;
    const bool coalesce = m_replacementStarted && !samples.isEmpty();
    if (coalesce && samples.size() > m_maximumSamples - m_replacementSamples) return Result::QueueFull;
    for (const auto &sample : samples) if (!Brush::validSample(sample)) return Result::InvalidInput;
    std::deque<Brush::Sample> next(samples.begin(), samples.end());
    if (coalesce) {
        m_queue.resize(size_t(m_replacementSamples));
        m_nextReplacement = ReplacementInput{std::move(next), samples.size()};
        return Result::Accepted;
    }
    m_brush.reset();
    if (m_edit.session && m_replacementStarted) m_edit.session->discardReplacement(m_edit.token);
    m_nextReplacement.reset();
    m_progress = {};
    m_queue.swap(next);
    m_replace = true;
    m_replacementStarted = false;
    m_replacementSamples = samples.size();
    m_stagedSamples = 0;
    return Result::Accepted;
}

KisGpuStrokeInput::Result KisGpuStrokeInput::commit(const Token &token)
{
    if (m_state != State::Drawing || !matches(token)) return Result::Stale;
    m_state = State::Finishing;
    return Result::Accepted;
}

KisGpuStrokeInput::Result KisGpuStrokeInput::cancel(const Token &token)
{
    if (!matches(token)) return Result::Stale;
    if (m_commitSent) return Result::Busy;
    const auto edit = m_document->currentEditing();
    const auto result = edit.result == Document::Result::Accepted
        ? m_document->cancel(edit.token) : m_document->discardRecoveringEdit(token);
    if (result == Document::Result::Accepted) finish(State::Cancelled);
    return translated(result);
}

void KisGpuStrokeInput::releaseGpu()
{
    if (m_brush && !m_replace) m_progress = m_brush->progress();
    m_brush.reset();
    if (m_edit.session && m_replacementStarted) m_edit.session->discardReplacement(m_edit.token);
    promoteReplacement();
    if (m_replace) m_progress = {};
    m_replacementStarted = false;
    m_stagedSamples = 0;
    m_selection.reset();
    m_texture.reset();
    m_edit = {};
}

void KisGpuStrokeInput::finish(State state)
{
    releaseGpu();
    m_queue.clear();
    m_nextReplacement.reset();
    m_token = {};
    m_configuration = {};
    m_progress = {};
    m_replace = m_commitSent = false;
    m_replacementStarted = false;
    m_replacementSamples = m_stagedSamples = 0;
    m_state = state;
}

void KisGpuStrokeInput::promoteReplacement()
{
    if (!m_nextReplacement) return;
    m_queue = std::move(m_nextReplacement->samples);
    m_replacementSamples = m_nextReplacement->shapeSamples;
    m_nextReplacement.reset();
    m_brush.reset();
    m_progress = {};
    m_stagedSamples = 0;
    m_replace = true;
    m_replacementStarted = false;
}

KisGpuStrokeInput::Result KisGpuStrokeInput::prepare()
{
    if (m_brush) return Result::Accepted;
    const auto edit = m_document->currentEditing();
    if (edit.result != Document::Result::Accepted) return Result::Busy;
    m_edit = edit;
    const auto fail = [&](Session::Result result) {
        m_gpuError = m_edit.session->lastGpuError();
        return result == Session::Result::RecoveryBudgetExceeded ? Result::RecoveryBudgetExceeded : Result::GpuRejected;
    };
    if (m_configuration.texture && !m_texture) {
        const auto &source = *m_configuration.texture;
        const auto prepared = m_edit.session->createTexture(source.size, source.alpha, source.origin);
        if (prepared.result != Session::Result::Accepted) return fail(prepared.result);
        m_texture = prepared.texture;
    }
    if (m_configuration.selection && !m_selection) {
        const auto &source = *m_configuration.selection;
        const auto prepared = m_edit.session->createSelection(source.commands, source.clip);
        if (prepared.result != Session::Result::Accepted) return fail(prepared.result);
        m_selection = prepared.selection;
    }
    auto settings = m_configuration.brush;
    settings.texture = m_texture;
    m_brush = std::make_unique<Brush>(*m_edit.session, m_edit.token, settings, m_configuration.clip, m_progress);
    return Result::Accepted;
}

KisGpuStrokeInput::Result KisGpuStrokeInput::poll()
{
    m_document->poll();
    if (m_state != State::Drawing && m_state != State::Finishing) return Result::Accepted;
    if (!matches(m_token)) {
        finish(m_commitSent ? State::Finished : State::Cancelled);
        return Result::Accepted;
    }
    if (m_document->state() == Document::State::Failed) return Result::GpuRejected;
    if (m_commitSent) return Result::Busy;
    const auto prepared = prepare();
    if (prepared != Result::Accepted) return prepared;
    const auto rejected = [&](Session::Result result) {
        m_gpuError = m_edit.session->lastGpuError();
        if (result == Session::Result::Busy) return Result::Busy;
        return result == Session::Result::RecoveryBudgetExceeded ? Result::RecoveryBudgetExceeded : Result::GpuRejected;
    };
    if (m_replace && !m_replacementStarted) {
        const auto result = m_edit.session->beginReplacement(m_edit.token);
        if (result != Session::Result::Accepted) return rejected(result);
        m_replacementStarted = true;
    }
    if ((m_replace && m_stagedSamples < m_replacementSamples) || (!m_replace && !m_queue.empty())) {
        QVector<Brush::Sample> samples;
        const qsizetype first = m_replace ? m_stagedSamples : 0;
        const qsizetype last = m_replace ? m_replacementSamples : qsizetype(m_queue.size());
        samples.reserve(std::min<qsizetype>(64, last - first));
        for (qsizetype i = first; i < last && samples.size() < 64; ++i) samples.append(m_queue[size_t(i)]);
        const auto update = m_brush->appendPartial(samples, m_selection);
        if (update.result != Brush::Result::Accepted) {
            m_gpuError = m_edit.session->lastGpuError();
            return update.editResult == Session::Result::RecoveryBudgetExceeded ? Result::RecoveryBudgetExceeded : Result::GpuRejected;
        }
        m_progress = m_brush->progress();
        if (m_replace) m_stagedSamples += update.completedSamples;
        else for (qsizetype i = 0; i < update.completedSamples; ++i) m_queue.pop_front();
        m_gpuError = KisGpuTileStore::Error::None;
    }
    if (m_replace && m_stagedSamples == m_replacementSamples) {
        const auto result = m_edit.session->publishReplacement(m_edit.token);
        if (result == Session::Result::Busy) return Result::Accepted;
        if (result != Session::Result::Accepted) return rejected(result);
        for (qsizetype i = 0; i < m_replacementSamples; ++i) m_queue.pop_front();
        m_replacementSamples = m_stagedSamples = 0;
        m_replace = m_replacementStarted = false;
        m_gpuError = KisGpuTileStore::Error::None;
        promoteReplacement();
    }
    if (m_state == State::Finishing && m_queue.empty() && !m_replace) {
        const auto result = m_document->commit(m_edit.token);
        if (result != Document::Result::Accepted) return translated(result);
        m_commitSent = true;
        releaseGpu();
    }
    return Result::Accepted;
}
