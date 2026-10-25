/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_SELECTION_MASK_H
#define KIS_GPU_SELECTION_MASK_H

#include "KisGpuRasterPixels.h"
#include <kritaimagegpuoperations_export.h>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QPair>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSharedPointer>
#include <QSize>

namespace LibrePaint::Image::Gpu {

/** Tile coordinate key (x, y) for packed coverage tiles. */
using SelectionTileKey = QPair<int, int>;
using SelectionTileMap = QMap<SelectionTileKey, QByteArray>;

/**
 * Frozen hard-edged document coverage for an arbitrary selection.
 *
 * Coverage uses one bit per document pixel, packed in 256 x 256 tiles whose
 * coordinates match GPU paint tiles. Set bits select the pixel. The rectangle
 * bounds the selected pixels, so sparse masks retain only touched tiles. The
 * value is immutable and shared by document versions, drawing operations and
 * history states; consumers never synchronize it per stroke.
 */
class KRITAIMAGEGPUOPERATIONS_EXPORT SelectionMask
{
public:
    static constexpr int TileSize = RgbaTile::Size;
    static constexpr int TileBytes = TileSize * TileSize / 8;
    static constexpr qsizetype MaximumBytes = 64 * 1024 * 1024;

    /** Rasterizes one closed polygon at pixel centers with the even-odd rule.
     * The coverage is clipped to `clip`. Invalid, non-finite or oversized
     * polygons and polygons selecting no clipped pixel return null.
     */
    static QSharedPointer<const SelectionMask> fromPolygon(const QList<QPointF> &points, const QRect &clip);
    /** Adopts row-major one-byte coverage (zero unselected, nonzero selected)
     * for `bounds`, clipped to `clip`. Null reports invalid or empty input.
     */
    static QSharedPointer<const SelectionMask> fromCoverage(const QRect &bounds, const QByteArray &coverage,
                                                            const QRect &clip);
    /** Adopts bit-packed tiles. Tile coordinates use TileSize units. The
     * selected bounds are derived from the bits. Null reports invalid,
     * repeated or empty tiles.
     */
    static QSharedPointer<const SelectionMask> fromTiles(const SelectionTileMap &tiles);
    /** Translates the selected pixels by `delta`, clipping them to `clip`. */
    QSharedPointer<const SelectionMask> translated(QPoint delta, const QRect &clip) const;
    /** Returns the complement inside `clip`; null means no selected pixels. */
    QSharedPointer<const SelectionMask> inverted(const QRect &clip) const;
    /** Expands or contracts the coverage by a rectangular radius, clipped to `clip`. */
    QSharedPointer<const SelectionMask> grown(int xRadius, int yRadius, const QRect &clip) const;
    QSharedPointer<const SelectionMask> shrunk(int xRadius, int yRadius, const QRect &clip) const;
    /** Returns the pixels selected here but not in `other`, clipped to `clip`. */
    QSharedPointer<const SelectionMask> subtracted(const SelectionMask &other, const QRect &clip) const;
    /** Returns the inside border band of the given rectangular radius. */
    QSharedPointer<const SelectionMask> border(int xRadius, int yRadius, const QRect &clip) const;

    bool isEmpty() const { return m_bounds.isEmpty(); }
    /** Tight bounds of the selected pixels. */
    QRect bounds() const { return m_bounds; }
    bool contains(int x, int y) const;
    /** Bit-packed tiles in increasing coordinate order; bit zero is the
     * top-left pixel of each tile. Missing tiles are unselected.
     */
    const SelectionTileMap &tiles() const { return m_tiles; }
    qsizetype byteCount() const { return m_tiles.size() * TileBytes; }
    bool operator==(const SelectionMask &other) const;
    bool operator!=(const SelectionMask &other) const { return !(*this == other); }

private:
    SelectionMask(QRect bounds, SelectionTileMap tiles);
    static QSharedPointer<const SelectionMask> make(QRect bounds, SelectionTileMap tiles);
    QRect m_bounds;
    SelectionTileMap m_tiles;
};

using SelectionMaskSP = QSharedPointer<const SelectionMask>;

}
#endif
