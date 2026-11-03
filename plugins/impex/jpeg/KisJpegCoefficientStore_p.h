/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_JPEG_COEFFICIENT_STORE_P_H
#define KIS_JPEG_COEFFICIENT_STORE_P_H

#include <QIODevice>
#include <cstdio>
#include <jpeglib.h>
#include <memory>
#include <vector>

/** Bounded coefficient buffers for one JPEG compression. The store and its
 * borrowed, seekable backing device outlive jpeg_destroy_compress(). Only
 * coefficient arrays use the backing device; the encoder owns format output.
 * Insufficient buffer budget throws std::bad_alloc. Native allocation errors
 * use the encoder's error handler; backing I/O errors throw std::runtime_error
 * through the existing JPEG exception boundary.
 */
class KisJpegCoefficientStore
{
public:
    KisJpegCoefficientStore(j_compress_ptr encoder, QIODevice &backing, qsizetype memoryBudgetBytes);
    ~KisJpegCoefficientStore();
    KisJpegCoefficientStore(const KisJpegCoefficientStore &) = delete;
    KisJpegCoefficientStore &operator=(const KisJpegCoefficientStore &) = delete;
    qsizetype peakBufferBytes() const;
private:
    struct Array;
    static jvirt_barray_ptr request(j_common_ptr, int, boolean, JDIMENSION, JDIMENSION, JDIMENSION);
    static void realize(j_common_ptr);
    static JBLOCKARRAY access(j_common_ptr, jvirt_barray_ptr, JDIMENSION, JDIMENSION, boolean);
    QIODevice &m_backing;
    qsizetype m_budget;
    std::vector<std::unique_ptr<Array>> m_arrays;
    void (*m_originalRealize)(j_common_ptr) = nullptr;
    bool m_realized = false;
};
#endif
