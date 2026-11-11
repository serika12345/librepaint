/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWintabMapping.h"

#include <QtGlobal>

namespace
{

KisWintabMapping::Mode s_mode = KisWintabMapping::Mode::Wintab;
QRect s_customRect;

int signOf(int value)
{
    return value >= 0 ? 1 : -1;
}

/**
 * The packet coordinate is measured from the origin of the input range. A
 * negative extent only turns the axis around, so the magnitude is always
 * between 0 and abs(extent).
 */
int clampToExtent(int value, int extent)
{
    const int limit = qAbs(extent);
    return qBound(0, value, limit);
}

qreal scaleAxis(int coordinate, int extent, int targetOrigin, int targetSize)
{
    if (extent == 0) {
        return targetOrigin;
    }

    const int min = 0;

    if (signOf(targetSize) == signOf(extent)) {
        return ((coordinate - min) * qAbs(targetSize) / qAbs(qreal(extent - min)))
            + targetOrigin;
    }

    return ((qAbs(extent) - (coordinate - min)) * qAbs(targetSize)
            / qAbs(qreal(extent - min))) + targetOrigin;
}

qreal distanceToRectSquared(const QPointF &point, const QRect &rect)
{
    const qreal clampedX = qBound(qreal(rect.left()), point.x(),
                                  qreal(rect.left() + rect.width()));
    const qreal clampedY = qBound(qreal(rect.top()), point.y(),
                                  qreal(rect.top() + rect.height()));
    const qreal dx = point.x() - clampedX;
    const qreal dy = point.y() - clampedY;
    return dx * dx + dy * dy;
}

} // namespace

void KisWintabMapping::setConfiguredMode(Mode mode, const QRect &customRect)
{
    s_mode = mode;
    s_customRect = customRect;
}

KisWintabMapping::Mode KisWintabMapping::configuredMode()
{
    return s_mode;
}

QRect KisWintabMapping::configuredCustomRect()
{
    return s_customRect;
}

QRect KisWintabMapping::targetRect(Mode mode, const Range &range,
                                   const QRect &virtualDesktopRect,
                                   const QRect &customRect)
{
    if (mode == Mode::Custom) {
        return customRect.isValid() ? customRect : virtualDesktopRect;
    }

    if (mode == Mode::VirtualScreen) {
        return virtualDesktopRect;
    }

    const QRect systemRect(range.sysOrgX, range.sysOrgY,
                           range.sysExtX, range.sysExtY);
    return systemRect.isValid() ? systemRect : virtualDesktopRect;
}

QPointF KisWintabMapping::nativePosition(const Range &range,
                                         const QRect &targetRect,
                                         int packetX, int packetY)
{
    const int maxX = range.inExtX - range.inOrgX;
    const int maxY = range.inExtY - range.inOrgY;

    const int x = clampToExtent(packetX, maxX);
    const int y = clampToExtent(packetY, maxY);

    return QPointF(scaleAxis(x, maxX, targetRect.x(), targetRect.width()),
                   scaleAxis(y, maxY, targetRect.y(), targetRect.height()));
}

QPointF KisWintabMapping::logicalPosition(const QPointF &nativePosition,
                                          const QVector<Screen> &screens)
{
    if (screens.isEmpty()) {
        return nativePosition;
    }

    const Screen *best = nullptr;
    qreal bestDistance = 0;

    for (const Screen &screen : screens) {
        const qreal distance = distanceToRectSquared(nativePosition,
                                                     screen.nativeRect);
        if (!best || distance < bestDistance) {
            best = &screen;
            bestDistance = distance;
        }
        if (distance == 0) {
            break;
        }
    }

    const qreal dpr = best->devicePixelRatio > 0
        ? best->devicePixelRatio : 1.0;
    const QPointF offset = nativePosition
        - QPointF(best->nativeRect.topLeft());

    return QPointF(best->logicalRect.x() + offset.x() / dpr,
                   best->logicalRect.y() + offset.y() / dpr);
}
