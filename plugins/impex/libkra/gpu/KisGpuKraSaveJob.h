/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_KRA_SAVE_JOB_H
#define KIS_GPU_KRA_SAVE_JOB_H
#include "KisGpuDocumentReadback.h"

/** Frozen GPU layers saved through the existing native KRA writer.
 * Construction, polling and destruction use the GUI/submitting thread.
 * Destruction drains an accepted native write before releasing its document.
 */
class KisGpuKraSaveJob
{
public:
    struct Metadata {
        QSize canvasSize;
        QString name;
        double xResolution = 1, yResolution = 1;
    };
    enum class State { Reading, Preparing, Writing, Succeeded, Failed };
    enum class Error { None, InvalidInput, ReadbackFailed, SaveFailed };
    /** Snapshot budget covers readback CPU values; native image/export allocations have existing owners.
     * Invalid dimensions, resolution, path or XML text fail before GPU readback.
     */
    KisGpuKraSaveJob(std::shared_ptr<KisGpuTileStore> store,
                    QVector<KisGpuLayerDocument::Layer> layers,
                    KisGpuLayerDocument::LayerId nextId, Metadata metadata,
                    QString path, quint64 maximumSnapshotBytes = 64 * 1024 * 1024);
    ~KisGpuKraSaveJob();
    KisGpuKraSaveJob(const KisGpuKraSaveJob &) = delete;
    KisGpuKraSaveJob &operator=(const KisGpuKraSaveJob &) = delete;
    void poll();
    State state() const;
    Error error() const;
    KisGpuDocumentReadback::Error readbackError() const;
    QString errorMessage() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
#endif
