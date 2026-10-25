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
 * Frozen document coverage for an arbitrary selection.
 *
 * Binary coverage uses one bit per document pixel, packed in 256 x 256 tiles
 * whose coordinates match GPU paint tiles. Soft coverage uses one byte per
 * pixel inside tiles of the same size, so feathered selections retain
 * fractional selectedness. Set bits and nonzero bytes select the pixel. The
 * rectangle bounds the pixels with nonzero coverage, so sparse masks retain
 * only touched tiles. The value is immutable and shared by document versions,
 * drawing operations and history states; consumers never synchronize it per
 * stroke.
 */
class KRITAIMAGEGPUOPERATIONS_EXPORT SelectionMask
{
public:
    static constexpr int TileSize = RgbaTile::Size;
    static constexpr int TileBytes = TileSize * TileSize / 8;
    static constexpr int AlphaTileBytes = TileSize * TileSize;
    static constexpr qsizetype MaximumBytes = 64 * 1024 * 1024;

    /** Binary tiles select a pixel; alpha tiles retain 0..255 selectedness. */
    enum class CoverageFormat : quint8 { Binary, Alpha };

    /** Rasterizes one closed polygon at pixel centers with the even-odd rule.
     * The coverage is binary and clipped to `clip`. Invalid, non-finite or
     * oversized polygons and polygons selecting no clipped pixel return null.
     */
    static QSharedPointer<const SelectionMask> fromPolygon(const QList<QPointF> &points, const QRect &clip);
    /** Adopts row-major one-byte coverage (zero unselected, nonzero selected)
     * for `bounds`, clipped to `clip`. The result is binary. Null reports
     * invalid or empty input.
     */
    static QSharedPointer<const SelectionMask> fromCoverage(const QRect &bounds, const QByteArray &coverage,
                                                            const QRect &clip);
    /** Adopts row-major one-byte selectedness (0 unselected, 255 fully
     * selected) for `bounds`, clipped to `clip`. The result retains fractional
     * values as an alpha mask. Null reports invalid or empty input.
     */
    static QSharedPointer<const SelectionMask> fromAlphaCoverage(const QRect &bounds, const QByteArray &coverage,
                                                                 const QRect &clip);
    /** Rasterizes a filled rectangle with binary coverage clipped to `clip`.
     * Empty rectangles and rectangles entirely outside `clip` return null.
     */
    static QSharedPointer<const SelectionMask> fromRectangle(const QRect &rect, const QRect &clip);
    /** Rasterizes a filled ellipse centered in `rect` at pixel centers with
     * binary coverage clipped to `clip`. Empty or fully clipped shapes return
     * null.
     */
    static QSharedPointer<const SelectionMask> fromEllipse(const QRect &rect, const QRect &clip);
    /** Adopts bit-packed tiles. Tile coordinates use TileSize units. The
     * selected bounds are derived from the bits. Null reports invalid,
     * repeated or empty tiles.
     */
    static QSharedPointer<const SelectionMask> fromTiles(const SelectionTileMap &tiles);
    /** Adopts byte-per-pixel alpha tiles with the same coordinate scheme.
     * Values are retained; nonzero bytes form the selected bounds.
     */
    static QSharedPointer<const SelectionMask> fromAlphaTiles(const SelectionTileMap &tiles);
    /** Translates the coverage by `delta`, clipping it to `clip`. Alpha masks
     * retain their fractional values. */
    QSharedPointer<const SelectionMask> translated(QPoint delta, const QRect &clip) const;
    /** Returns the complement inside `clip`; null means nothing is left. */
    QSharedPointer<const SelectionMask> inverted(const QRect &clip) const;
    /** Expands or contracts the coverage by a rectangular radius, clipped to `clip`.
     * Alpha masks use a maximum or minimum filter and retain fractional values.
     */
    QSharedPointer<const SelectionMask> grown(int xRadius, int yRadius, const QRect &clip) const;
    QSharedPointer<const SelectionMask> shrunk(int xRadius, int yRadius, const QRect &clip) const;
    /** Returns the coverage selected here but not in `other`, clipped to `clip`. */
    QSharedPointer<const SelectionMask> subtracted(const SelectionMask &other, const QRect &clip) const;
    /** Returns the inside border band of the given rectangular radius. */
    QSharedPointer<const SelectionMask> border(int xRadius, int yRadius, const QRect &clip) const;
    /** Softens the coverage with a separable box radius. Binary coverage gains
     * fractional edge values; alpha coverage is blurred in place. The result is
     * clipped to `clip`. A zero radius or empty coverage returns null.
     */
    QSharedPointer<const SelectionMask> feathered(int radius, const QRect &clip) const;

    bool isEmpty() const { return m_bounds.isEmpty(); }
    /** Tight bounds of the pixels with nonzero coverage. */
    QRect bounds() const { return m_bounds; }
    /** Selectedness at a document pixel; 0 for binary unselected, 255 for
     * binary selected, and the stored byte for alpha coverage. */
    quint8 coverage(int x, int y) const;
    bool contains(int x, int y) const { return coverage(x, y) != 0; }
    CoverageFormat format() const { return m_format; }
    bool isAlpha() const { return m_format == CoverageFormat::Alpha; }
    /** Packed tiles in increasing coordinate order; bit zero is the top-left
     * pixel of each binary tile and byte zero the top-left pixel of each alpha
     * tile. Missing tiles are unselected.
     */
    const SelectionTileMap &tiles() const { return m_tiles; }
    /** Bytes per stored tile for this coverage format. */
    qsizetype tileBytes() const { return m_format == CoverageFormat::Alpha ? AlphaTileBytes : TileBytes; }
    qsizetype byteCount() const { return m_tiles.size() * tileBytes(); }
    bool operator==(const SelectionMask &other) const;
    bool operator!=(const SelectionMask &other) const { return !(*this == other); }

private:
    SelectionMask(QRect bounds, SelectionTileMap tiles, CoverageFormat format);
    static QSharedPointer<const SelectionMask> make(QRect bounds, SelectionTileMap tiles, CoverageFormat format);
    QRect m_bounds;
    SelectionTileMap m_tiles;
    CoverageFormat m_format = CoverageFormat::Binary;
};

using SelectionMaskSP = QSharedPointer<const SelectionMask>;

}
#endif
