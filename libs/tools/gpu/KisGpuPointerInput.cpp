/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPointerInput.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QWindow>
#include <cmath>
#include <stdexcept>

namespace { using Input = KisGpuStrokeInput; }

KisGpuPointerInput::KisGpuPointerInput(Input &input, QWindow &window, qsizetype maximum, QObject *parent)
    : QObject(parent), m_input(input), m_window(window), m_maximumSamples(maximum)
{
    if (maximum <= 0) throw std::invalid_argument("Pointer input requires a positive CPU sample limit");
    m_window.installEventFilter(this);
}

KisGpuPointerInput::~KisGpuPointerInput()
{
    m_window.removeEventFilter(this);
    cancel();
}

bool KisGpuPointerInput::setTool(KisGpuLayerDocument::LayerId layer, Input::Configuration configuration,
                               KisGpuLayerDocument::Target target)
{
    if (!layer || !m_input.validConfiguration(configuration) || cancel() == Input::Result::Busy) return false;
    m_tool = {layer, std::move(configuration), target};
    return true;
}

bool KisGpuPointerInput::setWindowToCanvas(const QTransform &transform)
{
    if (!transform.isAffine() || !transform.isInvertible()
        || !std::isfinite(transform.m11()) || !std::isfinite(transform.m12())
        || !std::isfinite(transform.m21()) || !std::isfinite(transform.m22())
        || !std::isfinite(transform.dx()) || !std::isfinite(transform.dy())) return false;
    m_transform = transform;
    return true;
}

void KisGpuPointerInput::finish()
{
    m_queue.clear(); m_token = {}; m_frozen = {};
    m_source = Source::None;
    m_active = m_started = m_released = m_committed = false;
}

Input::Result KisGpuPointerInput::cancel()
{
    if (!m_active) return Input::Result::Accepted;
    const auto result = m_started ? m_input.cancel(m_token) : Input::Result::Accepted;
    if (result != Input::Result::Busy) finish();
    return result;
}

void KisGpuPointerInput::reject(Input::Result result)
{
    cancel();
    Q_EMIT inputRejected(result);
}

void KisGpuPointerInput::start(KisGpuBrushStroke::Sample sample, Source source, bool eraser)
{
    if (m_active) { Q_EMIT inputRejected(Input::Result::Busy); return; }
    if (!m_tool.layer) { Q_EMIT inputRejected(Input::Result::InvalidInput); return; }
    m_frozen = m_tool;
    if (eraser) m_frozen.configuration.brush.operation = KisGpuTileStore::CompositeOp::Erase;
    m_source = source; m_active = true;
    queue(sample);
}

void KisGpuPointerInput::queue(KisGpuBrushStroke::Sample sample)
{
    if (!m_active || m_released) return;
    if (!KisGpuBrushStroke::validSample(sample)) { reject(Input::Result::InvalidInput); return; }
    if (qsizetype(m_queue.size()) >= m_maximumSamples) { reject(Input::Result::QueueFull); return; }
    m_queue.push_back(sample);
    Q_EMIT inputReady();
}

bool KisGpuPointerInput::eventFilter(QObject *object, QEvent *event)
{
    if (object != &m_window) return false;
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape && m_active) {
        cancel(); event->accept(); return true;
    }
    if (event->type() == QEvent::WindowDeactivate && m_active && !m_released) {
        m_released = true; Q_EMIT inputReady();
    }
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove
        || event->type() == QEvent::MouseButtonRelease) {
        const auto mouse = static_cast<QMouseEvent *>(event);
        if (mouse->source() != Qt::MouseEventNotSynthesized) return false;
        const KisGpuBrushStroke::Sample sample{m_transform.map(mouse->position()), 1};
        if (event->type() == QEvent::MouseButtonPress && mouse->button() == Qt::LeftButton) start(sample, Source::Mouse);
        else if (m_source == Source::Mouse && event->type() == QEvent::MouseMove && (mouse->buttons() & Qt::LeftButton)) queue(sample);
        else if (m_source == Source::Mouse && event->type() == QEvent::MouseButtonRelease && mouse->button() == Qt::LeftButton) {
            queue(sample); m_released = m_active; Q_EMIT inputReady();
        } else return false;
        event->accept(); return true;
    }
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletMove || event->type() == QEvent::TabletRelease) {
        const auto tablet = static_cast<QTabletEvent *>(event);
        const KisGpuBrushStroke::Sample sample{m_transform.map(tablet->position()), tablet->pressure()};
        if (event->type() == QEvent::TabletPress && tablet->button() == Qt::LeftButton) {
            start(sample, Source::Tablet, tablet->pointingDevice()
                && tablet->pointingDevice()->pointerType() == QPointingDevice::PointerType::Eraser);
        } else if (m_source == Source::Tablet && event->type() == QEvent::TabletMove) queue(sample);
        else if (m_source == Source::Tablet && event->type() == QEvent::TabletRelease) {
            queue(sample); m_released = m_active; Q_EMIT inputReady();
        } else return false;
        event->accept(); return true;
    }
    return false;
}

Input::Result KisGpuPointerInput::poll()
{
    if (!m_active) return Input::Result::Accepted;
    if (!m_started) {
        const auto started = m_input.begin(m_frozen.layer, m_frozen.configuration, m_frozen.target);
        if (started.result != Input::Result::Accepted) {
            const auto gpu = m_input.lastGpuError();
            if (started.result != Input::Result::Busy && started.result != Input::Result::RecoveryBudgetExceeded
                && !(started.result == Input::Result::GpuRejected && (gpu == KisGpuTileStore::Error::BudgetExceeded
                    || gpu == KisGpuTileStore::Error::QueueFull || gpu == KisGpuTileStore::Error::DeviceLost))) reject(started.result);
            return started.result;
        }
        m_token = started.token; m_started = true;
    }
    if (m_input.state() == Input::State::Finished || m_input.state() == Input::State::Cancelled) {
        finish(); return Input::Result::Accepted;
    }
    for (int count = 0; count < 64 && !m_queue.empty(); ++count) {
        const auto accepted = m_input.append(m_token, {m_queue.front()});
        if (accepted == Input::Result::QueueFull) return Input::Result::Busy;
        if (accepted != Input::Result::Accepted) { reject(accepted); return accepted; }
        m_queue.pop_front();
    }
    if (m_released && m_queue.empty() && !m_committed) {
        const auto committed = m_input.commit(m_token);
        if (committed != Input::Result::Accepted) { reject(committed); return committed; }
        m_committed = true;
    }
    return Input::Result::Accepted;
}
