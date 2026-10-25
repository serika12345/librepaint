/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_SELECTION_TILES_P_H
#define KIS_KRA_SELECTION_TILES_P_H
#include "KisKraRasterCodec.h"

namespace LibrePaint::ImportExport::Kra::Detail {
/** Reads an alpha8 KRA pixel-selection stream. Fully selected or unselected
 * pixels are adopted as one rectangle, one ellipse, or an arbitrary frozen
 * coverage mask when neither shape describes them.
 */
Outcome readSelectionTiles(const QByteArray &data, quint8 defaultPixel, QPoint offset,
                           QRect imageBounds, qsizetype maximumBytes, QRect &selection, bool &inverted,
                           LibrePaint::Image::Gpu::SelectionKind &kind,
                           LibrePaint::Image::Gpu::SelectionMaskSP &mask);
/** Writes a rectangular, elliptical or arbitrary-coverage selection or its
 * complement as an alpha8 KRA selection stream.
 */
Outcome writeSelectionTiles(const QRect &selection, bool inverted, const QRect &imageBounds,
                            LibrePaint::Image::Gpu::SelectionKind kind,
                            const LibrePaint::Image::Gpu::SelectionMaskSP &mask,
                            qsizetype maximumBytes, QByteArray &output);
}
#endif
