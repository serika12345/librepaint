/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_DOCUMENT_P_H
#define KIS_GPU_PAINT_DOCUMENT_P_H
#include "KisGpuPaintDocument.h"
#include <gpu/KisGpuExecution.h>
#include <QMap>

namespace Krita::Document::Gpu {
struct PaintDocument::Private
{
    Paint::Execution &execution;
    std::unique_ptr<Paint::OperationDocument> document;
    Paint::Limits limits;
    Paint::DocumentHandle handle = 0;
    QString path;
    QByteArray description;
    QMap<QString, QByteArray> files;
    Paint::ExecutionOutcome lastRequest{Paint::ExecutionCode::Success, {}};
};
}
#endif
