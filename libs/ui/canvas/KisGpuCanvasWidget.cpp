/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasWidget_p.h"
#include "kis_coordinates_converter.h"
#include "kis_canvas2.h"
#include "kis_display_color_converter.h"
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
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>
#include <KoCompositeOpIds.h>
#include <KoToolProxy.h>
#include <KisOptimizedBrushOutline.h>
#include <kis_selection.h>
#include <application/ui/workspace/KisViewManager.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <QApplication>
#include <QMetaObject>
#include <QPainter>
#include <QPaintEngine>
#include <QPaintDevice>
#include <QPointer>
#include <QScopeGuard>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <cmath>

namespace {
class ToolStrokeEngine final : public QPaintEngine
{
public:
    explicit ToolStrokeEngine(QList<LibrePaint::Canvas::Gpu::ToolStroke> &strokes)
        : QPaintEngine(PainterPaths | PrimitiveTransform | AlphaBlend | Antialiasing), m_strokes(strokes) {}
    bool valid() const { return m_valid; }
    bool begin(QPaintDevice *) override { setActive(true); return true; }
    bool end() override { setActive(false); return true; }
    Type type() const override { return User; }
    void updateState(const QPaintEngineState &state) override
    {
        m_pen = state.pen(); m_transform = state.transform(); m_opacity = state.opacity();
    }
    void drawPath(const QPainterPath &path) override
    {
        if (!m_valid || m_pen.style() == Qt::NoPen || path.isEmpty()) return;
        if (path.elementCount() > 65536 || m_strokes.size() >= 64) { m_valid = false; return; }
        LibrePaint::Canvas::Gpu::ToolStroke stroke;
        stroke.contours = path.toSubpathPolygons(m_transform);
        for (const auto &contour : stroke.contours) {
            if (contour.size() > 65536 - m_points) { m_valid = false; return; }
            m_points += contour.size();
        }
        stroke.color = m_pen.color();
        stroke.color.setAlphaF(stroke.color.alphaF() * m_opacity);
        stroke.thickness = m_pen.widthF();
        if (!stroke.thickness) stroke.thickness = 1;
        else if (!m_pen.isCosmetic()) stroke.thickness *= std::sqrt(std::abs(m_transform.determinant()));
        m_strokes.append(std::move(stroke));
    }
    // The image owner presents raster previews independently of tool strokes.
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override {}
    void drawImage(const QRectF &, const QImage &, const QRectF &, Qt::ImageConversionFlags) override {}
private:
    QList<LibrePaint::Canvas::Gpu::ToolStroke> &m_strokes;
    QPen m_pen;
    QTransform m_transform;
    qreal m_opacity = 1;
    qsizetype m_points = 0;
    bool m_valid = true;
};

class ToolStrokeDevice final : public QPaintDevice
{
public:
    ToolStrokeDevice(QList<LibrePaint::Canvas::Gpu::ToolStroke> &strokes, QSize size)
        : m_engine(strokes), m_size(size) {}
    QPaintEngine *paintEngine() const override { return &m_engine; }
    bool valid() const { return m_engine.valid(); }
protected:
    int metric(PaintDeviceMetric value) const override
    {
        switch (value) {
        case PdmWidth: return m_size.width();
        case PdmHeight: return m_size.height();
        case PdmDpiX: case PdmDpiY: case PdmPhysicalDpiX: case PdmPhysicalDpiY: return 96;
        case PdmDepth: return 32;
        case PdmDevicePixelRatio: return 1;
        case PdmDevicePixelRatioScaled: return int(QPaintDevice::devicePixelRatioFScale());
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        case PdmDevicePixelRatioF_EncodedA: case PdmDevicePixelRatioF_EncodedB:
            return QPaintDevice::encodeMetricF(value, 1.0);
#endif
        default: return 0;
        }
    }
private:
    mutable ToolStrokeEngine m_engine;
    QSize m_size;
};

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
    updateDisplayConversion();
    connect(canvas->displayColorConverter(), &KisDisplayColorConverter::displayConfigurationChanged,
            this, [this] { m_displayConversionDirty = true; refresh(); });
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
    if (!m_window || m_collectingToolDecorations || m_refreshQueued) return;
    m_refreshQueued = true;
    // Tool cancellation can request a redraw while its path points are being
    // released. Collect decorations after that input/lifetime transition ends.
    QMetaObject::invokeMethod(this, [this] {
        m_refreshQueued = false;
        refreshNow();
    }, Qt::QueuedConnection);
}

void KisGpuCanvasWidget::refreshNow()
{
    if (!m_window || m_collectingToolDecorations) return;
    const auto image = canvas()->image()->gpuImage();
    if (image && image != m_image) {
        KIS_SAFE_ASSERT_RECOVER_RETURN(&image->execution() == &m_image->execution());
        disconnect(m_image.data(), nullptr, this, nullptr);
        m_image = image;
        m_window->setDocument(m_image->handle());
        m_displayConversionDirty = true;
        channelSelectionChanged(canvas()->image()->rootLayer()->channelFlags());
        connect(m_image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this, [this] { refresh(); });
    }
    if (!updateDisplayConversion()) return;
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
        QList<LibrePaint::Canvas::Gpu::ToolStroke> strokes;
        ToolStrokeDevice commands(strokes, size());
        QPainter painter(&commands);
        painter.setTransform(coordinatesConverter()->flakeToWidgetTransform());
        QPointer<KisGpuCanvasWidget> guard(this);
        m_collectingToolDecorations = true;
        const auto finishCollection = qScopeGuard([guard] {
            if (guard) guard->m_collectingToolDecorations = false;
        });
        toolProxy()->paint(painter, *canvas()->viewConverter());
        painter.end();
        if (!guard || !m_window || !commands.valid()) return false;
        m_toolOutline.painted = std::move(strokes);
    }
    if (const auto selection = canvas()->image()->globalSelection(); selection && selection->outlineCacheValid()) {
        const auto transform = coordinatesConverter()->imageToWidgetTransform();
        for (const auto &contour : selection->outlineCache().toSubpathPolygons(transform))
            m_toolOutline.contours.append(contour);
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
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TabletPress ||
            event->type() == QEvent::TouchBegin) {
            QPointer<KisGpuCanvasWidget> guard(this);
            setFocus(Qt::MouseFocusReason);
            if (!guard) return true;
        }
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

bool KisGpuCanvasWidget::updateDisplayConversion()
{
    using namespace LibrePaint::Image::Gpu;
    const auto requested = m_image->execution().requested(m_image->handle());
    // A retained version without an ICC leaves the encoding statement to the
    // image color space; a version with one is authoritative.
    QByteArray iccProfile = requested ? requested->colorProfile : QByteArray();
    const KoColorSpace *imageColorSpace = canvas()->image() ? canvas()->image()->colorSpace() : nullptr;
    if (requested && iccProfile.isEmpty() && imageColorSpace && imageColorSpace->profile())
        iccProfile = imageColorSpace->profile()->rawData();
    if (!requested || iccProfile.isEmpty()) {
        canvas()->viewManager()->showFloatingMessage(
            QStringLiteral("GPU canvas projection has no ICC encoding"), QIcon());
        return false;
    }
    KoColorConversionStages::Encoding input;
    input.depth = requested->format == PixelFormat::Rgba16
        ? KoColorConversionStages::Encoding::U16 : KoColorConversionStages::Encoding::U8;
    input.iccProfile = iccProfile;
    if (!m_displayConversionDirty && m_displayEncoding &&
        m_displayEncoding->depth == input.depth && m_displayEncoding->iccProfile == input.iccProfile)
        return true;
    const auto depth = input.depth == KoColorConversionStages::Encoding::U16
        ? Integer16BitsColorDepthID : Integer8BitsColorDepthID;
    auto *registry = KoColorSpaceRegistry::instance();
    const KoColorProfile *profile = registry->createColorProfileFromData(
        RGBAColorModelID.id(), depth.id(), input.iccProfile);
    const KoColorSpace *source = profile
        ? registry->colorSpace(RGBAColorModelID.id(), depth.id(), profile) : nullptr;
    const auto result = canvas()->displayColorConverter()->canvasDisplayStages(source);
    if (result.status != KoColorConversionStagesResult::Success ||
        !m_window->setDisplayConversion(m_image->handle(), result.value)) {
        canvas()->viewManager()->showFloatingMessage(result.diagnostic.isEmpty()
            ? QStringLiteral("GPU canvas rejected display conversion") : result.diagnostic, QIcon());
        return false;
    }
    m_displayEncoding = std::move(input);
    m_displayConversionDirty = false;
    return true;
}
