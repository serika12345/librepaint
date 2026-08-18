/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TILE_DATA_BUFFER_H
#define KIS_TILE_DATA_BUFFER_H

#include <utility>

#include <QtGlobal>

#include "kis_tile_data_pool.h"

/** Owns one pooled buffer used to transfer a canvas tile. */
class KRITACANVAS_EXPORT KisTileDataBuffer
{
public:
    explicit KisTileDataBuffer(KisTileDataPoolSP pool)
        : m_pool(std::move(pool))
    {
    }

    KisTileDataBuffer(int pixelSize, KisTileDataPoolSP pool)
        : m_pool(std::move(pool))
    {
        allocate(pixelSize);
    }

    KisTileDataBuffer(KisTileDataBuffer &&other) noexcept
        : m_data(other.m_data)
        , m_pixelSize(other.m_pixelSize)
        , m_pool(std::move(other.m_pool))
    {
        other.m_data = nullptr;
        other.m_pixelSize = 0;
    }

    KisTileDataBuffer &operator=(KisTileDataBuffer &&other) noexcept
    {
        swap(other);
        return *this;
    }

    ~KisTileDataBuffer()
    {
        if (m_data) {
            m_pool->release(m_data, m_pixelSize);
        }
    }

    void allocate(int pixelSize)
    {
        Q_ASSERT(!m_data);
        Q_ASSERT(m_pool);
        m_pixelSize = pixelSize;
        m_data = m_pool->allocate(pixelSize);
    }

    quint8 *data() const { return m_data; }
    int size() const { return m_data ? m_pool->chunkSize(m_pixelSize) : 0; }
    int pixelSize() const { return m_pixelSize; }
    KisTileDataPoolSP pool() const { return m_pool; }

    void swap(KisTileDataBuffer &other) noexcept
    {
        std::swap(m_data, other.m_data);
        std::swap(m_pixelSize, other.m_pixelSize);
        std::swap(m_pool, other.m_pool);
    }

private:
    Q_DISABLE_COPY(KisTileDataBuffer)

    quint8 *m_data {nullptr};
    int m_pixelSize {0};
    KisTileDataPoolSP m_pool;
};

#endif
