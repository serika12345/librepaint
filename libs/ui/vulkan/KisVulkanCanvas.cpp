/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisVulkanCanvas.h"
#include "KisVulkanFrameWindow.h"
#include "canvas/kis_canvas2.h"
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QVulkanInstance>
#include <QtMath>

struct KisVulkanCanvas::Private
{
    QVulkanInstance instance;
    KisVulkanFrameWindow *window = nullptr;
    QWidget *container = nullptr;
    QString initializationError;
    bool framePending = false;
};

KisVulkanCanvas::KisVulkanCanvas(KisCanvas2 *canvas, KisCoordinatesConverter *converter, QWidget *parent)
    : KisQPainterCanvas(canvas, converter, parent)
    , m_d(std::make_unique<Private>())
{
    m_d->instance.setApiVersion(QVersionNumber(1, 1, 0));
    m_d->instance.setExtensions({"VK_KHR_get_physical_device_properties2"});
    if (qEnvironmentVariableIntValue("LIBREPAINT_VULKAN_VALIDATION")) {
        if (!m_d->instance.supportedLayers().contains("VK_LAYER_KHRONOS_validation")) {
            m_d->initializationError = QStringLiteral("Vulkan validation layer unavailable");
            return;
        }
        m_d->instance.setLayers({"VK_LAYER_KHRONOS_validation"});
        m_d->instance.installDebugOutputFilter([this](auto severity, auto, const void *data) {
            if (severity & QVulkanInstance::ErrorSeverity) {
                const auto *diagnostic = static_cast<const VkDebugUtilsMessengerCallbackDataEXT *>(data);
                const QString message = QString::fromUtf8(diagnostic->pMessage);
                QMetaObject::invokeMethod(this, [this, message] {
                    Q_EMIT presentationFailed(QStringLiteral("Vulkan validation: %1").arg(message));
                }, Qt::QueuedConnection);
            }
            return false;
        });
    }
    if (!m_d->instance.create()) {
        m_d->initializationError = QStringLiteral("Vulkan instance failed: %1").arg(m_d->instance.errorCode());
        return;
    }
    m_d->window = new KisVulkanFrameWindow(&m_d->instance, this);
    m_d->window->setTitle(QStringLiteral("LibrePaint MoltenVK canvas"));
    m_d->container = QWidget::createWindowContainer(m_d->window, this);
    m_d->container->setFocusPolicy(Qt::StrongFocus);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_d->container);
    connect(m_d->window, &KisVulkanFrameWindow::renderFailed,
            this, &KisVulkanCanvas::presentationFailed, Qt::QueuedConnection);
    connect(m_d->window, &KisVulkanFrameWindow::frameSubmitted, this, [] {
        qInfo() << "Canvas backend selected: moltenvk-poc (CPU projection, RGBA8 frame submitted)";
    }, Qt::SingleShotConnection);
    qInfo() << "Canvas backend requested: moltenvk-poc";
}

KisVulkanCanvas::~KisVulkanCanvas()
{
    // Container destroys the Vulkan window while its borrowed instance is alive.
    delete m_d->container;
}

void KisVulkanCanvas::requestFrame()
{
    if (m_d->framePending || !m_d->window || !isVisible()) return;
    m_d->framePending = true;
    QTimer::singleShot(0, this, [this] {
        m_d->framePending = false;
        if (isVisible()) composeFrame();
    });
}

void KisVulkanCanvas::composeFrame()
{
    if (!canvas()->image() || size().isEmpty()) return;
    const qreal ratio = devicePixelRatioF();
    const QSize pixels(qCeil(width() * ratio), qCeil(height() * ratio));
    QImage frame(pixels, QImage::Format_RGBA8888);
    if (frame.isNull()) {
        Q_EMIT presentationFailed(QStringLiteral("Canvas frame allocation failed"));
        return;
    }
    frame.setDevicePixelRatio(ratio);
    frame.fill(Qt::black);
    QPainter painter(&frame);
    drawCanvasContents(painter, rect());
    painter.end();
    m_d->window->setFrame(frame);
}

void KisVulkanCanvas::updateCanvasImage(const QRect &rect)
{
    if (!rect.isEmpty()) requestFrame();
}

void KisVulkanCanvas::updateCanvasDecorations(const QRect &rect)
{
    if (!rect.isEmpty()) requestFrame();
}

void KisVulkanCanvas::paintEvent(QPaintEvent *) { requestFrame(); }

void KisVulkanCanvas::resizeEvent(QResizeEvent *event)
{
    KisQPainterCanvas::resizeEvent(event);
    requestFrame();
}

void KisVulkanCanvas::showEvent(QShowEvent *event)
{
    KisQPainterCanvas::showEvent(event);
    requestFrame();
    QTimer::singleShot(1000, this, [this] {
        if (isVisible() && m_d->window && m_d->window->isExposed() && !m_d->window->isValid()) {
            Q_EMIT presentationFailed(QStringLiteral("Qt Vulkan window initialization failed"));
        }
    });
}

QString KisVulkanCanvas::currentBitDepthUserReport() const
{
    return QStringLiteral("MoltenVK PoC: 8-bit CPU display frame");
}

QString KisVulkanCanvas::initializationError() const
{
    return m_d->initializationError;
}
