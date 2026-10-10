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
    /** Immutable selection source; its alpha is rebuilt on the replacement GPU. */
    struct ReplaySelection {
        QVector<KisGpuTileStore::DabCommand> commands;
        QRect clip;
    };
    struct ReplayTexture {
        QSize size;
        QByteArray alpha;
        QPoint origin;
    };
    struct ReplayBatch {
        QVector<KisGpuTileStore::DabCommand> commands;
        QRect clip;
        std::shared_ptr<const ReplaySelection> selection;
        std::shared_ptr<const ReplayTexture> texture;
    };
    using ReplayEdit = QVector<ReplayBatch>;
    struct RecoveryTile {
        QPoint coordinate;
        QByteArray pixels;
    };
    /** CPU checkpoint and accepted input values; independent of GPU handles and session tokens. */
    struct Recovery {
        QRect bounds;
        QByteArray pixels;
        QVector<RecoveryTile> tiles;
        QVector<ReplayEdit> edits;
        qsizetype firstRetained = 0, cursor = 0;
        std::optional<ReplayEdit> working;
        bool commitRequested = false;
    };
    /** Immutable GPU mask paired with its recovery commands; may outlive the session. */
    class Selection {
    public:
        Selection() = default;
        explicit operator bool() const { return bool(source); }
        KisGpuTileStore::Version pixels() const { return version; }
    private:
        friend class KisGpuEditSession;
        KisGpuTileStore::Version version;
        std::shared_ptr<const ReplaySelection> source;
    };
    struct SelectionResult {
        Result result = Result::GpuRejected;
        Selection selection;
    };
    /** Prepared immutable pattern and its CPU recovery source. */
    class Texture {
    public:
        Texture() = default;
        explicit operator bool() const { return bool(source); }
    private:
        friend class KisGpuEditSession;
        KisGpuTileStore::BrushTexture pattern;
        std::shared_ptr<const ReplayTexture> source;
    };
    struct TextureResult {
        Result result = Result::GpuRejected;
        Texture texture;
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
    /** Explicit asynchronous maintenance: fold discarded commands into sparse RGBA8 tiles.
     * Editing and retained Undo/Redo continue. Adoption is atomic after every read succeeds.
     * CPU preflight includes both the existing recovery values and the pending checkpoint.
     */
    Result compactRecovery();
    std::optional<KisGpuTileStore::Status> recoveryCompactionStatus() const;
    /** Rasterizes a new mask from empty on the session's store; earlier masks stay immutable. */
    SelectionResult createSelection(const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip);
    TextureResult createTexture(QSize size, const QByteArray &alpha, QPoint origin = {});
    Result append(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip);
    Result append(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip,
                  const Selection &selection);
    Result append(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip,
                  const Texture &texture, const std::optional<Selection> &selection = {});
    /** Redraw from the committed starting version; previous tentative footprints disappear. */
    Result replace(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip);
    Result replace(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip,
                   const Selection &selection);
    Result replace(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip,
                   const Texture &texture, const std::optional<Selection> &selection = {});
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
    friend class KisGpuLayerDocument;
    /** The document owns the association between this successful head and its CPU recovery values. */
    KisGpuEditSession(KisGpuTileStore &store, const KisGpuTileStore::Version &head,
                      Recovery recovery, quint64 maximumRecoveryBytes);
    bool matches(const Token &token) const;
    Result paint(const Token &token, const QVector<KisGpuTileStore::DabCommand> &commands, QRect clip, bool replace,
                 const Selection *selection = nullptr, const Texture *texture = nullptr);
    void releaseWorkingEdit();
    struct ReplayState;
    void pollRecovery();
    void pollRecoveryCompaction();
    bool canRecord(const ReplayBatch &batch, bool replace) const;
    void record(ReplayBatch batch, bool replace);
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
