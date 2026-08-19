/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TIFF_JPEG_BUFFER_H
#define KIS_TIFF_JPEG_BUFFER_H

#include <QVector>

#include <cstdint>
#include <tiffio.h>

bool readTiffJpegTile(TIFF *image,
                      uint32_t tile,
                      tmsize_t bufferSize,
                      const uint8_t *tables,
                      uint32_t tablesSize,
                      QVector<unsigned char> *jpegBuffer);

bool readTiffJpegStrip(TIFF *image,
                       uint32_t strip,
                       tmsize_t bufferSize,
                       const uint8_t *tables,
                       uint32_t tablesSize,
                       QVector<unsigned char> *jpegBuffer);

#endif
