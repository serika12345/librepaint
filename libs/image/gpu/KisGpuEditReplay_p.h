/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_EDIT_REPLAY_P_H
#define KIS_GPU_EDIT_REPLAY_P_H
#include "KisGpuEditSession.h"
struct KisGpuEditSession::ReplayState {
    struct Compaction {
        KisGpuTileStore::Version source;
        QVector<QPoint> coordinates;
        QVector<RecoveryTile> tiles;
        KisGpuTileStore::Readback latest;
        qsizetype firstRetained = 0;
        quint64 reservedBytes = 0;
        bool waiting = false;
    };
    std::optional<Recovery> data;
    ReplayEdit working;
    quint64 maximumBytes = 64 * 1024 * 1024;
    bool commitRequested = false;
    std::optional<Compaction> compacting;
    std::optional<KisGpuTileStore::Status> compactionStatus;
    std::optional<Recovery> restoring;
    KisGpuTileStore::Version replay;
    KisGpuTileStore::Version selection;
    bool selectionReady = false;
    KisGpuTileStore::BrushTexture texture;
    bool textureReady = false;
    KisGpuTileStore::Edit latest;
    QVector<KisGpuTileStore::Version> frames;
    qsizetype edit = 0, batch = 0, tile = 0;
    bool initialized = false, initialPixelsReady = false, workingPhase = false, waiting = false;
    static quint64 recoveryBytes(const Recovery &data, const ReplayEdit *working = nullptr);
    static bool validRecovery(const Recovery &data, qsizetype retainedEdits, quint64 maximumBytes);
    static std::optional<QRect> tileBounds(QPoint coordinate);
};
#endif
