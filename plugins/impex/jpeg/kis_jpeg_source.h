/*
 *  SPDX-FileCopyrightText: 2009 Adrian Page <adrian@pagenet.plus.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef _KIS_JPEG_SOURCE_H_
#define _KIS_JPEG_SOURCE_H_

#include <stdio.h>

extern "C" {
  #include <jpeglib.h>
}

class QIODevice;
class QCryptographicHash;

namespace KisJPEGSource
{
    /** The optional digest is borrowed until decompression ends. Successful
     * finish hashes the consumed JPEG codestream, independent of read-ahead.
     */
    void setSource(j_decompress_ptr cinfo, QIODevice* inputDevice, QCryptographicHash *digest = nullptr);
}

#endif
