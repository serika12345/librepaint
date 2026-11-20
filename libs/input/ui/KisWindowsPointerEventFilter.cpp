/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#if defined(_WIN32)
// winuser.h hides POINTER_INFO and GetPointerPenInfoHistory below Windows 8.
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#include "KisWindowsPointerEventFilter.h"

#include <QGuiApplication>
#include <QScreen>
#include <QVector>
#include <QWidget>
#include <qpa/qplatformscreen.h>

#include "KisWindowsPointerTimes.h"
#include "kis_input_manager.h"
#include "wintab/KisWintabMapping.h"

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

    const MSG *msg = static_cast<const MSG *>(message);

    // Qt takes the tablet event time from msg.time, which only moves with the
    // system timer. Every pen sample of a pointer message has its own
    // performance count, so keep them here, before the canvas check, for the
    // input manager to apply. Qt keeps handling the message as before.
    if (msg->message == WM_POINTERDOWN || msg->message == WM_POINTERUPDATE ||
        msg->message == WM_POINTERUP) {

        if (KisWindowsPointerTimes *times = KisWindowsPointerTimes::instance()) {
            const UINT32 pointerId = GET_POINTERID_WPARAM(msg->wParam);
            POINTER_INPUT_TYPE pointerType = PT_POINTER;

            if (::GetPointerType(pointerId, &pointerType) && pointerType == PT_PEN) {
                POINTER_INFO info;
                if (::GetPointerInfo(pointerId, &info)) {
                    UINT32 count = info.historyCount;
                    if (count > 64) {
                        count = 64;
                    }

                    QVector<POINTER_PEN_INFO> history(count);
                    if (count > 0 &&
                        ::GetPointerPenInfoHistory(pointerId, &count, history.data())) {

                        // The screen list uses the same procedure as
                        // KisWintabPacketPeeker::currentScreens: the logical
                        // geometry, the native geometry from QPlatformScreen
                        // and the device pixel ratio of each screen.
                        QVector<KisWintabMapping::Screen> screens;
                        if (QGuiApplication::instance()) {
                            const QList<QScreen *> guiScreens = QGuiApplication::screens();
                            screens.reserve(guiScreens.size());

                            for (QScreen *screen : guiScreens) {
                                KisWintabMapping::Screen entry;
                                entry.logicalRect = screen->geometry();
                                entry.nativeRect = screen->handle()
                                    ? screen->handle()->geometry() : entry.logicalRect;
                                entry.devicePixelRatio = screen->devicePixelRatio();
                                screens.append(entry);
                            }
                        }

                        // GetPointerPenInfoHistory reports the newest sample
                        // first; the times are stored oldest first.
                        QVector<KisWindowsPointerTimes::Sample> samples;
                        samples.reserve(int(count));
                        for (int i = int(count) - 1; i >= 0; --i) {
                            const POINTER_INFO &penInfo = history.at(i).pointerInfo;
                            KisWindowsPointerTimes::Sample sample;
                            sample.messageTime = quint32(msg->time);
                            sample.performanceCount = quint64(penInfo.PerformanceCount);
                            sample.logicalPosition = KisWintabMapping::logicalPosition(
                                QPointF(penInfo.ptPixelLocation.x, penInfo.ptPixelLocation.y),
                                screens);
                            samples.append(sample);
                        }

                        times->addSamples(samples);
                    }
                }
            }
        }
    }

    QWidget *canvas = m_canvasWidget.data();
    KisInputManager *inputManager = qobject_cast<KisInputManager *>(m_inputManager.data());
    if (!canvas || !inputManager) return false;

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
