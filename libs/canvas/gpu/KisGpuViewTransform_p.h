/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_VIEW_TRANSFORM_P_H
#define KIS_GPU_VIEW_TRANSFORM_P_H

#include <QTransform>
#include <cmath>

namespace Krita::Canvas::Gpu::Detail {
inline bool validDocumentTransform(const QTransform &transform)
{
    if (!transform.isAffine() || !transform.isInvertible()) return false;
    for (const double value : {transform.m11(), transform.m12(), transform.m21(), transform.m22(), transform.dx(), transform.dy()}) {
        if (!std::isfinite(value) || std::abs(value) > 8388608) return false;
    }
    return true;
}
}
#endif
