/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_EDIT_REPLAY_P_H
#define KIS_GPU_EDIT_REPLAY_P_H
#include "KisGpuEditSession.h"
struct KisGpuEditSession::ReplayState {
    std::optional<Recovery> data;
    ReplayEdit working;
    quint64 maximumBytes = 64 * 1024 * 1024;
    bool commitRequested = false;
    std::optional<Recovery> restoring;
    KisGpuTileStore::Version replay;
    KisGpuTileStore::Version selection;
    bool selectionReady = false;
    KisGpuTileStore::BrushTexture texture;
    bool textureReady = false;
    KisGpuTileStore::Edit latest;
    QVector<KisGpuTileStore::Version> frames;
    qsizetype edit = 0, batch = 0;
    bool initialized = false, workingPhase = false, waiting = false;
};
#endif
