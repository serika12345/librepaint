/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tiff_jpeg_buffer.h"

#include <cstring>

namespace
{
bool prepareJpegBuffer(QVector<unsigned char> *jpegBuffer,
                       tmsize_t bytesRead,
                       const uint8_t *tables,
                       uint32_t tablesSize)
{
    if (bytesRead < 2 || bytesRead > jpegBuffer->size()) {
        return false;
    }

    jpegBuffer->resize(bytesRead);
    if (!tables || tablesSize < 4) {
        return true;
    }

    const bool tablesHaveMarkers = tables[0] == 0xff && tables[1] == 0xd8 &&
        tables[tablesSize - 2] == 0xff && tables[tablesSize - 1] == 0xd9;
    const bool imageHasStartMarker = (*jpegBuffer)[0] == 0xff && (*jpegBuffer)[1] == 0xd8;
    if (!tablesHaveMarkers || !imageHasStartMarker) {
        return false;
    }

    QVector<unsigned char> completeJpeg(tablesSize + bytesRead - 4);
    std::memcpy(completeJpeg.data(), tables, tablesSize - 2);
    std::memcpy(completeJpeg.data() + tablesSize - 2,
                jpegBuffer->constData() + 2,
                bytesRead - 2);
    *jpegBuffer = std::move(completeJpeg);
    return true;
}
}

bool readTiffJpegTile(TIFF *image,
                      uint32_t tile,
                      tmsize_t bufferSize,
                      const uint8_t *tables,
                      uint32_t tablesSize,
                      QVector<unsigned char> *jpegBuffer)
{
    jpegBuffer->resize(bufferSize);
    return prepareJpegBuffer(jpegBuffer,
                             TIFFReadRawTile(image, tile, jpegBuffer->data(), bufferSize),
                             tables,
                             tablesSize);
}

bool readTiffJpegStrip(TIFF *image,
                       uint32_t strip,
                       tmsize_t bufferSize,
                       const uint8_t *tables,
                       uint32_t tablesSize,
                       QVector<unsigned char> *jpegBuffer)
{
    jpegBuffer->resize(bufferSize);
    return prepareJpegBuffer(jpegBuffer,
                             TIFFReadRawStrip(image, strip, jpegBuffer->data(), bufferSize),
                             tables,
                             tablesSize);
}
