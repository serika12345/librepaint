/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_EDIT_SESSION_H
#define KIS_GPU_EDIT_SESSION_H

#include "KisGpuTileStore.h"
#include <optional>

/**
 * Tentative raster edits and bounded immutable Undo/Redo history. The borrowed
 * store outlives the session. All calls share the store's submitting thread.
 * A token identifies one edit; asynchronous input retains that token so an old
 * edit cannot affect a replacement operation or another document session.
 */
class KisGpuEditSession
{
    struct Identity {};
public:
    enum class State { Idle, Editing, Committing, Failed, Restoring };
    enum class Result { Accepted, Busy, Stale, GpuRejected, RecoveryBudgetExceeded };
    struct ReplayBatch {
        QVector<KisGpuTileStore::DabCommand> commands;
        QRect clip;
    };
    using ReplayEdit = QVector<ReplayBatch>;
    /** CPU checkpoint and accepted input values; independent of GPU handles and session tokens. */
    struct Recovery {
        QRect bounds;
        QByteArray pixels;
        QVector<ReplayEdit> edits;
        qsizetype firstRetained = 0, cursor = 0;
        std::optional<ReplayEdit> working;
        bool commitRequested = false;
    };
    class Token {
    public:
        Token() = default;
        explicit operator bool() const { return generation && !owner.expired(); }
        bool operator==(const Token &other) const {
            return generation == other.generation && !owner.owner_before(other.owner) && !other.owner.owner_before(owner);
        }
        bool operator!=(const Token &other) const { return !(*this == other); }
    private:
        friend class KisGpuEditSession;
        quint64 generation = 0;
        std::weak_ptr<const Identity> owner;
    };

    explicit KisGpuEditSession(KisGpuTileStore &store, qsizetype retainedEdits);
    /** Initial version must belong to store and have succeeded; otherwise throws std::invalid_argument. */
    KisGpuEditSession(KisGpuTileStore &store, qsizetype retainedEdits, const KisGpuTileStore::Version &initial);
    /** Replays asynchronously in poll(); invalid indices, checkpoint or CPU budget throw std::invalid_argument. */
    KisGpuEditSession(KisGpuTileStore &store, qsizetype retainedEdits, Recovery recovery,
                      quint64 maximumRecoveryBytes = 64 * 1024 * 1024);
    ~KisGpuEditSession();
    KisGpuEditSession(const KisGpuEditSession &) = delete;
    KisGpuEditSession &operator=(const KisGpuEditSession &) = delete;

    /** Returns an empty token while another edit is active. Tokens may outlive the session. */
    Token begin();
    /** Current token after restoring an unfinished edit; old session tokens remain stale. */
    Token currentToken() const;
    /** Loaded GPU-only initial versions need their CPU checkpoint supplied at construction. */
    std::optional<Recovery> recovery() const;
    Result append(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip);
    /** Redraw from the committed starting version; previous tentative footprints disappear. */
    Result replace(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip);
    /** Adoption occurs in poll() after successful completion; no second raster pass. */
    Result commit(const Token &token);
    Result cancel(const Token &token);
    bool undo();
    bool redo();
    bool clearHistory();
    void poll();

    State state() const { return m_state; }
    KisGpuTileStore::Error lastGpuError() const { return m_lastGpuError; }
    KisGpuTileStore::Version head() const { return m_history[m_cursor]; }
    KisGpuTileStore::Version preview() const { return m_preview; }
    qsizetype historySize() const { return m_history.size(); }

private:
    bool matches(const Token &token) const;
    Result paint(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, bool replace);
    void releaseWorkingEdit();
    struct ReplayState;
    void pollRecovery();
    bool canRecord(const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, bool replace) const;
    void record(const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, bool replace);
    void publishRecovery();
    std::unique_ptr<ReplayState> m_replay;
    KisGpuTileStore &m_store;
    qsizetype m_retainedEdits;
    QVector<KisGpuTileStore::Version> m_history;
    qsizetype m_cursor = 0;
    quint64 m_generation = 0, m_token = 0;
    std::shared_ptr<const Identity> m_identity = std::make_shared<const Identity>();
    State m_state = State::Idle;
    KisGpuTileStore::Error m_lastGpuError = KisGpuTileStore::Error::None;
    KisGpuTileStore::Version m_base, m_preview;
    KisGpuTileStore::Edit m_latest;
    bool m_hasLatest = false;
};

#endif
