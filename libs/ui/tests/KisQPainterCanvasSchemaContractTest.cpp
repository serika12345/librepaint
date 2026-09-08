/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/kis_qpainter_canvas.h>
#include <widgets/kis_zoom_scrollbar.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_QPAINTER_CANVAS_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisQPainterCanvas::method)), signature>)
#define ASSERT_ZOOM_SCROLLBAR_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisZoomableScrollBar::method)), signature>)
} // namespace

class KisQPainterCanvasSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void widgetEventSignaturesRemainStable();
    void displayAndProjectionConfigurationSignaturesRemainStable();
    void wrapAroundViewingSignaturesRemainStable();
    void projectionUpdateSignaturesRemainStable();
    void widgetAndProcessingStateSignaturesRemainStable();
    void zoomScrollbarTypeAndLifetimeSchemaRemainStable();
    void zoomScrollbarPositionAndScrollSignaturesRemainStable();
    void zoomScrollbarInputEventSignaturesRemainStable();
    void zoomScrollbarConfigurationSignaturesRemainStable();
    void zoomScrollbarNotificationSignaturesRemainStable();
};

void KisQPainterCanvasSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Canvas = KisQPainterCanvas;

    static_assert(std::is_class_v<Canvas>);
    static_assert(std::is_base_of_v<QWidget, Canvas>);
    static_assert(std::is_base_of_v<KisCanvasWidgetBase, Canvas>);
    static_assert(!std::is_default_constructible_v<Canvas>);
    static_assert(std::is_constructible_v<Canvas, KisCanvas2 *, KisCoordinatesConverter *, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Canvas>);
}

void KisQPainterCanvasSchemaContractTest::widgetEventSignaturesRemainStable()
{
    using Canvas = KisQPainterCanvas;

    ASSERT_QPAINTER_CANVAS_SIGNATURE(focusInEvent, void (Canvas::*)(QFocusEvent *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(focusOutEvent, void (Canvas::*)(QFocusEvent *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(hideEvent, void (Canvas::*)(QHideEvent *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(inputMethodEvent, void (Canvas::*)(QInputMethodEvent *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(inputMethodQuery, QVariant (Canvas::*)(Qt::InputMethodQuery) const);
    ASSERT_QPAINTER_CANVAS_SIGNATURE(paintEvent, void (Canvas::*)(QPaintEvent *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(resizeEvent, void (Canvas::*)(QResizeEvent *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(showEvent, void (Canvas::*)(QShowEvent *));
}

void KisQPainterCanvasSchemaContractTest::displayAndProjectionConfigurationSignaturesRemainStable()
{
    using Canvas = KisQPainterCanvas;

    ASSERT_QPAINTER_CANVAS_SIGNATURE(channelSelectionChanged, void (Canvas::*)(const QBitArray &));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(notifyImageColorSpaceChanged, void (Canvas::*)(const KoColorSpace *));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(setDisplayConfig, void (Canvas::*)(const KisDisplayConfig &));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(setDisplayFilter, void (Canvas::*)(QSharedPointer<KisDisplayFilter>));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(setPrescaledProjection, void (Canvas::*)(KisPrescaledProjectionSP));
}

void KisQPainterCanvasSchemaContractTest::wrapAroundViewingSignaturesRemainStable()
{
    using Canvas = KisQPainterCanvas;

    ASSERT_QPAINTER_CANVAS_SIGNATURE(setWrapAroundViewingMode, void (Canvas::*)(bool));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(setWrapAroundViewingModeAxis, void (Canvas::*)(WrapAroundAxis));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(wrapAroundViewingMode, bool (Canvas::*)() const);
    ASSERT_QPAINTER_CANVAS_SIGNATURE(wrapAroundViewingModeAxis, WrapAroundAxis (Canvas::*)() const);
}

void KisQPainterCanvasSchemaContractTest::projectionUpdateSignaturesRemainStable()
{
    using Canvas = KisQPainterCanvas;

    ASSERT_QPAINTER_CANVAS_SIGNATURE(finishResizingImage, void (Canvas::*)(qint32, qint32));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(startUpdateCanvasProjection, KisUpdateInfoSP (Canvas::*)(const QRect &));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(updateCanvasDecorations, void (Canvas::*)(const QRect &));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(updateCanvasImage, void (Canvas::*)(const QRect &));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(updateCanvasProjection, QRect (Canvas::*)(KisUpdateInfoSP));
}

void KisQPainterCanvasSchemaContractTest::widgetAndProcessingStateSignaturesRemainStable()
{
    using Canvas = KisQPainterCanvas;

    ASSERT_QPAINTER_CANVAS_SIGNATURE(isBusy, bool (Canvas::*)() const);
    ASSERT_QPAINTER_CANVAS_SIGNATURE(setLodResetInProgress, void (Canvas::*)(bool));
    ASSERT_QPAINTER_CANVAS_SIGNATURE(widget, QWidget * (Canvas::*)());
}

void KisQPainterCanvasSchemaContractTest::zoomScrollbarTypeAndLifetimeSchemaRemainStable()
{
    using ScrollBar = KisZoomableScrollBar;

    static_assert(std::is_class_v<ScrollBar>);
    static_assert(std::is_constructible_v<ScrollBar, QWidget *>);
    static_assert(std::is_constructible_v<ScrollBar, Qt::Orientation, QWidget *>);
    static_assert(std::has_virtual_destructor_v<ScrollBar>);
}

void KisQPainterCanvasSchemaContractTest::zoomScrollbarPositionAndScrollSignaturesRemainStable()
{
    using ScrollBar = KisZoomableScrollBar;

    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(barPosition, QPoint (ScrollBar::*)());
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(catchTeleports, bool (ScrollBar::*)(QMouseEvent *));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(handleWrap, void (ScrollBar::*)(const QPoint &, const QPoint &));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(handleScroll, void (ScrollBar::*)(const QPoint &));
}

void KisQPainterCanvasSchemaContractTest::zoomScrollbarInputEventSignaturesRemainStable()
{
    using ScrollBar = KisZoomableScrollBar;

    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(tabletEvent, void (ScrollBar::*)(QTabletEvent *));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(mousePressEvent, void (ScrollBar::*)(QMouseEvent *));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(mouseMoveEvent, void (ScrollBar::*)(QMouseEvent *));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(mouseReleaseEvent, void (ScrollBar::*)(QMouseEvent *));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(wheelEvent, void (ScrollBar::*)(QWheelEvent *));
}

void KisQPainterCanvasSchemaContractTest::zoomScrollbarConfigurationSignaturesRemainStable()
{
    using ScrollBar = KisZoomableScrollBar;

    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(setZoomDeadzone, void (ScrollBar::*)(float));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(setWheelOverscrollSensitivity, void (ScrollBar::*)(float));
}

void KisQPainterCanvasSchemaContractTest::zoomScrollbarNotificationSignaturesRemainStable()
{
    using ScrollBar = KisZoomableScrollBar;

    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(zoom, void (ScrollBar::*)(qreal));
    ASSERT_ZOOM_SCROLLBAR_SIGNATURE(overscroll, void (ScrollBar::*)(qreal));
}

#undef ASSERT_QPAINTER_CANVAS_SIGNATURE
#undef ASSERT_ZOOM_SCROLLBAR_SIGNATURE

QTEST_GUILESS_MAIN(KisQPainterCanvasSchemaContractTest)

#include "KisQPainterCanvasSchemaContractTest.moc"
