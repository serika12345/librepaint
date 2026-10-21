/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasWidget_p.h"
#include "kis_coordinates_converter.h"
#include "kis_canvas2.h"
#include <kis_image.h>
#include <kis_group_layer.h>
#include <application/kis_config.h>
#include <kis_config_notifier.h>
#include <KoChannelInfo.h>
#include <KoCanvasResourcesIds.h>
#include <KoCanvasResourceProvider.h>
#include <KoColor.h>
#include <KoColorDisplayRendererInterface.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpIds.h>
#include <KoToolProxy.h>
#include <KisOptimizedBrushOutline.h>
#include <application/ui/workspace/KisViewManager.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <QApplication>
#include <QPainter>
#include <QPicture>
#include <QPointer>
#include <QScopeGuard>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <cmath>

namespace {
class GpuCanvasUpdateInfo final : public KisUpdateInfo
{
public:
    explicit GpuCanvasUpdateInfo(QRect rect) : m_rect(rect) {}
    QRect dirtyImageRect() const override { return m_rect; }
    int levelOfDetail() const override { return 0; }
private:
    QRect m_rect;
};
}

KisGpuCanvasWidget::KisGpuCanvasWidget(KisCanvas2 *canvas, KisCoordinatesConverter *converter,
                                     LibrePaint::Image::Gpu::PaintImageSP image, QWidget *parent)
    : QWidget(parent), KisCanvasWidgetBase(canvas, converter), m_image(std::move(image)),
      m_window(new LibrePaint::Canvas::Gpu::CanvasWindow(m_image->execution())),
      m_container(QWidget::createWindowContainer(m_window, this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_container);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    m_container->setFocusPolicy(Qt::NoFocus);
    m_window->installEventFilter(this);
    m_window->setDocument(m_image->handle());
    channelSelectionChanged(canvas->image()->rootLayer()->channelFlags());
    updateBackground();
    connect(KisConfigNotifier::instance(), &KisConfigNotifier::configChanged, this, [this] {
        const KisImageSP image = this->canvas()->image();
        if (image) channelSelectionChanged(image->rootLayer()->channelFlags());
        refresh();
    });
    connect(toolProxy(), &KoToolProxy::toolChanged, this, [this] { refresh(); });
    connect(canvas->viewManager(), &KisViewManager::viewChanged, this, [this] { refresh(); });
    connect(m_image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this, [this] { refresh(); });
    connect(canvas->image().data(), &KisImage::sigLayersChangedAsync, this, [this] { refresh(); });
    connect(m_window, &LibrePaint::Canvas::Gpu::CanvasWindow::renderingFailed, this,
            [this](quint64, quint64, LibrePaint::Image::Gpu::ExecutionCode, const QString &diagnostic) {
        if (m_window) this->canvas()->viewManager()->showFloatingMessage(diagnostic, QIcon());
    });
}

KisGpuCanvasWidget::~KisGpuCanvasWidget()
{
    disconnectImage();
    delete m_container;
}

void KisGpuCanvasWidget::disconnectImage()
{
    if (!m_window) return;
    disconnect(m_image.data(), nullptr, this, nullptr);
    m_window->setDocument(0);
    m_window->removeEventFilter(this);
    m_window = nullptr;
}

void KisGpuCanvasWidget::refresh()
{
    if (!m_window || m_collectingToolDecorations) return;
    const auto image = canvas()->image()->gpuImage();
    if (image && image != m_image) {
        KIS_SAFE_ASSERT_RECOVER_RETURN(&image->execution() == &m_image->execution());
        disconnect(m_image.data(), nullptr, this, nullptr);
        m_image = image;
        m_window->setDocument(m_image->handle());
        channelSelectionChanged(canvas()->image()->rootLayer()->channelFlags());
        connect(m_image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this, [this] { refresh(); });
    }
    m_window->setDocumentToWindow(coordinatesConverter()->imageToWidgetTransform());
    updateBackground();
    if (!updateToolDecorations()) return;
    m_window->refresh();
}

void KisGpuCanvasWidget::updateBackground()
{
    const KisConfig config(true);
    const auto *converter = coordinatesConverter();
    const auto transform = config.scrollCheckers() ? converter->viewportToWidgetTransform() : QTransform();
    const auto origin = config.scrollCheckers() ? converter->imageToViewport(QPointF()) : QPointF();
    LibrePaint::Canvas::Gpu::CheckerPattern pattern;
    pattern.first = config.checkersColor1();
    pattern.second = config.checkersColor2();
    pattern.cellSize = config.checkSize();
    const qreal period = 2.0 * pattern.cellSize;
    const QPointF phase(std::fmod(origin.x(), period), std::fmod(origin.y(), period));
    pattern.targetToPattern = transform.inverted() * QTransform::fromTranslate(-phase.x(), -phase.y());
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_window->setBackground(config.canvasBorderColor(), std::move(pattern)));
}

bool KisGpuCanvasWidget::updateToolDecorations()
{
    m_toolOutline = {};
    m_cropPreview.reset();
    if (canvas()->viewManager()->canvasBase() == canvas() && toolProxy()) {
        const KisConfig config(true);
        const bool eraser = config.separateEraserCursor() &&
            canvas()->resourceManager()->resource(KoCanvasResource::CurrentEffectiveCompositeOp).toString() == COMPOSITE_ERASE;
        const QColor color = eraser ? config.getEraserCursorMainColor() : config.getCursorMainColor();
        m_toolOutline.color = canvas()->displayRendererInterface()->convertColorToDisplayColorSpace(
            KoColor(color, KoColorSpaceRegistry::instance()->rgb8()));
        // #89: replace this active Qt paint receiver when the remaining tool
        // painting primitives have native GPU consumers. It records commands;
        // document pixels stay with the GPU image owner.
        QPicture commands;
        QPainter painter(&commands);
        painter.setTransform(coordinatesConverter()->flakeToWidgetTransform());
        QPointer<KisGpuCanvasWidget> guard(this);
        m_collectingToolDecorations = true;
        const auto finishCollection = qScopeGuard([guard] {
            if (guard) guard->m_collectingToolDecorations = false;
        });
        toolProxy()->paint(painter, *canvas()->viewConverter());
        painter.end();
        if (!guard || !m_window) return false;
    }
    m_window->setToolOutline(std::move(m_toolOutline));
    m_window->setCropPreview(std::move(m_cropPreview));
    return true;
}

bool KisGpuCanvasWidget::collectCropPreview(LibrePaint::Canvas::Gpu::CropPreview crop)
{
    if (!m_collectingToolDecorations) return false;
    crop.toTarget *= coordinatesConverter()->flakeToWidgetTransform();
    m_cropPreview = std::move(crop);
    return true;
}

void KisGpuCanvasWidget::collectToolOutline(const KisOptimizedBrushOutline &path, int thickness)
{
    if (!m_collectingToolDecorations) return;
    m_toolOutline.thickness = thickness;
    const auto transform = coordinatesConverter()->flakeToWidgetTransform();
    for (auto it = path.begin(); it != path.end(); ++it)
        m_toolOutline.contours.append(transform.map(*it));
}

bool KisGpuCanvasWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_window) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick: case QEvent::MouseMove:
        case QEvent::TabletPress: case QEvent::TabletMove: case QEvent::TabletRelease:
        case QEvent::Wheel: case QEvent::KeyPress: case QEvent::KeyRelease:
        case QEvent::TouchBegin: case QEvent::TouchUpdate: case QEvent::TouchEnd: case QEvent::TouchCancel:
        case QEvent::Enter: case QEvent::Leave:
        case QEvent::FocusIn: case QEvent::FocusOut:
            // Deliver the original native event without clearing its spontaneous flag.
            // Shape tools distinguish a user release from focus-loss cancellation.
            qApp->notify(this, event);
            return true;
        default: break;
        }
    }
    return QWidget::eventFilter(object, event);
}

void KisGpuCanvasWidget::resizeEvent(QResizeEvent *event) { QWidget::resizeEvent(event); refresh(); }
void KisGpuCanvasWidget::focusInEvent(QFocusEvent *event) { if (m_window) processFocusInEvent(event); else QWidget::focusInEvent(event); }
void KisGpuCanvasWidget::focusOutEvent(QFocusEvent *event) { if (m_window) processFocusOutEvent(event); else QWidget::focusOutEvent(event); }
QVariant KisGpuCanvasWidget::inputMethodQuery(Qt::InputMethodQuery query) const { return m_window ? processInputMethodQuery(query) : QWidget::inputMethodQuery(query); }
void KisGpuCanvasWidget::inputMethodEvent(QInputMethodEvent *event) { if (m_window) processInputMethodEvent(event); else QWidget::inputMethodEvent(event); }
bool KisGpuCanvasWidget::callFocusNextPrevChild(bool next) { return focusNextPrevChild(next); }
void KisGpuCanvasWidget::setDisplayFilter(QSharedPointer<KisDisplayFilter>) { refresh(); }
void KisGpuCanvasWidget::notifyImageColorSpaceChanged(const KoColorSpace *) { refresh(); }
void KisGpuCanvasWidget::setWrapAroundViewingMode(bool) {}
void KisGpuCanvasWidget::setWrapAroundViewingModeAxis(WrapAroundAxis) {}
void KisGpuCanvasWidget::channelSelectionChanged(const QBitArray &flags)
{
    if (!m_window) return;
    const KisImageSP image = canvas()->image();
    if (!image) return;
    quint8 mask = 15;
    const auto channels = image->colorSpace()->channels();
    if (flags.size() == channels.size()) {
        mask = 0;
        for (int index = 0; index < channels.size(); ++index)
            if (flags.testBit(index)) mask |= 1u << channels[index]->displayPosition();
    }
    m_window->setDisplayChannels({mask, KisConfig(true).showSingleChannelAsColor()});
}
void KisGpuCanvasWidget::setDisplayConfig(const KisDisplayConfig &) { refresh(); }
void KisGpuCanvasWidget::finishResizingImage(qint32, qint32) { refresh(); }
KisUpdateInfoSP KisGpuCanvasWidget::startUpdateCanvasProjection(const QRect &rect) { return new GpuCanvasUpdateInfo(rect); }
QRect KisGpuCanvasWidget::updateCanvasProjection(KisUpdateInfoSP info)
{
    refresh();
    return info ? coordinatesConverter()->imageToWidget(QRectF(info->dirtyImageRect())).toAlignedRect() : QRect();
}
void KisGpuCanvasWidget::updateCanvasImage(const QRect &) { refresh(); }
void KisGpuCanvasWidget::updateCanvasDecorations(const QRect &) { refresh(); }
