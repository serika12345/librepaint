/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_RGBA_TILES_P_H
#define KIS_KRA_RGBA_TILES_P_H
#include "KisKraRasterCodec.h"

namespace LibrePaint::ImportExport::Kra::Detail {
QByteArray rgba(const std::array<quint8, 4> &pixel);
QByteArray bgra(const std::array<quint8, 4> &pixel);
Outcome readTiles(const QByteArray &data, const std::array<quint8, 4> &defaultPixel,
                  QPoint offset, qsizetype maximumBytes, QList<Image::Gpu::RgbaTile> &output,
                  QList<QRect> *writtenAreas = nullptr);
Outcome writeTiles(const QList<Image::Gpu::RgbaTile> &tiles, const std::array<quint8, 4> &defaultPixel,
                   QPoint offset, qsizetype maximumBytes, QByteArray &output, const QList<QRect> &writtenAreas = {});
}
#endif
