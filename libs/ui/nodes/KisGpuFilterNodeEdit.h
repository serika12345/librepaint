/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_FILTER_NODE_EDIT_H
#define KIS_GPU_FILTER_NODE_EDIT_H

#include <kis_types.h>
#include <QString>
class KisViewManager;
namespace LibrePaint::Image::Gpu { enum class LayerKind : quint8; }

KisNodeSP createGpuFilterNode(KisViewManager *view, KisNodeSP parent, qsizetype index,
    LibrePaint::Image::Gpu::LayerKind kind, const QString &name,
    const KisFilterConfigurationSP &initial = {}, bool quiet = false);
void editGpuFilterNode(KisViewManager *view, KisNodeSP node);

#endif
