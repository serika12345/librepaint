/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasSession.h"
#include <QEvent>
#include <QWindow>
#include <limits>
#include <stdexcept>

namespace {
using Canvas = Krita::Canvas::DocumentCanvas;
using Document = KisGpuLayerDocument;
using Input = KisGpuStrokeInput;
}

KisGpuCanvasSession::KisGpuCanvasSession(KisGpuDocumentDevice &documents, KisGpuApplicationState &application,
    KisGpuDocumentDevice::DocumentId id, QWindow &window, QRect bounds, QObject *parent)
    : QObject(parent), m_documents(documents), m_application(application), m_id(id), m_window(window),
      m_bounds(bounds), m_model(documents.document(id))
{
    if (!m_model || !bounds.isValid()
        || qint64(bounds.x()) + bounds.width() - 1 > std::numeric_limits<int>::max()
        || qint64(bounds.y()) + bounds.height() - 1 > std::numeric_limits<int>::max())
        throw std::invalid_argument("GPU canvas requires a document and representable canvas bounds");
    m_input = std::make_unique<Input>(m_model);
    m_window.installEventFilter(this);
    connect(&m_documents, &KisGpuDocumentDevice::aboutToReleaseGpu, this, &KisGpuCanvasSession::releaseGpu, Qt::DirectConnection);
    connect(&m_documents, &KisGpuDocumentDevice::gpuRestored, this, &KisGpuCanvasSession::updatePolling, Qt::DirectConnection);
    connect(&m_documents, &KisGpuDocumentDevice::documentAboutToClose, this, [this](auto closing) {
        if (closing != m_id) return;
        releaseGpu(); m_input.reset(); m_model.reset();
        Q_EMIT documentClosed();
    }, Qt::DirectConnection);
    m_poll.setInterval(16);
    m_poll.setTimerType(Qt::PreciseTimer);
    connect(&m_poll, &QTimer::timeout, this, [this] { poll(); });
    updatePolling();
}

KisGpuCanvasSession::~KisGpuCanvasSession()
{
    m_window.removeEventFilter(this);
    releaseGpu();
    m_input.reset();
}

void KisGpuCanvasSession::releaseGpu()
{
    m_poll.stop();
    m_canvas.reset();
    if (m_input) m_input->releaseGpu();
}

void KisGpuCanvasSession::updatePolling()
{
    const auto input = inputState();
    if (m_model && !m_documents.isSuspended()
        && (m_window.isExposed() || input == Input::State::Drawing || input == Input::State::Finishing)) m_poll.start();
    else m_poll.stop();
}

bool KisGpuCanvasSession::eventFilter(QObject *object, QEvent *event)
{
    if (object == &m_window && (event->type() == QEvent::Expose || event->type() == QEvent::Resize
        || event->type() == QEvent::DevicePixelRatioChange)) {
        if (m_canvas) m_canvas->requestRepaint();
        updatePolling();
    }
    return QObject::eventFilter(object, event);
}

void KisGpuCanvasSession::setView(const Krita::Canvas::GpuRenderer::View &view)
{
    m_view = view;
    if (m_canvas) m_canvas->setView(view);
}

Input::Started KisGpuCanvasSession::begin(Document::LayerId layer, Input::Configuration configuration, Document::Target target)
{
    if (!m_input) return {Input::Result::Stale, {}};
    if (!m_application.resumeForInput()) return {Input::Result::Busy, {}};
    const auto started = m_input->begin(layer, std::move(configuration), target);
    updatePolling();
    return started;
}

Input::Result KisGpuCanvasSession::append(const Input::Token &token, const QVector<KisGpuBrushStroke::Sample> &samples)
{
    if (!m_input) return Input::Result::Stale;
    if (m_documents.isSuspended()) m_application.resumeForInput();
    return m_input->append(token, samples);
}

Input::Result KisGpuCanvasSession::replace(const Input::Token &token, const QVector<KisGpuBrushStroke::Sample> &samples)
{
    if (!m_input) return Input::Result::Stale;
    if (m_documents.isSuspended()) m_application.resumeForInput();
    return m_input->replace(token, samples);
}

Input::Result KisGpuCanvasSession::commit(const Input::Token &token)
{
    return m_input ? m_input->commit(token) : Input::Result::Stale;
}

Input::Result KisGpuCanvasSession::cancel(const Input::Token &token)
{
    return m_input ? m_input->cancel(token) : Input::Result::Stale;
}

Input::State KisGpuCanvasSession::inputState() const { return m_input ? m_input->state() : Input::State::Cancelled; }
Krita::Canvas::SurfaceRenderer::Frame KisGpuCanvasSession::lastFrame() const { return m_canvas ? m_canvas->lastFrame() : Krita::Canvas::SurfaceRenderer::Frame{}; }
Krita::Canvas::SurfaceRenderer::Statistics KisGpuCanvasSession::statistics() const { return m_canvas ? m_canvas->statistics() : Krita::Canvas::SurfaceRenderer::Statistics{}; }

KisGpuCanvasSession::Result KisGpuCanvasSession::poll()
{
    m_application.poll();
    if (!m_input) return Result::Closed;
    const auto state = m_model->state();
    if (m_documents.isSuspended() || state == Document::State::Restoring) return Result::Busy;
    if (state == Document::State::Failed) return Result::GpuRejected;
    auto *device = m_documents.device();
    if (!device || !device->available()) return Result::DeviceLost;
    if (!m_canvas && m_window.isExposed()) {
        try {
            m_canvas = std::make_unique<Canvas>(*device, m_documents.store(m_id), m_model, m_window, m_bounds);
            m_canvas->setView(m_view);
            m_error.clear();
        } catch (const std::exception &error) {
            m_error = QString::fromUtf8(error.what());
            return Result::GpuRejected;
        }
    }
    const auto display = m_window.isExposed() && m_canvas ? m_canvas->poll() : Canvas::Result::WindowUnavailable;
    const auto input = m_input->poll();
    updatePolling();
    if (input == Input::Result::GpuRejected || input == Input::Result::RecoveryBudgetExceeded) return Result::GpuRejected;
    switch (display) {
    case Canvas::Result::Updated: return Result::Updated;
    case Canvas::Result::Busy: return Result::Busy;
    case Canvas::Result::GpuRejected: return Result::GpuRejected;
    case Canvas::Result::WindowUnavailable: return Result::WindowUnavailable;
    case Canvas::Result::DeviceLost: return Result::DeviceLost;
    case Canvas::Result::Idle: break;
    }
    return inputState() == Input::State::Drawing || inputState() == Input::State::Finishing ? Result::Busy : Result::Idle;
}
