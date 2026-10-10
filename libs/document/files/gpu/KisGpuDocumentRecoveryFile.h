/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DOCUMENT_RECOVERY_FILE_H
#define KIS_GPU_DOCUMENT_RECOVERY_FILE_H
#include "KisGpuLayerDocument.h"

/** CPU-only recovery file I/O. Calls may run on a worker after capturing recovery
 * on the document's submitting thread. Successful writes replace the file atomically.
 */
class KisGpuDocumentRecoveryFile
{
public:
    enum class Error { None, Io, InvalidData, UnsupportedVersion, BudgetExceeded };
    struct ReadResult {
        Error error = Error::InvalidData;
        std::optional<KisGpuLayerDocument::Recovery> recovery;
    };
    static Error write(const QString &path, const KisGpuLayerDocument::Recovery &recovery,
                       quint64 maximumBytes = 64 * 1024 * 1024);
    static ReadResult read(const QString &path, quint64 maximumBytes = 64 * 1024 * 1024);
};
#endif
