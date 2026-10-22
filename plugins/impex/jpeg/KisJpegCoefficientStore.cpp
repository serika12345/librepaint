/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisJpegCoefficientStore_p.h"
#include <QBitArray>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {
qint64 product(qint64 first, qint64 second)
{
    if (first < 0 || second < 0 || (second && first > std::numeric_limits<qint64>::max() / second))
        throw std::bad_alloc();
    return first * second;
}

void seek(QIODevice &device, qint64 position)
{
    if (!device.seek(position)) throw std::runtime_error("JPEG coefficient backing seek failed");
}

void transfer(QIODevice &device, char *bytes, qint64 size, bool writing)
{
    qint64 completed = 0;
    while (completed < size) {
        const auto count = writing ? device.write(bytes + completed, size - completed) :
                                     device.read(bytes + completed, size - completed);
        if (count <= 0) throw std::runtime_error(writing ?
            "JPEG coefficient backing write failed" : "JPEG coefficient backing read failed");
        completed += count;
    }
}
}

struct KisJpegCoefficientStore::Array
{
    JDIMENSION width, height, maxAccess;
    qint64 rowBytes, rowCost, offset = 0;
    JDIMENSION first = 0, count = 0;
    bool resident = false, dirty = false;
    JBLOCKARRAY rows = nullptr;
    qsizetype bufferBytes = 0;
    QBitArray written;

    Array(JDIMENSION width, JDIMENSION height, JDIMENSION maxAccess)
        : width(width), height(height), maxAccess(maxAccess),
          rowBytes(product(width, sizeof(JBLOCK))), rowCost(rowBytes + sizeof(JBLOCKROW))
    {
        if (!width || !height || !maxAccess || maxAccess > height)
            throw std::runtime_error("Invalid JPEG coefficient array");
    }

    void allocate(j_common_ptr context, JDIMENSION bufferRows)
    {
        rows = context->mem->alloc_barray(context, JPOOL_IMAGE, width, bufferRows);
        bufferBytes = product(rowCost, bufferRows);
        for (JDIMENSION row = 0; row < bufferRows; ++row)
            std::memset(rows[row], 0, size_t(rowBytes));
        if (resident) count = height;
        else written.resize(height);
    }

    JBLOCKARRAY access(QIODevice &backing, JDIMENSION start, JDIMENSION requested, bool writable)
    {
        if (!requested || requested > maxAccess || qint64(start) + requested > height)
            throw std::runtime_error("Invalid JPEG coefficient access");
        if (!resident && !(count && start >= first && qint64(start) + requested <= qint64(first) + count)) {
            if (dirty) {
                for (JDIMENSION row = 0; row < count; ++row) {
                    seek(backing, offset + product(first + row, rowBytes));
                    transfer(backing, reinterpret_cast<char *>(rows[row]), rowBytes, true);
                    written.setBit(first + row);
                }
            }
            first = start;
            count = std::min(maxAccess, height - first);
            for (JDIMENSION row = 0; row < count; ++row) {
                if (written.testBit(first + row)) {
                    seek(backing, offset + product(first + row, rowBytes));
                    transfer(backing, reinterpret_cast<char *>(rows[row]), rowBytes, false);
                } else {
                    std::memset(rows[row], 0, size_t(rowBytes));
                }
            }
            dirty = false;
        }
        dirty |= writable;
        return rows + (start - first);
    }
};

KisJpegCoefficientStore::KisJpegCoefficientStore(j_compress_ptr encoder, QIODevice &backing, qsizetype memoryBudgetBytes)
    : m_backing(backing), m_budget(memoryBudgetBytes)
{
    if (!encoder || !encoder->mem || encoder->client_data || memoryBudgetBytes <= 0)
        throw std::runtime_error("Invalid JPEG coefficient store owner");
    m_originalRealize = encoder->mem->realize_virt_arrays;
    encoder->client_data = this;
    encoder->mem->request_virt_barray = request;
    encoder->mem->realize_virt_arrays = realize;
    encoder->mem->access_virt_barray = access;
}
KisJpegCoefficientStore::~KisJpegCoefficientStore() = default;

qsizetype KisJpegCoefficientStore::peakBufferBytes() const
{
    qsizetype total = 0;
    for (const auto &array : m_arrays) total += array->bufferBytes;
    return total;
}

jvirt_barray_ptr KisJpegCoefficientStore::request(j_common_ptr context, int pool, boolean,
    JDIMENSION width, JDIMENSION height, JDIMENSION maxAccess)
{
    auto &store = *static_cast<KisJpegCoefficientStore *>(context->client_data);
    if (pool != JPOOL_IMAGE || store.m_realized)
        throw std::runtime_error("Invalid JPEG coefficient allocation lifetime");
    auto array = std::make_unique<Array>(width, height, maxAccess);
    const auto handle = reinterpret_cast<jvirt_barray_ptr>(array.get());
    store.m_arrays.push_back(std::move(array));
    return handle;
}

void KisJpegCoefficientStore::realize(j_common_ptr context)
{
    auto &store = *static_cast<KisJpegCoefficientStore *>(context->client_data);
    store.m_originalRealize(context);
    if (store.m_realized) return;
    qint64 minimum = 0;
    for (const auto &array : store.m_arrays) {
        const auto bytes = product(array->rowCost, array->maxAccess);
        if (bytes > store.m_budget - minimum) throw std::bad_alloc();
        minimum += bytes;
    }
    qint64 remaining = store.m_budget - minimum;
    qint64 fileOffset = 0;
    bool needsBacking = false;
    for (const auto &array : store.m_arrays) {
        const auto extra = product(array->rowCost, array->height - array->maxAccess);
        array->resident = extra <= remaining;
        if (array->resident) remaining -= extra;
        else {
            const auto bytes = product(array->rowBytes, array->height);
            if (bytes > std::numeric_limits<qint64>::max() - fileOffset) throw std::bad_alloc();
            array->offset = fileOffset;
            fileOffset += bytes;
            needsBacking = true;
        }
        array->allocate(context, array->resident ? array->height : array->maxAccess);
    }
    if (needsBacking) {
        if (store.m_backing.isSequential() ||
            (!store.m_backing.isOpen() && !store.m_backing.open(QIODevice::ReadWrite)) ||
            !store.m_backing.isReadable() || !store.m_backing.isWritable())
            throw std::runtime_error("JPEG coefficient backing could not open");
    }
    store.m_realized = true;
}

JBLOCKARRAY KisJpegCoefficientStore::access(j_common_ptr context, jvirt_barray_ptr handle,
    JDIMENSION start, JDIMENSION count, boolean writable)
{
    auto &store = *static_cast<KisJpegCoefficientStore *>(context->client_data);
    if (!store.m_realized) throw std::runtime_error("JPEG coefficient buffers are not ready");
    const auto found = std::find_if(store.m_arrays.begin(), store.m_arrays.end(),
        [handle](const auto &array) { return reinterpret_cast<jvirt_barray_ptr>(array.get()) == handle; });
    if (found == store.m_arrays.end()) throw std::runtime_error("JPEG coefficient array belongs to another encoder");
    return (*found)->access(store.m_backing, start, count, writable);
}
