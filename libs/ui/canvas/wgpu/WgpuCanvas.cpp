/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuCanvas.h"
#include "WgpuCanvasPresenter.h"
#include "../kis_canvas_performance_measurement_p.h"
#include "../kis_canvas2.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <kis_image.h>
#include <KisExperimentCpuProfile.h>
#include <kis_coordinates_converter.h>
#include <kis_projection_update_info.h>
#include <kis_paint_device.h>
#include <KoColorSpaceRegistry.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <KoCanvasResourceProvider.h>
#include <kis_group_layer.h>
#include <QMutex>
#include <QMutexLocker>

namespace Krita::Canvas
{
class RawProjectionPatch : public KisProjectionUpdateInfo {
public:
    explicit RawProjectionPatch(QRect rect) : KisProjectionUpdateInfo(rect) {}
    bool deferred=false;
    qint64 captureNs=0;
    QImage patch;
    QVector<WgpuCanvasPresenter::LayerPatch> layers;
    QVector<quint8> opacities;
};
struct WgpuCanvas::Private
{
    std::unique_ptr<WgpuCanvasPresenter> presenter;
    QTimer composeTimer;
    QRect dirty;
    QImage frame;
    bool gpuProjection = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_GPU_PROJECTION");
    bool rawProjection = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_RAW_PROJECTION");
    bool gpuLayers=qEnvironmentVariableIntValue("LIBREPAINT_WGPU_LAYER_COMPOSE");
    bool lateCapture=qEnvironmentVariableIntValue("LIBREPAINT_WGPU_LATE_LAYER_CAPTURE");
    bool gpuIndirect=qEnvironmentVariableIntValue("LIBREPAINT_WGPU_INDIRECT_COMPOSE");
    KisNodeSP indirectNode;
    QVector<int> layerSlots;
    bool layersQueued=false;
    QMutex layersMutex;
    QVector<KisNodeSP> layers;
    QVector<int> initialSequences;
    quint64 generation = 0;
};

WgpuCanvas::WgpuCanvas(KisCanvas2 *canvas, KisCoordinatesConverter *converter, QWidget *parent)
    : KisQPainterCanvas(canvas, converter, parent), m_wgpu(new Private)
{
    // Keep QWidget's native input/focus route and attach only GPU presentation.
    setAttribute(Qt::WA_NativeWindow);
    if(qEnvironmentVariableIntValue("LIBREPAINT_WGPU_DIRECT_WIDGET")) {
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }
    winId();
    m_wgpu->presenter = std::make_unique<WgpuCanvasPresenter>(*windowHandle());
    if(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_CPU_PROFILE")) {
        m_wgpu->presenter->setFrameScopeFactory([]() -> std::shared_ptr<void> {
            return std::make_shared<KisExperimentCpuProfile::Scope>(KisExperimentCpuProfile::GpuFrame);
        });
    }
    if (m_wgpu->lateCapture) m_wgpu->presenter->setLayerCapture([this](const QRect &rect) {
        auto info=captureProjection(rect,true);
        return static_cast<RawProjectionPatch *>(info.data())->layers;
    });
    m_wgpu->composeTimer.setSingleShot(true);
    m_wgpu->composeTimer.setInterval(qMax(0,qEnvironmentVariableIntValue("LIBREPAINT_WGPU_COMPOSE_INTERVAL_MS")));
    connect(&m_wgpu->composeTimer, &QTimer::timeout, this, &WgpuCanvas::compose);
}

WgpuCanvas::~WgpuCanvas()
{
    // Release the GPU surface before QWidget destroys its native window.
    m_wgpu->presenter.reset();
}

QImage WgpuCanvas::capturedFrame()
{
    QImage image = m_wgpu->presenter->readback();
    image.setDevicePixelRatio(devicePixelRatioF());
    return image;
}
QImage WgpuCanvas::composedFrame() const {
    if (!m_wgpu->gpuProjection) return m_wgpu->frame;
    QImage reference(size()*devicePixelRatioF(),QImage::Format_ARGB32_Premultiplied);
    reference.setDevicePixelRatio(devicePixelRatioF());
    if (m_wgpu->rawProjection) {
        QPainter painter(&reference); painter.fillRect(rect(),borderColor());
        painter.setTransform(coordinatesConverter()->imageToWidgetTransform());
        painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
        const auto image=canvas()->image();
        const QImage pixels=image->projection()->convertToQImage(KoColorSpaceRegistry::instance()->rgb8()->profile(),
            0,0,image->width(),image->height());
        painter.drawImage(QPoint(0,0),pixels); painter.end(); return reference;
    }
    QPainter painter(&reference); paintCanvas(painter,rect()); painter.end(); return reference;
}
KisUpdateInfoSP WgpuCanvas::startUpdateCanvasProjection(const QRect &rect)
{
    return captureProjection(rect,false);
}
KisUpdateInfoSP WgpuCanvas::captureProjection(const QRect &rect,bool forceCapture)
{
    if (!m_wgpu->rawProjection) return KisQPainterCanvas::startUpdateCanvasProjection(rect);
    const qint64 captureStart=Performance::nowNs();
    const QRect clipped = rect.intersected(canvas()->image()->bounds());
    auto *info = new RawProjectionPatch(clipped);
    if (m_wgpu->lateCapture && !forceCapture) { info->deferred=true;return info; }
    if (m_wgpu->gpuLayers && !clipped.isEmpty()) {
        QMutexLocker lock(&m_wgpu->layersMutex);
        const bool initial=m_wgpu->layers.isEmpty();
        if (initial) {
            std::function<void(KisNodeSP)> collect=[&](KisNodeSP parent) {
                for (auto node=parent->firstChild();node;node=node->nextSibling()) {
                    if (!node->visible()) continue;
                    if (dynamic_cast<KisGroupLayer *>(node.data())) collect(node);
                    else if (dynamic_cast<KisPaintLayer *>(node.data())) {
                        m_wgpu->layers.append(node);
                        m_wgpu->initialSequences.append(node->paintDevice()->sequenceNumber());
                        m_wgpu->layerSlots.append(info->opacities.size());
                        info->opacities.append(node->opacity());
                        if (m_wgpu->gpuIndirect && node==m_wgpu->indirectNode) info->opacities.append(255);
                    }
                }
            };
            collect(canvas()->image()->rootLayer());
        }
        for (int i=0;i<m_wgpu->layers.size();++i) {
            auto *layer=static_cast<KisPaintLayer *>(m_wgpu->layers[i].data());
            auto device=layer->paintDevice();
            const bool indirect=m_wgpu->gpuIndirect && m_wgpu->layers[i]==m_wgpu->indirectNode;
            if (!initial && !indirect && !layer->hasTemporaryTarget() && device->sequenceNumber()==m_wgpu->initialSequences[i]) continue;
            const QRect update=initial ? canvas()->image()->bounds() : clipped;
            const int slot=m_wgpu->layerSlots[i];
            if (indirect) {
                KisIndirectPaintingSupport::ReadLocker locker(layer);
                const auto target=layer->temporaryTarget();
                if (initial || !target) info->layers.append({slot,device->convertToQImage(
                    KoColorSpaceRegistry::instance()->rgb8()->profile(),update.x(),update.y(),update.width(),update.height()),update});
                QImage temporary;
                if (target) temporary=target->convertToQImage(KoColorSpaceRegistry::instance()->rgb8()->profile(),
                    update.x(),update.y(),update.width(),update.height());
                else { temporary=QImage(update.size(),QImage::Format_RGBA8888_Premultiplied);temporary.fill(Qt::transparent); }
                info->layers.append({slot+1,temporary,update});
                continue;
            }
            if (!qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_SKIP_CPU_COMPOSE")) device=layer->projection();
            else {
                KisIndirectPaintingSupport::ReadLocker locker(layer);
                if (layer->hasTemporaryTarget()) {
                    KisPaintDeviceSP merged=new KisPaintDevice(device->colorSpace());
                    KisPainter::copyAreaOptimized(update.topLeft(),device,merged,update);
                    KisPainter painter(merged);layer->setupTemporaryPainter(&painter);
                    painter.bitBlt(update.topLeft(),layer->temporaryTarget(),update);
                    device=merged;
                }
            }
            info->layers.append({slot,device->convertToQImage(KoColorSpaceRegistry::instance()->rgb8()->profile(),
                update.x(),update.y(),update.width(),update.height()),update});
        }
        info->captureNs=Performance::nowNs()-captureStart;
        return info;
    }
    if (!clipped.isEmpty()) info->patch = canvas()->image()->projection()->convertToQImage(
        KoColorSpaceRegistry::instance()->rgb8()->profile(),clipped.x(),clipped.y(),clipped.width(),clipped.height());
    info->captureNs=Performance::nowNs()-captureStart;
    return info;
}
QRect WgpuCanvas::updateCanvasProjection(KisUpdateInfoSP info)
{
    if (!m_wgpu->rawProjection) return KisQPainterCanvas::updateCanvasProjection(info);
    auto *patch = dynamic_cast<RawProjectionPatch *>(info.data());
    if (!patch) return {};
    Performance::record(*this,m_wgpu->gpuLayers ? QStringLiteral("layer_capture") : QStringLiteral("projection_capture"),
        patch->captureNs,property(Performance::inputProperty).toLongLong(),Performance::nowNs());
    if (m_wgpu->gpuLayers) {
        if (patch->deferred) m_wgpu->presenter->queueLayerDirty(patch->dirtyImageRect());
        else m_wgpu->presenter->queueLayerPatches(patch->layers,patch->opacities);
        if (!patch->opacities.isEmpty()) m_wgpu->layersQueued=true;
    }
    else {
        if (patch->patch.isNull()) return {};
        m_wgpu->presenter->queueProjectionPatch(patch->patch,patch->dirtyImageRect());
    }
    return coordinatesConverter()->imageToViewport(patch->dirtyImageRect()).toAlignedRect();
}
quint64 WgpuCanvas::uploadedBytes() const { return m_wgpu->presenter->uploadedBytes(); }
quint64 WgpuCanvas::submittedFrames() const { return m_wgpu->presenter->submittedFrames(); }
QString WgpuCanvas::presentationError() const { return m_wgpu->presenter->error(); }

void WgpuCanvas::updateCanvasImage(const QRect &rect) {
    setProperty("librepaintCanvasProjectionGeneration",qulonglong(++m_wgpu->generation));
    scheduleFrame(rect);
}
void WgpuCanvas::updateCanvasDecorations(const QRect &rect) { scheduleFrame(rect); }
void WgpuCanvas::paintEvent(QPaintEvent *event) {
    if(qEnvironmentVariableIntValue("LIBREPAINT_WGPU_SKIP_REDUNDANT") && !m_wgpu->frame.isNull()) return;
    scheduleFrame(event->rect());
}

void WgpuCanvas::resizeEvent(QResizeEvent *event)
{
    KisQPainterCanvas::resizeEvent(event);
    scheduleFrame(rect());
}

void WgpuCanvas::showEvent(QShowEvent *event)
{
    KisQPainterCanvas::showEvent(event);
    scheduleFrame(rect());
}

void WgpuCanvas::scheduleFrame(const QRect &update)
{
    if (update.isEmpty()) return;
    m_wgpu->dirty |= update.intersected(rect());
    if (!m_wgpu->dirty.isEmpty() && !m_wgpu->composeTimer.isActive()) m_wgpu->composeTimer.start();
}

void WgpuCanvas::compose()
{
    if (!canvas()->image() || size().isEmpty()) return;
    m_wgpu->presenter->setProjectionGeneration(m_wgpu->generation);
    if (m_wgpu->gpuProjection) {
        Performance::Measurement measurement(*this,"compose");
        if (m_wgpu->gpuIndirect && !m_wgpu->indirectNode) {
            m_wgpu->indirectNode=canvas()->resourceManager()->resource(KoCanvasResource::CurrentKritaNode).value<KisNodeWSP>();
            m_wgpu->layersQueued=false;
        }
        if (m_wgpu->gpuLayers && !m_wgpu->layersQueued) {
            { QMutexLocker lock(&m_wgpu->layersMutex);m_wgpu->layers.clear();m_wgpu->initialSequences.clear();m_wgpu->layerSlots.clear(); }
            updateCanvasProjection(captureProjection(canvas()->image()->bounds(),true));
        }
        if (m_wgpu->rawProjection && m_wgpu->frame.size()!=canvas()->image()->size()) {
            m_wgpu->frame=QImage(canvas()->image()->size(),QImage::Format_ARGB32);
            m_wgpu->frame.fill(Qt::white);
        }
        const QImage image = m_wgpu->rawProjection ? m_wgpu->frame : canvasProjectionImage();
        if (image.isNull()) return;
        const QTransform t = m_wgpu->rawProjection ? coordinatesConverter()->imageToWidgetTransform().inverted()
                                                  : coordinatesConverter()->viewportToWidgetTransform().inverted();
        const QColor c = borderColor();
        const QRectF r = coordinatesConverter()->imageRectInWidgetPixels();
        m_wgpu->presenter->setProjectionGeometry({float(t.m11()),float(t.m12()),float(t.m21()),float(t.m22()),
            float(t.dx()),float(t.dy()),float(devicePixelRatioF()),0,
            float(c.redF()),float(c.greenF()),float(c.blueF()),float(c.alphaF()),
            float(r.left()),float(r.top()),float(r.right()),float(r.bottom())});
        const QRect dirty = m_wgpu->rawProjection ? image.rect()
            : coordinatesConverter()->widgetToViewport(m_wgpu->dirty).toAlignedRect().intersected(image.rect());
        const bool resized = m_wgpu->frame.size()!=image.size();
        m_wgpu->dirty = {};
        m_wgpu->frame = image;
        m_wgpu->presenter->setImage(image, resized ? image.rect() : dirty);
        return;
    }
    const qreal ratio = devicePixelRatioF();
    const QSize pixels = size() * ratio;
    QRect dirty = m_wgpu->dirty.intersected(rect());
    m_wgpu->dirty = {};
    {
        Performance::Measurement measurement(*this, "compose");
        if (m_wgpu->frame.size() != pixels || m_wgpu->frame.devicePixelRatio() != ratio) {
            m_wgpu->frame = QImage(pixels, QImage::Format_ARGB32_Premultiplied);
            m_wgpu->frame.setDevicePixelRatio(ratio);
            dirty = rect();
        }
        if (dirty.isEmpty()) return;
        QPainter painter(&m_wgpu->frame);
        painter.setClipRect(dirty);
        paintCanvas(painter, dirty);
        painter.end();
    }
    const QRect physical = QRectF(dirty.x() * ratio, dirty.y() * ratio,
                                 dirty.width() * ratio, dirty.height() * ratio).toAlignedRect();
    m_wgpu->presenter->setImage(m_wgpu->frame, physical.intersected(m_wgpu->frame.rect()));
}
}
