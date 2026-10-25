/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_SELECTION_TILES_P_H
#define KIS_KRA_SELECTION_TILES_P_H
#include "KisKraRasterCodec.h"

namespace LibrePaint::ImportExport::Kra::Detail {
/** Reads an alpha8 KRA pixel-selection stream. A selection is accepted only
 * when every stored value is fully selected or unselected and the selected
 * pixels form one rectangle. Other selections need a coverage value.
 */
Outcome readSelectionTiles(const QByteArray &data, quint8 defaultPixel, QPoint offset,
                           QRect imageBounds, qsizetype maximumBytes, QRect &selection, bool &inverted,
                           LibrePaint::Image::Gpu::SelectionKind &kind);
/** Writes a rectangular or elliptical selection or its complement as an alpha8 KRA selection stream. */
Outcome writeSelectionTiles(const QRect &selection, bool inverted, const QRect &imageBounds,
                            LibrePaint::Image::Gpu::SelectionKind kind, qsizetype maximumBytes, QByteArray &output);
}
#endif
