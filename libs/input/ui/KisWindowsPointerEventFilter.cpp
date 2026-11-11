/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWindowsPointerEventFilter.h"

#include <QWidget>

#include "kis_input_manager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

KisWindowsPointerEventFilter *KisWindowsPointerEventFilter::s_instance = nullptr;

namespace
{

#ifdef Q_OS_WIN
/**
 * Turns off the press-and-hold gesture that Windows Ink reports as a right
 * click. The window that receives the pen messages is the top-level window of
 * the canvas; an alien canvas widget has no HWND of its own. SetGestureConfig
 * changes only the listed gestures, so zoom, pan, rotate and two-finger tap
 * keep their current settings.
 */
void applyCanvasGestureConfig(QWidget *canvasWidget)
{
    if (!canvasWidget) return;

    QWidget *window = canvasWidget->window();
    if (!window) return;

    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) return;

    const auto config = KisWindowsPointer::canvasGestureConfig();
    GESTURECONFIG nativeConfig[config.size()];
    for (size_t i = 0; i < config.size(); ++i) {
        nativeConfig[i].dwID = config[i].id;
        nativeConfig[i].dwWant = config[i].want;
        nativeConfig[i].dwBlock = config[i].block;
    }

    ::SetGestureConfig(hwnd, 0, UINT(config.size()), nativeConfig,
                       sizeof(GESTURECONFIG));
}
#endif

} // namespace

KisWindowsPointerEventFilter::KisWindowsPointerEventFilter()
{
    s_instance = this;
}

KisWindowsPointerEventFilter::~KisWindowsPointerEventFilter()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

KisWindowsPointerEventFilter *KisWindowsPointerEventFilter::instance()
{
    return s_instance;
}

QWidget *KisWindowsPointerEventFilter::canvasWidget() const
{
    return m_canvasWidget.data();
}

void KisWindowsPointerEventFilter::setCanvas(KisInputManager *inputManager, QWidget *canvasWidget)
{
    const bool canvasChanged = m_canvasWidget.data() != canvasWidget;

    m_inputManager = inputManager;
    m_canvasWidget = canvasWidget;

#ifdef Q_OS_WIN
    if (canvasWidget && canvasChanged) {
        applyCanvasGestureConfig(canvasWidget);
    }
#endif
}

bool KisWindowsPointerEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG") return false;

    QWidget *canvas = m_canvasWidget.data();
    KisInputManager *inputManager = qobject_cast<KisInputManager *>(m_inputManager.data());
    if (!canvas || !inputManager) return false;

    const MSG *msg = static_cast<const MSG *>(message);
    if (KisWindowsPointer::nonClientMessage(msg->message) ==
        KisWindowsPointer::NonClientMessage::None) {
        return false;
    }

    QWidget *window = canvas->window();
    if (!window || reinterpret_cast<HWND>(window->winId()) != msg->hwnd) {
        return false;
    }

    // The top-level client origin comes from Win32 and the canvas offset from
    // Qt, so the native pixels stay right on a monitor with another scale
    // factor.
    POINT clientOrigin{0, 0};
    if (!::ClientToScreen(msg->hwnd, &clientOrigin)) {
        return false;
    }

    const qreal dpr = canvas->devicePixelRatioF();
    const QPointF canvasOffset = QPointF(canvas->mapTo(window, QPoint(0, 0))) * dpr;
    const QPoint nativeOrigin = QPoint(clientOrigin.x, clientOrigin.y) + canvasOffset.toPoint();

    const QPoint localPoint = KisWindowsPointer::canvasLocalPoint(
        KisWindowsPointer::screenPointFromLParam(msg->lParam), nativeOrigin);
    const bool insideCanvas = KisWindowsPointer::isInsideCanvas(
        localPoint, KisWindowsPointer::nativeCanvasSize(canvas->size(), dpr));

    const bool entered = inputManager->notifyNonClientPointer(insideCanvas);
    if (KisWindowsPointer::restoresCanvasCursor(entered)) {
        // The window frame replaced the canvas cursor with a resize cursor.
        canvas->setCursor(canvas->cursor());
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif

    return false;
}
