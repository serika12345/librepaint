/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_KRA_LOAD_JOB_H
#define KIS_GPU_KRA_LOAD_JOB_H
#include "KisGpuLayerDocument.h"
#include "KisGpuKraMetadata.h"

/** Existing native KRA parsing followed by checkpoint-based GPU restoration.
 * Calls and destruction use the GUI/submitting thread. Native parsing uses
 * its existing synchronous contract; GPU restoration progresses through poll().
 * A complete document is transferred once through takeDocument().
 */
class KisGpuKraLoadJob
{
public:
    enum class State { Reading, Restoring, Succeeded, Failed };
    enum class Error { None, InvalidInput, LoadFailed, UnsupportedDocument, SnapshotBudgetExceeded, GpuRejected };
    /** CPU budget includes the frozen document-info XML and raster recovery values. */
    KisGpuKraLoadJob(std::shared_ptr<KisGpuTileStore> store, QString path, qsizetype retainedEdits,
                    quint64 maximumRecoveryBytes = 64 * 1024 * 1024);
    ~KisGpuKraLoadJob();
    KisGpuKraLoadJob(const KisGpuKraLoadJob &) = delete;
    KisGpuKraLoadJob &operator=(const KisGpuKraLoadJob &) = delete;
    void poll();
    State state() const;
    Error error() const;
    KisGpuTileStore::Error gpuError() const;
    std::optional<KisGpuKraMetadata> metadata() const;
    std::unique_ptr<KisGpuLayerDocument> takeDocument();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
#endif
