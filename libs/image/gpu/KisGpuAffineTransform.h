/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_AFFINE_TRANSFORM_H
#define KIS_GPU_AFFINE_TRANSFORM_H

#include <kritaimagegpuoperations_export.h>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QSharedPointer>
#include <QString>
#include <array>
#include <optional>

namespace LibrePaint::Image::Gpu {
/** Frozen native filter coefficients. The image's existing filter definition
 * supplies all 256 subpixel phases once per resampling scale; these values are
 * editing inputs, independent of display interpolation and later tool changes.
 */
struct AffineKernel
{
    struct Phase { qint32 offset = 0, count = 0, center = 0; };
    std::array<Phase, 256> phases;
    QList<qint32> weights;
    qint32 positionScale = 256;
    double support = 0;
};
using AffineKernelSP = QSharedPointer<const AffineKernel>;

struct AffinePass
{
    enum Kind : quint32 { Horizontal, Vertical, Remap };
    Kind kind = Horizontal;
    QRect source, destination;
    double scale = 1, shear = 0, offset = 0;
    AffineKernelSP kernel;
    /** Destination pixel indices from source indices: x'=a*x+b*y+tx,
     * y'=c*x+d*y+ty. Orthogonal integer maps preserve every native RGBA bit.
     */
    std::array<qint32, 6> map{1, 0, 0, 1, 0, 0};
    bool replaceLayer = false;
    bool mapWholeLayer = false;
};
struct AffinePlan
{
    QList<AffinePass> passes;
    QPoint positionDelta;
    QString filterId;
};
using AffinePlanSP = QSharedPointer<const AffinePlan>;

/** Native line limits, including the nonzero shear's filter support. */
KRITAIMAGEGPUOPERATIONS_EXPORT std::optional<AffinePass> affineResamplePass(
    AffinePass::Kind kind, QRect source, double scale, double shear, double offset, const AffineKernelSP &kernel);
KRITAIMAGEGPUOPERATIONS_EXPORT bool validAffinePlan(const AffinePlan &plan);
/** The complete predecessor region needed to evaluate the requested output.
 * Retains unmodified pixels around in-place resampling passes.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT QRect affineNeededRegion(const AffinePass &pass, QRect output);
KRITAIMAGEGPUOPERATIONS_EXPORT QRect affineOutputRegion(const AffinePass &pass, QRect input);
}
#endif
