/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_LAYER_PROJECTION_H
#define KIS_GPU_LAYER_PROJECTION_H

#include "KisGpuTileStore.h"

/**
 * Owns the ordered layer inputs and their last successful projection inside
 * fixed canvas bounds. The borrowed store outlives this owner; all calls use
 * its submitting thread. Snapshots may outlive both owners.
 */
class KisGpuLayerProjection
{
public:
    struct Snapshot {
        QVector<KisGpuTileStore::Layer> layers;
        KisGpuTileStore::Version pixels;
    };
    /** Empty or unrepresentable canvas bounds throw std::invalid_argument. */
    KisGpuLayerProjection(KisGpuTileStore &store, QRect bounds);
    KisGpuLayerProjection(const KisGpuLayerProjection &) = delete;
    KisGpuLayerProjection &operator=(const KisGpuLayerProjection &) = delete;

    /**
     * Replaces bottom-to-top layer inputs, deriving damage from GPU tile identities.
     * Pending inputs are accepted in queue order. Rejection preserves accepted
     * inputs and the published snapshot. Only the latest accepted result is
     * published, atomically with its layer inputs, by poll() after success.
     */
    KisGpuTileStore::Error setLayers(const QVector<KisGpuTileStore::Layer> &layers);
    void poll();
    KisGpuTileStore::Status status() const { return m_completion.status(); }
    Snapshot snapshot() const { return m_published; }
    QRect bounds() const { return m_bounds; }

private:
    QRect damage(const QVector<KisGpuTileStore::Layer> &before,
                 const QVector<KisGpuTileStore::Layer> &after) const;
    KisGpuTileStore &m_store;
    QRect m_bounds;
    Snapshot m_published, m_requested;
    KisGpuTileStore::Completion m_completion;
};

#endif
