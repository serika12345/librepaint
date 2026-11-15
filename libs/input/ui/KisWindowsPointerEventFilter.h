/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_WINDOWS_POINTER_EVENT_FILTER_H
#define KIS_WINDOWS_POINTER_EVENT_FILTER_H

#include "kritainputui_export.h"

#include <QAbstractNativeEventFilter>
#include <QPoint>
#include <QPointer>
#include <QSize>
#include <QWidget>
#include <QtGlobal>

#include <array>

class KisInputManager;

/**
 * The parts of the Windows pointer handling that do not need a HWND.
 *
 * The Qt fork commits c41d66281e (Enter/Leave and the cursor when the stylus
 * touches the non-client area) and 33a49f25d1 (the press-and-hold gesture
 * turning into a right click) are reimplemented for official Qt by a native
 * event filter. The message classification, the coordinate conversion and the
 * GESTURECONFIG list are pure functions so the contract stays testable on the
 * platforms that have no Windows SDK.
 */
namespace KisWindowsPointer
{

// Message identifiers from winuser.h. They are repeated here so this header
// compiles without the Windows SDK.
constexpr quint32 NonClientPointerUpdate = 0x0241; // WM_NCPOINTERUPDATE
constexpr quint32 NonClientPointerDown = 0x0242;   // WM_NCPOINTERDOWN
constexpr quint32 NonClientPointerUp = 0x0243;     // WM_NCPOINTERUP
constexpr quint32 NonClientMouseMove = 0x00A0;     // WM_NCMOUSEMOVE

enum class NonClientMessage {
    None,
    Move,
    Press,
    Release
};

/**
 * The pointer messages that Windows sends while the stylus is over the window
 * frame, the title bar or another non-client area. Anything else is left to
 * Qt, including WM_POINTER* for the client area.
 */
inline NonClientMessage nonClientMessage(quint32 message)
{
    switch (message) {
    case NonClientPointerUpdate:
    case NonClientMouseMove:
        return NonClientMessage::Move;
    case NonClientPointerDown:
        return NonClientMessage::Press;
    case NonClientPointerUp:
        return NonClientMessage::Release;
    default:
        return NonClientMessage::None;
    }
}

/**
 * Windows packs a screen point into the low 32 bits of lParam as two signed
 * 16-bit values, low word first. Negative values address a monitor that is
 * left of or above the primary one.
 */
inline QPoint screenPointFromLParam(qintptr lParam)
{
    const quint32 packed = quint32(lParam) & 0xffffffffu;
    return QPoint(qint16(packed & 0xffffu), qint16((packed >> 16) & 0xffffu));
}

/**
 * Canvas position in the same native (physical pixel) unit as the screen
 * point. The caller keeps both values in native pixels, so a monitor with
 * another scale factor does not change the answer.
 */
inline QPoint canvasLocalPoint(QPoint nativeScreenPoint, QPoint nativeCanvasOrigin)
{
    return nativeScreenPoint - nativeCanvasOrigin;
}

/**
 * The canvas size in native pixels, matching the unit of canvasLocalPoint().
 */
inline QSize nativeCanvasSize(QSize logicalCanvasSize, qreal devicePixelRatio)
{
    const qreal dpr = devicePixelRatio > 0.0 ? devicePixelRatio : 1.0;
    return QSize(qRound(logicalCanvasSize.width() * dpr),
                 qRound(logicalCanvasSize.height() * dpr));
}

/**
 * Whether the pen is over the canvas. A point on the right or bottom edge
 * belongs to the next widget, and a negative point is outside.
 */
inline bool isInsideCanvas(QPoint localPoint, QSize nativeSize)
{
    return localPoint.x() >= 0 && localPoint.y() >= 0 &&
        localPoint.x() < nativeSize.width() && localPoint.y() < nativeSize.height();
}

/**
 * Windows shows a resize cursor when the pen touches the frame, and it does
 * not switch back on its own because Qt never saw the canvas re-enter. The
 * canvas cursor is restored only when this message really entered the canvas.
 */
inline bool restoresCanvasCursor(bool canvasEnteredByThisMessage)
{
    return canvasEnteredByThisMessage;
}

// Gesture identifiers and flags from winuser.h, for the same reason as the
// message identifiers above.
constexpr quint32 GestureIdPressAndTap = 7;     // GID_PRESSANDTAP
constexpr quint32 GestureConfigPressAndTap = 1; // GC_PRESSANDTAP
constexpr quint32 GestureIdZoom = 3;            // GID_ZOOM
constexpr quint32 GestureIdPan = 4;             // GID_PAN
constexpr quint32 GestureIdRotate = 5;          // GID_ROTATE
constexpr quint32 GestureIdTwoFingerTap = 6;    // GID_TWOFINGERTAP

struct GestureConfig {
    quint32 id {0};
    quint32 want {0};
    quint32 block {0};
};

/**
 * The GESTURECONFIG list for the canvas window. SetGestureConfig changes only
 * the gestures that are listed, so a single press-and-tap entry turns off the
 * long press that Windows Ink turns into a right click and leaves every other
 * gesture (zoom, pan, rotate, two-finger tap) as it is. L1 experiment for Q4
 * (official Qt); stage 11 removes it when official Qt handles the long press.
 */
inline std::array<GestureConfig, 1> canvasGestureConfig()
{
    return {GestureConfig{GestureIdPressAndTap, 0, GestureConfigPressAndTap}};
}

} // namespace KisWindowsPointer

/**
 * Watches the Windows messages for the canvas window that Qt does not turn
 * into Qt events: the pointer over the non-client area, and the gesture
 * configuration of the window.
 *
 * The filter does nothing on other platforms and nothing before an input
 * manager registers a canvas. It returns false from nativeEventFilter() so Qt
 * keeps handling every message as before.
 */
class KRITAINPUTUI_EXPORT KisWindowsPointerEventFilter : public QAbstractNativeEventFilter
{
public:
    KisWindowsPointerEventFilter();
    ~KisWindowsPointerEventFilter() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

    /**
     * Tells the filter which canvas follows the pointer. The input manager
     * calls this when it starts or stops filtering a canvas widget, so the
     * filter stays idle while there is no canvas.
     */
    void setCanvas(KisInputManager *inputManager, QWidget *canvasWidget);
    QWidget *canvasWidget() const;

    /**
     * The filter installed by main() for the application, or nullptr when it
     * is not installed (tests and the other platforms).
     */
    static KisWindowsPointerEventFilter *instance();

private:
    // Stored as QObject so this header stays usable without the input
    // manager headers; the implementation casts back to KisInputManager.
    QPointer<QObject> m_inputManager;
    QPointer<QWidget> m_canvasWidget;
    static KisWindowsPointerEventFilter *s_instance;
};

#endif // KIS_WINDOWS_POINTER_EVENT_FILTER_H
