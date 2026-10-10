/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuEditSession.h"
#include "KisGpuEditReplay_p.h"
#include <limits>
#include <stdexcept>

KisGpuEditSession::KisGpuEditSession(KisGpuTileStore &store, qsizetype retainedEdits)
    : KisGpuEditSession(store, retainedEdits, store.emptyVersion())
{
}

KisGpuEditSession::KisGpuEditSession(KisGpuTileStore &store, qsizetype retainedEdits,
                                   const KisGpuTileStore::Version &initial)
    : m_replay(new ReplayState), m_store(store), m_retainedEdits(qMax(qsizetype(0), retainedEdits)), m_history{initial}
    , m_preview(m_history.front())
{
    const auto checked = store.paint(initial, QVector<KisGpuTileStore::PaintCommand>());
    if (checked.error != KisGpuTileStore::Error::None || checked.completion.status() != KisGpuTileStore::Status::Succeeded) {
        throw std::invalid_argument("GPU editing requires a successful version from its tile store");
    }
    if (!initial.tileCount()) m_replay->data = Recovery{};
}

KisGpuEditSession::Token KisGpuEditSession::begin()
{
    if (m_state != State::Idle || !m_store.deviceAvailable()
        || m_generation == std::numeric_limits<quint64>::max()) return {};
    m_token = ++m_generation;
    m_base = head();
    m_preview = m_base;
    m_state = State::Editing;
    m_replay->working.clear();
    m_replay->commitRequested = false;
    Token token;
    token.generation = m_token;
    token.owner = m_identity;
    return token;
}

bool KisGpuEditSession::matches(const Token &token) const
{
    return token.generation && token.generation == m_token && token.owner.lock() == m_identity;
}

KisGpuEditSession::Result KisGpuEditSession::append(const Token &token,
    const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip)
{
    return paint(token, commands, clip, false);
}

KisGpuEditSession::Result KisGpuEditSession::append(const Token &token,
    const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, const Selection &selection)
{
    return paint(token, commands, clip, false, &selection);
}

KisGpuEditSession::Result KisGpuEditSession::replace(const Token &token,
    const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip)
{
    return paint(token, commands, clip, true);
}

KisGpuEditSession::Result KisGpuEditSession::replace(const Token &token,
    const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, const Selection &selection)
{
    return paint(token, commands, clip, true, &selection);
}

KisGpuEditSession::SelectionResult KisGpuEditSession::createSelection(
    const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip)
{
    if (sizeof(ReplaySelection) + quint64(commands.size()) * sizeof(KisGpuTileStore::DabCommand)
        > m_replay->maximumBytes) return {Result::RecoveryBudgetExceeded, {}};
    const auto edit = m_store.paintDabs(m_store.emptyVersion(), commands, clip);
    m_lastGpuError = edit.error;
    if (edit.error != KisGpuTileStore::Error::None) return {};
    Selection selection;
    selection.version = edit.version;
    selection.source = std::make_shared<const ReplaySelection>(ReplaySelection{commands, clip});
    return {Result::Accepted, std::move(selection)};
}

KisGpuEditSession::Result KisGpuEditSession::paint(const Token &token,
    const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, bool replace, const Selection *selection)
{
    if (!matches(token)) return Result::Stale;
    if (m_state != State::Editing) return Result::Busy;
    if (m_hasLatest && m_latest.completion.status() == KisGpuTileStore::Status::Failed) return Result::Busy;
    if (selection && !*selection) {
        m_lastGpuError = KisGpuTileStore::Error::InvalidVersion;
        return Result::GpuRejected;
    }
    ReplayBatch batch{commands, clip, selection ? selection->source : nullptr};
    if (!canRecord(batch, replace)) return Result::RecoveryBudgetExceeded;
    const auto &source = replace || !m_hasLatest ? m_base : m_latest.version;
    auto edit = selection ? m_store.paintDabs(source, commands, clip, selection->version)
                          : m_store.paintDabs(source, commands, clip);
    m_lastGpuError = edit.error;
    if (edit.error != KisGpuTileStore::Error::None) return Result::GpuRejected;
    record(std::move(batch), replace);
    m_latest = std::move(edit);
    m_hasLatest = true;
    return Result::Accepted;
}

KisGpuEditSession::Result KisGpuEditSession::commit(const Token &token)
{
    if (!matches(token)) return Result::Stale;
    if (m_state != State::Editing) return Result::Busy;
    m_state = State::Committing;
    m_replay->commitRequested = true;
    return Result::Accepted;
}

void KisGpuEditSession::releaseWorkingEdit()
{
    m_token = 0;
    m_base = {};
    m_latest = {};
    m_hasLatest = false;
    m_preview = head();
    m_state = State::Idle;
    m_replay->working.clear();
    m_replay->commitRequested = false;
}

KisGpuEditSession::Result KisGpuEditSession::cancel(const Token &token)
{
    if (!matches(token)) return Result::Stale;
    releaseWorkingEdit();
    return Result::Accepted;
}

void KisGpuEditSession::poll()
{
    m_store.poll();
    if (m_state == State::Restoring) { pollRecovery(); return; }
    if (m_state != State::Editing && m_state != State::Committing) return;
    if (!m_store.deviceAvailable()) {
        m_preview = m_base;
        m_state = State::Failed;
        return;
    }
    if (m_hasLatest) {
        const auto status = m_latest.completion.status();
        if (status == KisGpuTileStore::Status::Pending) return;
        if (status == KisGpuTileStore::Status::Failed) {
            m_preview = m_base;
            m_state = State::Failed;
            return;
        }
        m_preview = m_latest.version;
    }
    if (m_state != State::Committing) return;
    if (m_hasLatest && !(m_latest.version == m_base)) {
        m_history.resize(m_cursor + 1);
        m_history.push_back(m_latest.version);
        if (m_history.size() - 1 > m_retainedEdits) m_history.remove(0, m_history.size() - 1 - m_retainedEdits);
        m_cursor = m_history.size() - 1;
        publishRecovery();
    }
    releaseWorkingEdit();
}

bool KisGpuEditSession::undo()
{
    if (m_state != State::Idle || !m_cursor) return false;
    m_preview = m_history[--m_cursor];
    if (m_replay->data) --m_replay->data->cursor;
    return true;
}

bool KisGpuEditSession::redo()
{
    if (m_state != State::Idle || m_cursor + 1 >= m_history.size()) return false;
    m_preview = m_history[++m_cursor];
    if (m_replay->data) ++m_replay->data->cursor;
    return true;
}

bool KisGpuEditSession::clearHistory()
{
    if (m_state != State::Idle) return false;
    m_history = {head()};
    m_cursor = 0;
    if (m_replay->data) {
        m_replay->data->edits.resize(m_replay->data->cursor);
        m_replay->data->firstRetained = m_replay->data->cursor;
    }
    return true;
}
