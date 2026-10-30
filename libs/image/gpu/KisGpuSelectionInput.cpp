/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuSelectionInput.h"
#include "kis_image.h"
#include "kis_pixel_selection.h"
#include "kis_painter.h"
#include "kis_selection_filters.h"
#include "kis_default_bounds.h"
#include <KoColor.h>
#include <QPainterPath>
#include <cmath>
#include <new>

namespace LibrePaint::Image::Gpu {
SelectionMaskSP selectionMaskFromPath(const QPainterPath &path, const KisImageSP &image,
                                     bool antiAlias, int grow, int feather)
{
    if (!image || path.isEmpty() || qAbs(qint64(grow)) > 400 || feather < 0 || feather > 400) return {};
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto element = path.elementAt(i);
        if (!std::isfinite(element.x) || !std::isfinite(element.y) ||
            qAbs(element.x) > RgbaTile::CoordinateLimit - 1024 ||
            qAbs(element.y) > RgbaTile::CoordinateLimit - 1024) return {};
    }
    // Native filters include complete selection blocks around the geometric
    // input. Admit that working extent before allocating or changing history.
    const int halo = qAbs(grow) + feather + 128;
    const QRect work = path.boundingRect().toAlignedRect().adjusted(-halo, -halo, halo, halo);
    if (qint64(work.width()) * work.height() > SelectionMask::MaximumBytes) return {};
    try {
        KisPixelSelectionSP input = new KisPixelSelection(new KisDefaultBounds(image));
        KisPainter painter(input);
        painter.setPaintColor(KoColor(Qt::black, input->colorSpace()));
        painter.setAntiAliasPolygonFill(antiAlias && feather == 0);
        painter.setFillStyle(KisPainter::FillStyleForegroundColor);
        painter.setStrokeStyle(KisPainter::StrokeStyleNone);
        painter.fillPainterPath(path);
        painter.end();
        if (grow > 0) {
            KisGrowSelectionFilter filter(grow, grow);
            filter.process(input, input->selectedRect().adjusted(-grow, -grow, grow, grow));
        } else if (grow < 0) {
            KisShrinkSelectionFilter filter(-grow, -grow, false);
            filter.process(input, input->selectedRect());
        }
        if (feather > 0) {
            KisFeatherSelectionFilter filter(feather);
            filter.process(input, input->selectedRect().adjusted(-feather, -feather, feather, feather));
        }
        const QRect bounds = input->selectedExactRect();
        if (bounds.isEmpty()) return SelectionMask::empty();
        const qint64 bytes = qint64(bounds.width()) * bounds.height();
        if (bytes > SelectionMask::MaximumBytes) return {};
        QByteArray coverage(qsizetype(bytes), '\0');
        input->readBytes(reinterpret_cast<quint8 *>(coverage.data()), bounds);
        return SelectionMask::fromAlphaCoverage(bounds, coverage, bounds);
    } catch (const std::bad_alloc &) {
        return {};
    }
}
}
