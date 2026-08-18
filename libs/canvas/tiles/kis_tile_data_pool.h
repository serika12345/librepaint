/*
 * SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TILE_DATA_POOL_H
#define KIS_TILE_DATA_POOL_H

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QSharedPointer>
#include <QVector>

#include <kritacanvas_export.h>

class KisTileDataPoolSingleSize;
class KisTileDataPoolWorker;

/**
 * Reuses fixed-size buffers for canvas tile transfers. A pool owns one
 * allocator for every pixel size requested for its tile dimensions.
 */
class KRITACANVAS_EXPORT KisTileDataPool : public QObject
{
    Q_OBJECT

public:
    KisTileDataPool(int tileWidth, int tileHeight);
    ~KisTileDataPool() override;

    quint8 *allocate(int pixelSize);
    void release(quint8 *data, int pixelSize);
    int chunkSize(int pixelSize) const;
    void tryPurge(int pixelSize, int releaseCount);

Q_SIGNALS:
    void purgeRequested(int pixelSize, int releaseCount);

private:
    mutable QMutex m_mutex;
    const int m_tileWidth;
    const int m_tileHeight;
    QVector<KisTileDataPoolSingleSize *> m_pools;
    KisTileDataPoolWorker *m_worker {nullptr};
};

using KisTileDataPoolSP = QSharedPointer<KisTileDataPool>;

class KRITACANVAS_EXPORT KisTileDataPoolRegistry
{
public:
    KisTileDataPoolSP poolForTileSize(int tileWidth, int tileHeight);

private:
    using PoolId = QPair<int, int>;
    using KisTileDataPoolWSP = QWeakPointer<KisTileDataPool>;

    QMutex m_mutex;
    QHash<PoolId, KisTileDataPoolWSP> m_storage;
};

#endif
