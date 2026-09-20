#include "kis_convex_hull.h"

#include "kis_convex_hull_p.h"

#include "kis_paint_device.h"
#include "kis_random_accessor_ng.h"
#include "KoColorSpace.h"
#include "KoColor.h"
#include "KoColorModelStandardIds.h"

namespace {

// From libs/image/kis_paint_device.cc
struct CheckFullyTransparent {
    CheckFullyTransparent(const KoColorSpace *colorSpace)
        : m_colorSpace(colorSpace)
    {
    }

    bool isPixelEmpty(const quint8 *pixelData)
    {
        return m_colorSpace->opacityU8(pixelData) == OPACITY_TRANSPARENT_U8;
    }

private:
    const KoColorSpace *m_colorSpace;
};

struct CheckNonDefault {
    CheckNonDefault(int pixelSize, const quint8 *defaultPixel)
        : m_pixelSize(pixelSize),
          m_defaultPixel(defaultPixel)
    {
    }

    bool isPixelEmpty(const quint8 *pixelData)
    {
        return memcmp(m_defaultPixel, pixelData, m_pixelSize) == 0;
    }

private:
    int m_pixelSize;
    const quint8 *m_defaultPixel;
};

struct CheckDeselected {
    CheckDeselected(const KoColorSpace *colorSpace)
        : m_colorSpace(colorSpace),
          m_deselectedColor(Qt::black, colorSpace),
          m_pixelSize(colorSpace->pixelSize())
    {
        KIS_SAFE_ASSERT_RECOVER_NOOP(colorSpace->colorModelId() == AlphaColorModelID ||
                                     colorSpace->colorModelId() == GrayAColorModelID);
    }

    bool isPixelEmpty(const quint8 *pixelData)
    {
        return m_colorSpace->opacityU8(pixelData) == OPACITY_TRANSPARENT_U8 ||
            memcmp(m_deselectedColor.data(), pixelData, m_pixelSize) == 0;
    }

private:
    const KoColorSpace *m_colorSpace;
    const KoColor m_deselectedColor;
    const int m_pixelSize;
};

template <class ComparePixelOp>
QVector<QPoint> retrieveAllBoundaryPointsImpl(const KisPaintDevice *device, const QRect &rect, const QRect &skip, ComparePixelOp compareOp)
{
    QVector<QPoint> points;
    int defaultMin = rect.x() + rect.width() + 1;
    int defaultMax = rect.x() - 1;
    QVector<int> minX(rect.height(), defaultMin);
    QVector<int> maxX(rect.height(), defaultMax);
    int base = rect.top();
    if (!skip.isEmpty()) {
        for (int y = skip.top(); y <= skip.bottom(); y++) {
            minX[y - base] = skip.left();
            maxX[y - base] = skip.right();
        }
    }

    int pixelSize = device->pixelSize();
    KisRandomConstAccessorSP accessor = device->createRandomConstAccessorNG();
    for (int y = rect.top(); y <= rect.bottom();) {
        int rows = accessor->numContiguousRows(y);
        for (int x = rect.left(); x <= rect.right();) {
            int columns = accessor->numContiguousColumns(x);
            accessor->moveTo(x, y);
            int strideBytes = accessor->rowStride(x, y);
            const quint8 *data = accessor->rawDataConst();
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < columns; c++) {
                    if (!compareOp.isPixelEmpty(data + c * pixelSize)) {
                        int index = y + r - base;
                        minX[index] = std::min(minX[index], x + c);
                        maxX[index] = std::max(maxX[index], x + c);
                    }
                }
                data += strideBytes;
            }
            x += columns;
        }
        y += rows;
    }

    for (int y = rect.top(); y <= rect.bottom(); y++) {
        int index = y - base;
        if (minX[index] < defaultMin) {
            points << QPoint(minX[index], y);
            points << QPoint(minX[index], y + 1);
        }
        if (maxX[index] > defaultMax) {
            points << QPoint(maxX[index] + 1, y);
            points << QPoint(maxX[index] + 1, y + 1);
        }
    }
    
    return points;
}
// This matches the behavior of KisPaintDevice::calculateExactBounds(false), whose result is returned by KisPaintDevice::exactBounds()
QVector<QPoint> retrieveAllBoundaryPoints(const KisPaintDevice *device) {
    QRect rect = device->extent();

    const KoColor defaultPixel = device->defaultPixel();
    const quint8 defaultOpacity = defaultPixel.opacityU8();

    QVector<QPoint> points;

    if (defaultOpacity != OPACITY_TRANSPARENT_U8) {
        QRect skip = device->defaultBounds()->bounds();
        CheckNonDefault compareOp(device->pixelSize(), defaultPixel.data());

        points = retrieveAllBoundaryPointsImpl(device, rect, skip, compareOp);
        if (!skip.isEmpty()) {
            int x, y, w, h;
            skip.getRect(&x, &y, &w, &h);
            points << QPoint(x, y) << QPoint(x + w, y) << QPoint(x + w, y + h) << QPoint(x, y + h);
        }
    } else {
        CheckFullyTransparent compareOp(device->colorSpace());
        points = retrieveAllBoundaryPointsImpl(device, rect, QRect(), compareOp);
    }
    return points;
}

QVector<QPoint> retrieveAllBoundaryPointsSelectionLike(const KisPaintDevice *device) {
    QRect rect = device->extent();

    const KoColor defaultPixel = device->defaultPixel();
    const quint8 defaultOpacity = defaultPixel.opacityU8();

    QVector<QPoint> points;

    if (defaultOpacity != OPACITY_TRANSPARENT_U8 &&
        defaultPixel != KoColor(Qt::black, defaultPixel.colorSpace())) {

        QRect skip = device->defaultBounds()->bounds();
        CheckNonDefault compareOp(device->pixelSize(), defaultPixel.data());

        points = retrieveAllBoundaryPointsImpl(device, rect, skip, compareOp);
        if (!skip.isEmpty()) {
            int x, y, w, h;
            skip.getRect(&x, &y, &w, &h);
            points << QPoint(x, y) << QPoint(x + w, y) << QPoint(x + w, y + h) << QPoint(x, y + h);
        }
    } else if (device->colorSpace()->colorModelId() == AlphaColorModelID) {
        CheckFullyTransparent compareOp(device->colorSpace());
        points = retrieveAllBoundaryPointsImpl(device, rect, QRect(), compareOp);
    } else {
        // pre-condition:
        // defaultOpacity == OPACITY_TRANSPARENT_U8 ||
        // defaultPixel == "deselected"

        CheckDeselected compareOp(device->colorSpace());
        points = retrieveAllBoundaryPointsImpl(device, rect, QRect(), compareOp);
    }
    return points;
}

}

namespace KisConvexHullPrivate
{

QVector<QPoint> retrieveBoundaryPoints(const KisPaintDeviceSP &device, BoundaryMode mode)
{
    return mode == BoundaryMode::SelectionLike
        ? retrieveAllBoundaryPointsSelectionLike(device.data())
        : retrieveAllBoundaryPoints(device.data());
}

} // namespace KisConvexHullPrivate
