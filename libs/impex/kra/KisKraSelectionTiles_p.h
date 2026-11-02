/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_SELECTION_TILES_P_H
#define KIS_KRA_SELECTION_TILES_P_H
#include "KisKraRasterCodec.h"

namespace LibrePaint::ImportExport::Kra::Detail {
/** Reads an alpha8 KRA pixel-selection stream. Fully selected or unselected
 * pixels are adopted as one rectangle, one ellipse, or a binary frozen
 * coverage mask when neither shape describes them. Fractional alpha values are
 * adopted as a frozen alpha coverage mask so feathered selections survive.
 * Finite arbitrary coverage retains coordinates outside the canvas; inversion
 * is classified relative to the supplied image bounds. A default of 255 stores
 * finite holes as inverted selection geometry or coverage.
 */
Outcome readSelectionTiles(const QByteArray &data, quint8 defaultPixel, QPoint offset,
                           QRect imageBounds, qsizetype maximumBytes, QRect &selection, bool &inverted,
                           LibrePaint::Image::Gpu::SelectionKind &kind,
                           LibrePaint::Image::Gpu::SelectionMaskSP &mask);
/** Writes a rectangular, elliptical, binary or alpha arbitrary-coverage
 * selection or its complement as an alpha8 KRA selection stream. Alpha masks
 * retain their fractional selectedness and finite off-canvas pixels. The caller
 * stores the default pixel as 255 for complements and zero for finite coverage.
 */
Outcome writeSelectionTiles(const QRect &selection, bool inverted,
                            LibrePaint::Image::Gpu::SelectionKind kind,
                            const LibrePaint::Image::Gpu::SelectionMaskSP &mask,
                            qsizetype maximumBytes, QByteArray &output);
/** Reads one KRA alpha8 pixel-selection stream as layer transparency coverage.
 * Coordinates are shifted by `offset` and retained across the supported layer
 * domain, including off-canvas coverage. An empty stream returns a retained
 * zero-coverage mask, distinct from the absence of a layer mask.
 */
Outcome readTransparencyMask(const QByteArray &data, QPoint offset, QRect imageBounds,
                             qsizetype maximumBytes,
                             LibrePaint::Image::Gpu::SelectionMaskSP &mask, quint8 defaultCoverage = 0);
/** Writes layer transparency coverage as one KRA alpha8 pixel-selection stream. */
Outcome writeTransparencyMask(const LibrePaint::Image::Gpu::SelectionMaskSP &mask,
                              const QRect &imageBounds, qsizetype maximumBytes,
                              QByteArray &output, const QPoint &offset);
}
#endif
