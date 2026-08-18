/*
 * SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2017 Bernhard Liebl <poke1024@gmx.de>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tile_data_pool.h"

#include <boost/pool/pool.hpp>

#include <QCoreApplication>
#include <QMap>
#include <QMutexLocker>
#include <QThread>

#include <kis_assert.h>
#include <kis_shared_ptr.h>
#include <kis_signal_compressor.h>

namespace
{
constexpr int minimumPoolChunks = 32;
constexpr int maximumPoolChunks = 128;
constexpr int purgeThreshold = 64;
}

class KisTileDataPoolSingleSize
{
public:
    KisTileDataPoolSingleSize(int tileWidth, int tileHeight, int pixelSize);

    quint8 *allocate();
    bool release(quint8 *data);
    int chunkSize() const;
    int releaseCount() const;
    void tryPurge(int releaseCount);

private:
    const int m_chunkSize;
    boost::pool<boost::default_user_allocator_new_delete> m_pool;
    int m_allocationCount {0};
    int m_maxAllocationCount {0};
    int m_releaseCount {0};
};

class KisTileDataPoolWorker : public QObject
{
public:
    explicit KisTileDataPoolWorker(KisTileDataPool *pool);

    void schedulePurge(int pixelSize, int releaseCount);
    void purge();

private:
    KisTileDataPool *m_pool;
    KisSignalCompressor m_compressor;
    QMap<int, int> m_pendingPurges;
};

KisTileDataPoolSingleSize::KisTileDataPoolSingleSize(int tileWidth, int tileHeight, int pixelSize)
    : m_chunkSize(tileWidth * tileHeight * pixelSize)
    , m_pool(m_chunkSize, minimumPoolChunks, maximumPoolChunks)
{
}

quint8 *KisTileDataPoolSingleSize::allocate()
{
    ++m_allocationCount;
    m_maxAllocationCount = qMax(m_maxAllocationCount, m_allocationCount);
    return static_cast<quint8 *>(m_pool.malloc());
}

bool KisTileDataPoolSingleSize::release(quint8 *data)
{
    --m_allocationCount;
    ++m_releaseCount;
    m_pool.free(data);
    KIS_ASSERT_RECOVER_NOOP(m_allocationCount >= 0);
    return !m_allocationCount && m_maxAllocationCount > purgeThreshold;
}

int KisTileDataPoolSingleSize::chunkSize() const
{
    return m_chunkSize;
}

int KisTileDataPoolSingleSize::releaseCount() const
{
    return m_releaseCount;
}

void KisTileDataPoolSingleSize::tryPurge(int releaseCount)
{
    if (releaseCount == m_releaseCount && !m_allocationCount) {
        m_pool.purge_memory();
        m_maxAllocationCount = 0;
    }
}

KisTileDataPoolWorker::KisTileDataPoolWorker(KisTileDataPool *pool)
    : m_pool(pool)
    , m_compressor(1000, KisSignalCompressor::POSTPONE)
{
    connect(&m_compressor, &KisSignalCompressor::timeout,
            this, &KisTileDataPoolWorker::purge);
}

void KisTileDataPoolWorker::schedulePurge(int pixelSize, int releaseCount)
{
    m_pendingPurges[pixelSize] = releaseCount;
    m_compressor.start();
}

void KisTileDataPoolWorker::purge()
{
    for (auto it = m_pendingPurges.constBegin(); it != m_pendingPurges.constEnd(); ++it) {
        m_pool->tryPurge(it.key(), it.value());
    }
    m_pendingPurges.clear();
}

KisTileDataPool::KisTileDataPool(int tileWidth, int tileHeight)
    : m_tileWidth(tileWidth)
    , m_tileHeight(tileHeight)
    , m_worker(new KisTileDataPoolWorker(this))
{
    QThread *applicationThread = QCoreApplication::instance()
        ? QCoreApplication::instance()->thread()
        : QThread::currentThread();
    m_worker->moveToThread(applicationThread);
    connect(this, &KisTileDataPool::purgeRequested,
            m_worker, &KisTileDataPoolWorker::schedulePurge);
}

KisTileDataPool::~KisTileDataPool()
{
    delete m_worker;
    qDeleteAll(m_pools);
}

quint8 *KisTileDataPool::allocate(int pixelSize)
{
    QMutexLocker locker(&m_mutex);
    if (m_pools.size() <= pixelSize) {
        m_pools.resize(pixelSize + 1);
    }
    if (!m_pools[pixelSize]) {
        m_pools[pixelSize] = new KisTileDataPoolSingleSize(m_tileWidth, m_tileHeight, pixelSize);
    }
    return m_pools[pixelSize]->allocate();
}

void KisTileDataPool::release(quint8 *data, int pixelSize)
{
    QMutexLocker locker(&m_mutex);
    KIS_ASSERT_RECOVER_RETURN(pixelSize >= 0 && pixelSize < m_pools.size());
    KisTileDataPoolSingleSize *pool = m_pools[pixelSize];
    KIS_ASSERT_RECOVER_RETURN(pool);
    if (pool->release(data)) {
        Q_EMIT purgeRequested(pixelSize, pool->releaseCount());
    }
}

int KisTileDataPool::chunkSize(int pixelSize) const
{
    QMutexLocker locker(&m_mutex);
    KIS_ASSERT_RECOVER_RETURN_VALUE(pixelSize >= 0 && pixelSize < m_pools.size(), 0);
    KIS_ASSERT_RECOVER_RETURN_VALUE(m_pools[pixelSize], 0);
    return m_pools[pixelSize]->chunkSize();
}

void KisTileDataPool::tryPurge(int pixelSize, int releaseCount)
{
    QMutexLocker locker(&m_mutex);
    KIS_ASSERT_RECOVER_RETURN(pixelSize >= 0 && pixelSize < m_pools.size());
    KIS_ASSERT_RECOVER_RETURN(m_pools[pixelSize]);
    m_pools[pixelSize]->tryPurge(releaseCount);
}

KisTileDataPoolSP KisTileDataPoolRegistry::poolForTileSize(int tileWidth, int tileHeight)
{
    QMutexLocker locker(&m_mutex);
    const PoolId id(tileWidth, tileHeight);
    KisTileDataPoolSP pool = m_storage[id].toStrongRef();
    if (!pool) {
        pool = KisTileDataPoolSP(new KisTileDataPool(tileWidth, tileHeight));
        m_storage[id] = pool;
    }
    return pool;
}
