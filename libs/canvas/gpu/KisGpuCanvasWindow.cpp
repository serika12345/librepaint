/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuCanvasWindow.h"
#include "KisGpuColorConversion_p.h"
#include <webgpu/wgpu.h>
#include "KisGpuTilePresentation.h"
#include "KisGpuWindowSurface_p.h"
#include "KisGpuViewTransform_p.h"
#include <QPlatformSurfaceEvent>
#include <QGuiApplication>
#include <QThread>
#include <stdexcept>

namespace LibrePaint::Canvas::Gpu {
using namespace Image::Gpu;

struct CanvasWindow::Private
{
    DevicePtr device;
    Execution &execution;
    quint64 generation;
    DocumentHandle handle = 0;
    QTransform transform;
    DisplayChannels channels;
    std::optional<KoColorConversionStages> conversion;
    QColor background{Qt::white};
    std::optional<CheckerPattern> checkers;
    ToolOutline outline;
    std::optional<CropPreview> crop;
    bool recording = false;
    std::unique_ptr<PresentationSurface> surface;
    quintptr nativeSurfaceIdentity = 0;
    std::unique_ptr<TilePresentation> presentation;

    explicit Private(Execution &execution)
        : device(execution.device()), execution(execution), generation(execution.generation()) {}

    void releaseSurface()
    {
        presentation.reset();
        surface.reset();
        nativeSurfaceIdentity = 0;
    }
    QTransform physicalTransform(qreal ratio) const
    {
        return {transform.m11() * ratio, transform.m12() * ratio,
                transform.m21() * ratio, transform.m22() * ratio,
                transform.dx() * ratio, transform.dy() * ratio};
    }
};

CanvasWindow::CanvasWindow(Execution &execution, QWindow *parent) : QWindow(parent)
{
    if (!execution.device() || execution.device()->thread() != QThread::currentThread()) {
        throw std::invalid_argument("GPU canvas requires the shared execution thread");
    }
    m_d = std::make_unique<Private>(execution);
    connectSurface();
#ifdef Q_OS_ANDROID
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state <= Qt::ApplicationHidden) m_d->releaseSurface();
    });
#endif
}

CanvasWindow::~CanvasWindow()
{
    m_d->releaseSurface();
    destroy();
}

void CanvasWindow::connectSurface()
{
    Detail::setNativeSurfaceType(*this);
}

bool CanvasWindow::setDocument(DocumentHandle handle)
{
    if (handle && !m_d->execution.requested(handle)) return false;
    if (m_d->handle != handle) {
        m_d->conversion.reset();
        m_d->outline = {};
        m_d->crop.reset();
    }
    m_d->handle = handle;
    refresh();
    return true;
}

bool CanvasWindow::setDisplayConversion(DocumentHandle handle, KoColorConversionStages conversion)
{
    auto &d = *m_d;
    const auto source = d.execution.requested(handle);
    if (!handle || handle != d.handle || !Detail::displayEncodingMatches(conversion.input, source))
        return false;
    if (d.conversion && Detail::sameDisplayConversion(*d.conversion, conversion)) return true;
    WGPULimits limits{};
    QByteArray packed;
    QString diagnostic;
    if (wgpuDeviceGetLimits(d.execution.device()->nativeDevice(), &limits) != WGPUStatus_Success ||
        !Detail::packDisplayConversion(conversion,
            std::min(quint64(limits.maxBufferSize), quint64(limits.maxStorageBufferBindingSize)),
            packed, diagnostic)) return false;
    d.conversion = std::move(conversion);
    refresh();
    return true;
}

void CanvasWindow::refresh() { requestUpdate(); }
bool CanvasWindow::setDisplayChannels(DisplayChannels channels)
{
    if (channels.rgbaMask > 15) return false;
    m_d->channels = channels;
    refresh();
    return true;
}
bool CanvasWindow::setBackground(QColor color, std::optional<CheckerPattern> checkers)
{
    if (!color.isValid() || (checkers && (!checkers->first.isValid() || !checkers->second.isValid() ||
        checkers->cellSize <= 0 || !Detail::validDocumentTransform(checkers->targetToPattern)))) return false;
    m_d->background = color;
    m_d->checkers = std::move(checkers);
    refresh();
    return true;
}
void CanvasWindow::setToolOutline(ToolOutline outline)
{
    m_d->outline = std::move(outline);
    refresh();
}
void CanvasWindow::setCropPreview(std::optional<CropPreview> crop)
{
    m_d->crop = std::move(crop);
    refresh();
}
bool CanvasWindow::setDocumentToWindow(const QTransform &transform)
{
    if (!Detail::validDocumentTransform(transform)) return false;
    const auto ratio = devicePixelRatio();
    const QTransform physical(transform.m11() * ratio, transform.m12() * ratio,
                              transform.m21() * ratio, transform.m22() * ratio,
                              transform.dx() * ratio, transform.dy() * ratio);
    if (!Detail::validDocumentTransform(physical)) return false;
    m_d->transform = transform;
    refresh();
    return true;
}
QTransform CanvasWindow::documentToWindow() const { return m_d->transform; }
QRect CanvasWindow::documentViewport() const
{
    const auto document = m_d->execution.requested(m_d->handle);
    if (!document || size().isEmpty()) return {};
    return m_d->transform.inverted().mapRect(QRectF(QPointF(0, 0), size()))
        .intersected(QRectF(QPointF(0, 0), document->size)).toAlignedRect();
}

bool CanvasWindow::resetSurface()
{
    auto &d = *m_d;
    const auto next = d.execution.device();
    if (d.recording || !next || d.device->isRecordingFrame() || next->isRecordingFrame()) return false;
    const bool visible = isVisible();
    d.releaseSurface();
    destroy();
    d.device = next;
    d.generation = d.execution.generation();
    connectSurface();
    if (visible) show();
    return true;
}

bool CanvasWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderFrame();
        return true;
    }
    if (event->type() == QEvent::PlatformSurface &&
        static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        m_d->releaseSurface();
    }
    return QWindow::event(event);
}
void CanvasWindow::exposeEvent(QExposeEvent *event)
{
    QWindow::exposeEvent(event);
    if (isExposed()) refresh();
}
void CanvasWindow::resizeEvent(QResizeEvent *event)
{
    QWindow::resizeEvent(event);
    refresh();
}

void CanvasWindow::renderFrame()
{
    auto &d = *m_d;
    if (d.recording || !isExposed() || !d.execution.requested(d.handle) || !d.conversion) return;
    if (d.device != d.execution.device() || d.generation != d.execution.generation()) {
        resetSurface();
        return;
    }
    d.recording = true;
    bool begun = false;
    const auto outcome = [&]() -> ExecutionOutcome {
        try {
            if (d.device->isRecordingFrame()) return {ExecutionCode::Busy, {}};
            if (!Detail::validDocumentTransform(d.physicalTransform(devicePixelRatio()))) {
                return {ExecutionCode::InvalidRequest, QStringLiteral("GPU canvas transform is outside the supported finite range")};
            }
#ifdef Q_OS_ANDROID
            const auto nativeIdentity = Detail::currentNativeSurfaceIdentity(*this);
            if (!nativeIdentity || (d.surface && d.nativeSurfaceIdentity != nativeIdentity)) d.releaseSurface();
            if (!nativeIdentity) return {ExecutionCode::Busy, {}};
#endif
            if (!d.surface) {
                d.surface = Detail::createWindowSurface(d.device, *this, &d.nativeSurfaceIdentity);
                d.presentation = std::make_unique<TilePresentation>(d.device);
            }
            const QSize pixels = QSizeF(size() * devicePixelRatio()).toSize();
            if (d.surface->size() != pixels && !d.surface->configure(pixels)) {
                return {ExecutionCode::DeviceFailure, QStringLiteral("GPU canvas surface could not be created or resized")};
            }
            auto frame = d.execution.beginPresentationFrame(*d.surface);
            if (frame.code == ExecutionCode::SurfaceOutOfDate && d.surface->configure(pixels)) {
                frame = d.execution.beginPresentationFrame(*d.surface);
            }
            if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
            begun = true;
            const auto projected = d.execution.project(d.handle, documentViewport());
            if (projected.code != RenderOutcome::Recorded) {
                d.execution.cancelFrame();
                begun = false;
                const auto code = projected.code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded :
                    projected.code == RenderOutcome::InvalidVersion ? ExecutionCode::InvalidRequest : ExecutionCode::DeviceFailure;
                return {code, projected.diagnostic};
            }
            auto outline = d.outline;
            const auto logicalToPhysical = QTransform::fromScale(devicePixelRatio(), devicePixelRatio());
            for (auto &contour : outline.contours) contour = logicalToPhysical.map(contour);
            auto checkers = d.checkers;
            if (checkers) checkers->targetToPattern =
                QTransform::fromScale(1.0 / devicePixelRatio(), 1.0 / devicePixelRatio()) * checkers->targetToPattern;
            auto crop = d.crop;
            if (crop) crop->toTarget *= logicalToPhysical;
            const auto drawn = d.presentation->record(frame.commands, projected.version,
                                                      {d.device, frame.target, pixels, d.surface->format()},
                                                      d.physicalTransform(devicePixelRatio()), d.background, *d.conversion, d.channels, outline, checkers, crop);
            if (!drawn.recorded) {
                d.execution.cancelFrame();
                begun = false;
                return {ExecutionCode::DeviceFailure, drawn.diagnostic};
            }
            const auto submitted = d.execution.submitFrame();
            begun = false;
            return submitted;
        } catch (const std::exception &exception) {
            if (begun) d.execution.cancelFrame();
            return {ExecutionCode::DeviceFailure, QString::fromUtf8(exception.what())};
        }
    }();
    d.recording = false;
    const auto handle = d.handle;
    const auto generation = d.generation;
    if (outcome.code == ExecutionCode::Success) {
        const auto document = d.execution.published(handle);
        if (document) Q_EMIT frameSubmitted(handle, document->revision(), generation);
    } else if (outcome.code == ExecutionCode::Busy) {
        refresh();
    } else {
        Q_EMIT renderingFailed(handle, generation, outcome.code, outcome.diagnostic);
    }
}
}
