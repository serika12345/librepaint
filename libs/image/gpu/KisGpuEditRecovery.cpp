/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuEditReplay_p.h"
#include <set>
#include <stdexcept>

namespace {
using Session = KisGpuEditSession;
quint64 editBytes(const Session::ReplayEdit &edit, std::set<const Session::ReplaySelection *> &selections,
                  std::set<const Session::ReplayTexture *> &textures)
{
    quint64 bytes = sizeof(Session::ReplayEdit);
    for (const auto &batch : edit) {
        bytes += sizeof(Session::ReplayBatch) + quint64(batch.commands.size()) * sizeof(KisGpuTileStore::DabCommand);
        if (batch.selection && selections.insert(batch.selection.get()).second) {
            bytes += sizeof(Session::ReplaySelection)
                + quint64(batch.selection->commands.size()) * sizeof(KisGpuTileStore::DabCommand);
        }
        if (batch.texture && textures.insert(batch.texture.get()).second)
            bytes += sizeof(Session::ReplayTexture) + quint64(batch.texture->alpha.size());
    }
    return bytes;
}
quint64 recoveryBytes(const Session::Recovery &data, const Session::ReplayEdit *working = nullptr)
{
    std::set<const Session::ReplaySelection *> selections;
    std::set<const Session::ReplayTexture *> textures;
    quint64 bytes = data.pixels.size();
    for (const auto &edit : data.edits) bytes += editBytes(edit, selections, textures);
    if (data.working) bytes += editBytes(*data.working, selections, textures);
    if (working) bytes += editBytes(*working, selections, textures);
    return bytes;
}
}

KisGpuEditSession::~KisGpuEditSession() = default;
KisGpuEditSession::KisGpuEditSession(KisGpuTileStore &store, qsizetype retained, Recovery recovery, quint64 maximum)
    : KisGpuEditSession(store, retained)
{
    const qint64 width = recovery.bounds.width(), height = recovery.bounds.height();
    const bool empty = recovery.bounds.isEmpty();
    if (recovery.firstRetained < 0 || recovery.cursor < recovery.firstRetained
        || recovery.cursor > recovery.edits.size()
        || recovery.edits.size() - recovery.firstRetained > m_retainedEdits
        || (empty ? !recovery.pixels.isEmpty()
                  : width * height > (qint64(1) << 29) - 1 || width * height * 4 != recovery.pixels.size())
        || (!recovery.working && recovery.commitRequested) || recoveryBytes(recovery) > maximum) {
        throw std::invalid_argument("Invalid GPU recovery checkpoint, history or CPU payload budget");
    }
    m_replay->restoring = std::move(recovery);
    m_replay->maximumBytes = maximum;
    m_state = State::Restoring;
}

KisGpuEditSession::Token KisGpuEditSession::currentToken() const
{
    Token result;
    result.generation = m_token;
    result.owner = m_identity;
    return result;
}

std::optional<KisGpuEditSession::Recovery> KisGpuEditSession::recovery() const
{
    if (m_replay->restoring) return m_replay->restoring;
    if (!m_replay->data) return {};
    Recovery result = *m_replay->data;
    if (m_token) {
        result.working = m_replay->working;
        result.commitRequested = m_replay->commitRequested;
    }
    return result;
}

bool KisGpuEditSession::canRecord(const ReplayBatch &batch, bool replace) const
{
    if (!m_replay->data || batch.commands.isEmpty() || batch.clip.isEmpty()) return true;
    ReplayEdit working = replace ? ReplayEdit{} : m_replay->working;
    working.push_back(batch);
    return recoveryBytes(*m_replay->data, &working) <= m_replay->maximumBytes;
}

void KisGpuEditSession::record(ReplayBatch batch, bool replace)
{
    if (!m_replay->data) return;
    if (replace) m_replay->working.clear();
    if (!batch.commands.isEmpty() && !batch.clip.isEmpty()) m_replay->working.push_back(std::move(batch));
}

void KisGpuEditSession::publishRecovery()
{
    if (!m_replay->data) return;
    auto &data = *m_replay->data;
    data.edits.resize(data.cursor);
    data.edits.push_back(m_replay->working);
    ++data.cursor;
    data.firstRetained = data.cursor - m_cursor;
}

void KisGpuEditSession::pollRecovery()
{
    auto &state = *m_replay;
    const auto &request = *state.restoring;
    if (!m_store.deviceAvailable()) { m_lastGpuError = KisGpuTileStore::Error::DeviceLost; m_state = State::Failed; return; }
    if (state.waiting) {
        const auto status = state.latest.completion.status();
        if (status == KisGpuTileStore::Status::Pending) return;
        if (status == KisGpuTileStore::Status::Failed) { m_state = State::Failed; return; }
        state.waiting = false;
    }
    const auto wait = [&](KisGpuTileStore::Edit edit) {
        if (edit.error != KisGpuTileStore::Error::None) {
            if (edit.error != KisGpuTileStore::Error::QueueFull) {
                m_lastGpuError = edit.error;
                m_state = State::Failed;
            }
            return false;
        }
        state.latest = std::move(edit);
        state.waiting = true;
        return true;
    };
    const auto submit = [&](KisGpuTileStore::Edit edit) {
        const auto version = edit.version;
        if (!wait(std::move(edit))) return false;
        state.replay = version;
        return true;
    };
    const auto replayBatch = [&](const ReplayBatch &batch) {
        if (batch.texture && !state.textureReady) {
            const auto texture = m_store.uploadBrushTexture(batch.texture->size, batch.texture->alpha);
            if (wait({texture.error, {}, texture.completion})) {
                state.texture = texture.texture;
                state.textureReady = true;
            }
            return;
        }
        if (batch.selection && !state.selectionReady) {
            auto mask = m_store.paintDabs(m_store.emptyVersion(), batch.selection->commands, batch.selection->clip);
            const auto version = mask.version;
            if (wait(std::move(mask))) {
                state.selection = version;
                state.selectionReady = true;
            }
            return;
        }
        auto edit = batch.texture ? m_store.paintDabs(state.replay, batch.commands, batch.clip, state.texture,
                                                      batch.texture->origin, batch.selection ? &state.selection : nullptr)
                                  : (batch.selection ? m_store.paintDabs(state.replay, batch.commands, batch.clip, state.selection)
                                                     : m_store.paintDabs(state.replay, batch.commands, batch.clip));
        if (submit(std::move(edit))) {
            ++state.batch;
            state.selection = {};
            state.selectionReady = false;
            state.texture = {};
            state.textureReady = false;
        }
    };
    if (!state.initialized) {
        state.replay = m_store.emptyVersion();
        if (!request.pixels.isEmpty() && !submit(m_store.upload(state.replay, request.bounds, request.pixels))) return;
        state.initialized = true;
        if (!request.firstRetained) state.frames.push_back(state.replay);
        if (state.waiting) return;
    }
    // At most one GPU operation per poll; the caller's event loop remains available.
    while (!state.workingPhase && state.edit < request.edits.size()) {
        const auto &edit = request.edits[state.edit];
        if (state.batch < edit.size()) {
            const auto &batch = edit[state.batch];
            replayBatch(batch);
            return;
        }
        ++state.edit;
        state.batch = 0;
        if (state.edit >= request.firstRetained) state.frames.push_back(state.replay);
    }
    if (request.working && !state.workingPhase) {
        state.workingPhase = true;
        state.replay = state.frames[request.cursor - request.firstRetained];
        state.latest = {};
    }
    if (request.working && state.batch < request.working->size()) {
        const auto &batch = (*request.working)[state.batch];
        replayBatch(batch);
        return;
    }
    m_history = state.frames;
    m_cursor = request.cursor - request.firstRetained;
    m_preview = head();
    state.data = request;
    state.data->working.reset();
    state.data->commitRequested = false;
    m_state = State::Idle;
    if (request.working) {
        begin();
        state.working = *request.working;
        state.commitRequested = request.commitRequested;
        m_hasLatest = !state.working.isEmpty();
        if (m_hasLatest) {
            m_latest = state.latest;
            m_preview = state.replay;
        }
        if (request.commitRequested) m_state = State::Committing;
    }
    state.restoring.reset();
    state.frames.clear();
    state.replay = {};
    state.latest = {};
}
