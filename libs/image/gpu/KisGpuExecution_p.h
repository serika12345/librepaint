/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_EXECUTION_P_H
#define KIS_GPU_EXECUTION_P_H

#include "KisGpuExecution.h"
#include <QHash>
#include <QThread>
#include "KisGpuPresentationSurface.h"

namespace LibrePaint::Image::Gpu {
struct Execution::Private
{
    struct Document
    {
        Snapshot requested, published;
        quint64 serial = 1;
        RenderedSnapshot rendered;
        ProjectedSnapshot projected;
    };
    struct Stage
    {
        DocumentHandle handle = 0;
        quint64 serial = 0, generation = 0;
        RenderedSnapshot rendered;
        ProjectedSnapshot projected;
    };
    DevicePtr device;
    qsizetype byteBudget, maximumDocuments, maximumReadbackBytes;
    std::unique_ptr<TileRenderer> renderer;
    QHash<DocumentHandle, Document> documents;
    quint64 nextHandle = 0, generation = 1;
    PresentationSurface *surface = nullptr;
    WGPUCommandEncoder commands = nullptr;
    bool active = false, attempted = false;
    Stage stage;
    RenderedSnapshot readbackRendered;
    ProjectedSnapshot readbackProjected;

    Private(DevicePtr device, qsizetype bytes, qsizetype documents, qsizetype readbackBytes)
        : device(std::move(device)), byteBudget(bytes), maximumDocuments(documents),
          maximumReadbackBytes(readbackBytes), renderer(std::make_unique<TileRenderer>(this->device, bytes)) {}
    bool onThread() const { return device->thread() == QThread::currentThread(); }
    bool isCurrent(const Stage &value) const
    {
        const auto document = documents.constFind(value.handle);
        return value.handle && document != documents.cend() && document->serial == value.serial &&
            value.generation == generation;
    }
};
}
#endif
