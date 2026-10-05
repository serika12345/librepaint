/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisExperimentCpuProfile.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <chrono>
#include <time.h>
#include "kis_assert.h"
#include "kis_painter.h"
#include "kis_painter_p.h"

#include "kis_paint_device.h"
#include "kis_fixed_paint_device.h"
#include "kis_random_accessor_ng.h"
#include "KisRenderedDab.h"
#include "kis_types.h"
#include <QtGlobal>
#include <qlist.h>

namespace KisExperimentCpuProfile {
namespace {
std::atomic<bool> active{false};
struct Counter { std::atomic<qint64> cpu{0}, exclusiveCpu{0}, wall{0}, calls{0}, units{0}, peakUnits{0}; };
std::array<Counter, Count> counters;
thread_local Scope *current = nullptr;
std::mutex threadMutex;
ThreadCpu threads;
qint64 cpuNow() { timespec t{}; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return t.tv_sec*1000000000LL+t.tv_nsec; }
qint64 wallNow() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
}
void start() {
    { std::lock_guard<std::mutex> lock(threadMutex); threads.clear(); }
    for (auto &c : counters) { c.cpu=0; c.exclusiveCpu=0; c.wall=0; c.calls=0; c.units=0; c.peakUnits=0; }
    active.store(true, std::memory_order_release);
}
Snapshot stop() {
    active.store(false, std::memory_order_release);
    Snapshot result;
    for (int i=0; i<Count; ++i) result[i]={counters[i].cpu.load(),counters[i].exclusiveCpu.load(),counters[i].wall.load(),counters[i].calls.load(),counters[i].units.load(),counters[i].peakUnits.load()};
    return result;
}
ThreadCpu threadCpu() { std::lock_guard<std::mutex> lock(threadMutex); return threads; }
Scope::Scope(Stage stage, qint64 units) : m_stage(stage), m_enabled(active.load(std::memory_order_acquire)), m_units(units) {
    if (m_enabled) { m_parent=current; current=this; m_wall=wallNow(); m_cpu=cpuNow(); }
}
Scope::~Scope() {
    if (!m_enabled) return;
    const qint64 cpu=cpuNow()-m_cpu, wall=wallNow()-m_wall;
    current=m_parent;
    if(m_parent) m_parent->m_children+=cpu;
    else { uint64_t id=0; pthread_threadid_np(nullptr,&id); std::lock_guard<std::mutex> lock(threadMutex); threads[id]+=cpu; }
    auto &c=counters[m_stage];
    c.cpu.fetch_add(cpu,std::memory_order_relaxed);
    c.exclusiveCpu.fetch_add(cpu-m_children,std::memory_order_relaxed);
    c.wall.fetch_add(wall,std::memory_order_relaxed);
    c.calls.fetch_add(1,std::memory_order_relaxed);
    c.units.fetch_add(m_units,std::memory_order_relaxed);
    qint64 peak=c.peakUnits.load(std::memory_order_relaxed);
    while(peak<m_units && !c.peakUnits.compare_exchange_weak(peak,m_units,std::memory_order_relaxed)) {}
}
}

void KisPainter::Private::applyDevice(const QRect &applyRect,
                                      const KisRenderedDab &dab,
                                      KisRandomAccessorSP dstIt,
                                      const KoColorSpace *srcColorSpace,
                                      KoCompositeOp::ParameterInfo &localParamInfo)
{
    const QRect dabRect = dab.realBounds();
    const QRect rc = applyRect & dabRect;

    const int srcPixelSize = srcColorSpace->pixelSize();
    const int dabRowStride = srcPixelSize * dabRect.width();


    qint32 dstY = rc.y();
    qint32 rowsRemaining = rc.height();

    while (rowsRemaining > 0) {
        qint32 dstX = rc.x();

        qint32 numContiguousDstRows = dstIt->numContiguousRows(dstY);
        qint32 rows = qMin(rowsRemaining, numContiguousDstRows);

        qint32 columnsRemaining = rc.width();

        while (columnsRemaining > 0) {

            qint32 numContiguousDstColumns = dstIt->numContiguousColumns(dstX);
            qint32 columns = qMin(numContiguousDstColumns, columnsRemaining);

            qint32 dstRowStride = dstIt->rowStride(dstX, dstY);
            dstIt->moveTo(dstX, dstY);

            localParamInfo.dstRowStart   = dstIt->rawData();
            localParamInfo.dstRowStride  = dstRowStride;
            localParamInfo.maskRowStart  = 0;
            localParamInfo.maskRowStride = 0;
            localParamInfo.rows          = rows;
            localParamInfo.cols          = columns;


            const int dabX = dstX - dabRect.x();
            const int dabY = dstY - dabRect.y();

            localParamInfo.srcRowStart   = dab.device->constData() + dabX * srcPixelSize + dabY * dabRowStride;
            localParamInfo.srcRowStride  = dabRowStride;
            localParamInfo.setOpacityAndAverage(dab.opacity, dab.averageOpacity);
            localParamInfo.flow = dab.flow;
            { KisExperimentCpuProfile::Scope profile(KisExperimentCpuProfile::Composite, qint64(rows)*columns);
              colorSpace->bitBlt(srcColorSpace, localParamInfo, compositeOp(srcColorSpace), renderingIntent, conversionFlags); }

            dstX += columns;
            columnsRemaining -= columns;
        }

        dstY += rows;
        rowsRemaining -= rows;
    }

}

void KisPainter::Private::applyDeviceWithSelection(const QRect &applyRect,
                                                   const KisRenderedDab &dab,
                                                   KisRandomAccessorSP dstIt,
                                                   KisRandomConstAccessorSP maskIt,
                                                   const KoColorSpace *srcColorSpace,
                                                   KoCompositeOp::ParameterInfo &localParamInfo)
{
    const QRect dabRect = dab.realBounds();
    const QRect rc = applyRect & dabRect;

    const int srcPixelSize = srcColorSpace->pixelSize();
    const int dabRowStride = srcPixelSize * dabRect.width();


    qint32 dstY = rc.y();
    qint32 rowsRemaining = rc.height();

    while (rowsRemaining > 0) {
        qint32 dstX = rc.x();

        qint32 numContiguousDstRows = dstIt->numContiguousRows(dstY);
        qint32 numContiguousMaskRows = maskIt->numContiguousRows(dstY);
        qint32 rows = qMin(rowsRemaining, qMin(numContiguousDstRows, numContiguousMaskRows));

        qint32 columnsRemaining = rc.width();

        while (columnsRemaining > 0) {

            qint32 numContiguousDstColumns = dstIt->numContiguousColumns(dstX);
            qint32 numContiguousMaskColumns = maskIt->numContiguousColumns(dstX);
            qint32 columns = qMin(columnsRemaining, qMin(numContiguousDstColumns, numContiguousMaskColumns));

            qint32 dstRowStride = dstIt->rowStride(dstX, dstY);
            qint32 maskRowStride = maskIt->rowStride(dstX, dstY);
            dstIt->moveTo(dstX, dstY);
            maskIt->moveTo(dstX, dstY);

            localParamInfo.dstRowStart   = dstIt->rawData();
            localParamInfo.dstRowStride  = dstRowStride;
            localParamInfo.maskRowStart  = maskIt->rawDataConst();
            localParamInfo.maskRowStride = maskRowStride;
            localParamInfo.rows          = rows;
            localParamInfo.cols          = columns;


            const int dabX = dstX - dabRect.x();
            const int dabY = dstY - dabRect.y();

            localParamInfo.srcRowStart   = dab.device->constData() + dabX * srcPixelSize + dabY * dabRowStride;
            localParamInfo.srcRowStride  = dabRowStride;
            localParamInfo.setOpacityAndAverage(dab.opacity, dab.averageOpacity);
            localParamInfo.flow = dab.flow;
            { KisExperimentCpuProfile::Scope profile(KisExperimentCpuProfile::Composite, qint64(rows)*columns);
              colorSpace->bitBlt(srcColorSpace, localParamInfo, compositeOp(srcColorSpace), renderingIntent, conversionFlags); }

            dstX += columns;
            columnsRemaining -= columns;
        }

        dstY += rows;
        rowsRemaining -= rows;
    }

}

void KisPainter::bltFixed(const QRect &applyRect, const QList<KisRenderedDab> allSrcDevices)
{
    KisExperimentCpuProfile::Scope profile(KisExperimentCpuProfile::BrushApply);
    const KoColorSpace *srcColorSpace = 0;
    QList<KisRenderedDab> devices;
    QRect rc = applyRect;

    if (d->selection) {
        rc &= d->selection->selectedRect();
    }

    QRect totalDevicesRect;

    Q_FOREACH (const KisRenderedDab &dab, allSrcDevices) {
        if (rc.intersects(dab.realBounds())) {
            devices.append(dab);
            totalDevicesRect |= dab.realBounds();
        }

        if (!srcColorSpace) {
            srcColorSpace = dab.device->colorSpace();
        } else {
            KIS_SAFE_ASSERT_RECOVER_RETURN(*srcColorSpace == *dab.device->colorSpace());
        }
    }

    rc &= totalDevicesRect;

    if (devices.isEmpty() || rc.isEmpty()) return;

    KoCompositeOp::ParameterInfo localParamInfo = d->paramInfo;
    static const bool packed=qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_BRUSH_PACKED");
    if(packed && !d->selection) {
        const int dstPixelSize=d->device->pixelSize();
        const int stride=rc.width()*dstPixelSize;
        std::unique_ptr<quint8[]> pixels(new quint8[qint64(stride)*rc.height()]);
        d->device->readBytes(pixels.get(),rc);
        for(const auto &dab : std::as_const(devices)) {
            const QRect dabRect=dab.realBounds(), part=rc&dabRect;
            if(part.isEmpty()) continue;
            localParamInfo.dstRowStart=pixels.get()+(part.y()-rc.y())*stride+(part.x()-rc.x())*dstPixelSize;
            localParamInfo.dstRowStride=stride;
            localParamInfo.srcRowStart=dab.device->constData()+((part.y()-dabRect.y())*dabRect.width()+part.x()-dabRect.x())*srcColorSpace->pixelSize();
            localParamInfo.srcRowStride=dabRect.width()*srcColorSpace->pixelSize();
            localParamInfo.maskRowStart=nullptr;
            localParamInfo.maskRowStride=0;
            localParamInfo.rows=part.height(); localParamInfo.cols=part.width();
            localParamInfo.setOpacityAndAverage(dab.opacity,dab.averageOpacity);
            localParamInfo.flow=dab.flow;
            { KisExperimentCpuProfile::Scope composite(KisExperimentCpuProfile::Composite,qint64(part.width())*part.height());
              d->colorSpace->bitBlt(srcColorSpace,localParamInfo,d->compositeOp(srcColorSpace),d->renderingIntent,d->conversionFlags); }
        }
        d->device->writeBytes(pixels.get(),rc);
        return;
    }
    KisRandomAccessorSP dstIt = d->device->createRandomAccessorNG();
    KisRandomConstAccessorSP maskIt = d->selection ? d->selection->projection()->createRandomConstAccessorNG() : 0;

    if (maskIt) {
        Q_FOREACH (const KisRenderedDab &dab, devices) {
            d->applyDeviceWithSelection(rc, dab, dstIt, maskIt, srcColorSpace, localParamInfo);
        }
    } else {
        static const bool tileOrder=qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_BRUSH_TILE_ORDER");
        if (tileOrder) {
            // Preserve dab order for each pixel while keeping a destination tile hot.
            for (int y=rc.y();y<=rc.bottom();) {
                const int rows=qMin(rc.bottom()-y+1,dstIt->numContiguousRows(y));
                for(int x=rc.x();x<=rc.right();) {
                    const int cols=qMin(rc.right()-x+1,dstIt->numContiguousColumns(x));
                    const QRect tileRect(x,y,cols,rows);
                    for(const auto &dab : std::as_const(devices)) {
                        if(tileRect.intersects(dab.realBounds())) d->applyDevice(tileRect,dab,dstIt,srcColorSpace,localParamInfo);
                    }
                    x+=cols;
                }
                y+=rows;
            }
        } else {
            Q_FOREACH (const KisRenderedDab &dab, devices) {
                d->applyDevice(rc, dab, dstIt, srcColorSpace, localParamInfo);
            }
        }
    }


#if 0
    // the code above does basically the same thing as this one,
    // but more efficiently :)

    Q_FOREACH (KisFixedPaintDeviceSP dev, devices) {
        const QRect copyRect = dev->bounds() & rc;
        if (copyRect.isEmpty()) continue;

        bltFixed(copyRect.topLeft(), dev, copyRect);
    }
#endif
}

