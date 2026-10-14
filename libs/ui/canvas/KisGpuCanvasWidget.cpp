/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasWidget_p.h"
#include "kis_coordinates_converter.h"
#include "kis_canvas2.h"
#include <kis_image.h>
#include <application/ui/workspace/KisViewManager.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <QCoreApplication>
#include <QResizeEvent>
#include <QVBoxLayout>

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
    if (!m_window) return;
    const auto image = canvas()->image()->gpuImage();
    if (image && image != m_image) {
        KIS_SAFE_ASSERT_RECOVER_RETURN(&image->execution() == &m_image->execution());
        disconnect(m_image.data(), nullptr, this, nullptr);
        m_image = image;
        m_window->setDocument(m_image->handle());
        connect(m_image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this, [this] { refresh(); });
    }
    m_window->setDocumentToWindow(coordinatesConverter()->imageToWidgetTransform());
    m_window->refresh();
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
            QCoreApplication::sendEvent(this, event);
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
void KisGpuCanvasWidget::channelSelectionChanged(const QBitArray &) { refresh(); }
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
