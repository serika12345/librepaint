/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_SELECTION_MASK_H
#define KIS_GPU_SELECTION_MASK_H

#include "KisGpuRasterPixels.h"
#include "KisGpuAffineTransform.h"
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
#include <memory>

namespace LibrePaint::Image::Gpu {

struct RegionFill;
namespace Detail { struct SelectionPixels; }

/** Tile coordinate key (x, y) for packed coverage tiles. */
using SelectionTileKey = QPair<int, int>;
using SelectionTileMap = QMap<SelectionTileKey, QByteArray>;

/** Native pixel-selection arithmetic, including fractional coverage. */
enum class SelectionCombination : quint8 { Replace, Add, Subtract, Intersect, SymmetricDifference };

class SelectionMask;
/** Frozen inputs for the ordinary selection tools' Alpha8 arithmetic. */
struct SelectionCombinationSource
{
    QSharedPointer<const SelectionMask> first, second;
    SelectionCombination action = SelectionCombination::Replace;
    QRect extent;
    bool inverted = false;
};

/** Frozen input for moving a generated selection without reading its pixels. */
struct SelectionTranslationSource
{
    QSharedPointer<const SelectionMask> source;
    QPoint offset;
    QRect extent;
};

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

    /** Explicit zero coverage for masked external copies; null denotes no mask. */
    static QSharedPointer<const SelectionMask> empty();

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
    /** Adopts byte-per-pixel alpha tiles with the same coordinate scheme.
     * Values are retained; nonzero bytes form the selected bounds.
     */
    static QSharedPointer<const SelectionMask> fromAlphaTiles(const SelectionTileMap &tiles);
    /** Frozen GPU-generated coverage. Only its exact bounds and display/hit
     * geometry are CPU values; editing and external pixel readers use the
     * retained GPU source. Invalid geometry or capacity returns null.
     */
    bool isGpuGenerated() const { return !m_region.isNull() || !m_combination.isNull() || !m_translation.isNull(); }
    const QSharedPointer<const RegionFill> &regionSource() const { return m_region; }
    const QSharedPointer<const SelectionCombinationSource> &combinationSource() const { return m_combination; }
    const QSharedPointer<const SelectionTranslationSource> &translationSource() const { return m_translation; }
    /** Threshold-128 row spans for display and selection-tool hit testing.
     * These vectors do not supply editing coverage or saved pixels.
     */
    const QList<QRect> &outlineRuns() const { return m_outlineRuns; }
    /** Combines frozen input coverage inside `clip`. Addition saturates,
     * subtraction clamps at zero, intersection takes the minimum and symmetric
     * difference takes the absolute difference, matching native Alpha8 selection.
     * Empty coverage is a valid value; null reports invalid input or capacity.
     * Inversion complements the existing coverage inside the document bounds.
     */
    QSharedPointer<const SelectionMask> combined(const SelectionMask &other, SelectionCombination action,
                                                const QRect &clip, bool inverted = false) const;
    /** Translates the coverage by `delta`, clipping it to `clip`. Alpha masks
     * retain their fractional values. Valid fully clipped literal input returns
     * an empty mask; null reports invalid input, GPU input or capacity. */
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

    /** Applies frozen native Alpha8 filter coefficients to retained selection
     * coverage. Inverted resources transform their finite holes with the same
     * plan. Null reports invalid input or capacity; empty() is valid zero.
     */
    QSharedPointer<const SelectionMask> transformed(const AffinePlan &plan) const;
    /** Alpha8 normal capture and clear/merge used by selected mask transforms.
     * Null reports capacity; empty() retains a valid fully transparent result.
     */
    QSharedPointer<const SelectionMask> selectedCopy(const SelectionMask &selection, bool inverted, QRect copyRect) const;
    QSharedPointer<const SelectionMask> replaceSelectedPixels(const SelectionMask &selection, bool inverted,
                                                             QRect clearRect, const SelectionMask &replacement) const;

    bool isEmpty() const { return m_bounds.isEmpty(); }
    /** Tight bounds of the pixels with nonzero coverage. */
    QRect bounds() const { return m_bounds; }
    /** Literal input selectedness; 0 for binary unselected, 255 for binary
     * selected, and the stored byte for alpha coverage. GPU-generated masks
     * require Execution::readSelectionPixels(); direct CPU access throws.
     */
    quint8 coverage(int x, int y) const;
    bool contains(int x, int y) const { return coverage(x, y) != 0; }
    bool isAlpha() const { return m_format == CoverageFormat::Alpha; }
    /** Literal input tiles in increasing coordinate order; GPU sources retain
     * their native pixels separately. Bit zero is the top-left
     * pixel of each binary tile and byte zero the top-left pixel of each alpha
     * tile. Missing tiles are unselected.
     */
    const SelectionTileMap &tiles() const { return m_tiles; }
    /** Bytes per stored tile for this coverage format. */
    qsizetype tileBytes() const { return m_format == CoverageFormat::Alpha ? AlphaTileBytes : TileBytes; }
    qsizetype byteCount() const { return m_tiles.size() * tileBytes() + m_outlineRuns.size() * qsizetype(sizeof(QRect)); }
    bool operator==(const SelectionMask &other) const;
    bool operator!=(const SelectionMask &other) const { return !(*this == other); }

private:
    friend class Execution;
    friend class TileRenderer;
    static QSharedPointer<const SelectionMask> fromRegion(const QSharedPointer<const RegionFill> &source,
        const QRect &bounds, const QList<QRect> &outlineRuns, const std::shared_ptr<Detail::SelectionPixels> &pixels);
    static QSharedPointer<const SelectionMask> fromCombination(const QSharedPointer<const SelectionCombinationSource> &source,
        const QRect &bounds, const QList<QRect> &outlineRuns, const std::shared_ptr<Detail::SelectionPixels> &pixels);
    static QSharedPointer<const SelectionMask> fromTranslation(const QSharedPointer<const SelectionTranslationSource> &source,
        const QRect &bounds, const QList<QRect> &outlineRuns, const std::shared_ptr<Detail::SelectionPixels> &pixels);
    QRect nativeExtent() const;
    const std::shared_ptr<Detail::SelectionPixels> &nativePixels() const { return m_nativePixels; }
    enum class CoverageFormat : quint8 { Binary, Alpha };
    SelectionMask(QRect bounds, SelectionTileMap tiles, CoverageFormat format);
    static QSharedPointer<const SelectionMask> make(QRect bounds, SelectionTileMap tiles, CoverageFormat format);
    QRect m_bounds;
    SelectionTileMap m_tiles;
    CoverageFormat m_format = CoverageFormat::Binary;
    QSharedPointer<const RegionFill> m_region;
    QSharedPointer<const SelectionCombinationSource> m_combination;
    QSharedPointer<const SelectionTranslationSource> m_translation;
    QList<QRect> m_outlineRuns;
    std::shared_ptr<Detail::SelectionPixels> m_nativePixels;
};

using SelectionMaskSP = QSharedPointer<const SelectionMask>;

}
#endif
