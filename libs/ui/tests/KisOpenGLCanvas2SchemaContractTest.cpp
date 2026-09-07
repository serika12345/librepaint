/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <opengl/kis_opengl_canvas2.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_OPENGL_CANVAS_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisOpenGLCanvas2::method)), signature>)
} // namespace

class KisOpenGLCanvas2SchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void widgetEventSignaturesRemainStable();
    void openGLRenderingSignaturesRemainStable();
    void displayConfigurationSignaturesRemainStable();
    void wrapAroundViewingSignaturesRemainStable();
    void projectionUpdateSignaturesRemainStable();
    void canvasStateSignaturesRemainStable();
    void configurationSlotSignaturesRemainStable();
};

void KisOpenGLCanvas2SchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Canvas = KisOpenGLCanvas2;

    static_assert(std::is_class_v<Canvas>);
    static_assert(std::is_base_of_v<QOpenGLWidget, Canvas>);
    static_assert(std::is_base_of_v<KisCanvasWidgetBase, Canvas>);
    static_assert(std::is_constructible_v<Canvas,
                                          KisCanvas2 *,
                                          KisCoordinatesConverter *,
                                          QWidget *,
                                          KisImageWSP,
                                          const KisDisplayConfig &,
                                          QSharedPointer<KisDisplayFilter>,
                                          Canvas::BitDepthMode>);
    static_assert(std::has_virtual_destructor_v<Canvas>);
}

void KisOpenGLCanvas2SchemaContractTest::widgetEventSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(event, bool (KisOpenGLCanvas2::*)(QEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(inputMethodQuery, QVariant (KisOpenGLCanvas2::*)(Qt::InputMethodQuery) const);
    ASSERT_OPENGL_CANVAS_SIGNATURE(inputMethodEvent, void (KisOpenGLCanvas2::*)(QInputMethodEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(focusInEvent, void (KisOpenGLCanvas2::*)(QFocusEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(focusOutEvent, void (KisOpenGLCanvas2::*)(QFocusEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(hideEvent, void (KisOpenGLCanvas2::*)(QHideEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(showEvent, void (KisOpenGLCanvas2::*)(QShowEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(paintEvent, void (KisOpenGLCanvas2::*)(QPaintEvent *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(resizeEvent, void (KisOpenGLCanvas2::*)(QResizeEvent *));
}

void KisOpenGLCanvas2SchemaContractTest::openGLRenderingSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(resizeGL, void (KisOpenGLCanvas2::*)(int, int));
    ASSERT_OPENGL_CANVAS_SIGNATURE(initializeGL, void (KisOpenGLCanvas2::*)());
    ASSERT_OPENGL_CANVAS_SIGNATURE(paintGL, void (KisOpenGLCanvas2::*)());
    ASSERT_OPENGL_CANVAS_SIGNATURE(paintToolOutline, void (KisOpenGLCanvas2::*)(const KisOptimizedBrushOutline &, int));
    ASSERT_OPENGL_CANVAS_SIGNATURE(openGLImageTextures, KisOpenGLImageTexturesSP (KisOpenGLCanvas2::*)() const);
}

void KisOpenGLCanvas2SchemaContractTest::displayConfigurationSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(setDisplayFilter, void (KisOpenGLCanvas2::*)(QSharedPointer<KisDisplayFilter>));
    ASSERT_OPENGL_CANVAS_SIGNATURE(notifyImageColorSpaceChanged, void (KisOpenGLCanvas2::*)(const KoColorSpace *));
    ASSERT_OPENGL_CANVAS_SIGNATURE(channelSelectionChanged, void (KisOpenGLCanvas2::*)(const QBitArray &));
    ASSERT_OPENGL_CANVAS_SIGNATURE(setDisplayConfig, void (KisOpenGLCanvas2::*)(const KisDisplayConfig &));
}

void KisOpenGLCanvas2SchemaContractTest::wrapAroundViewingSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(setWrapAroundViewingMode, void (KisOpenGLCanvas2::*)(bool));
    ASSERT_OPENGL_CANVAS_SIGNATURE(wrapAroundViewingMode, bool (KisOpenGLCanvas2::*)() const);
    ASSERT_OPENGL_CANVAS_SIGNATURE(setWrapAroundViewingModeAxis, void (KisOpenGLCanvas2::*)(WrapAroundAxis));
    ASSERT_OPENGL_CANVAS_SIGNATURE(wrapAroundViewingModeAxis, WrapAroundAxis (KisOpenGLCanvas2::*)() const);
}

void KisOpenGLCanvas2SchemaContractTest::projectionUpdateSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(finishResizingImage, void (KisOpenGLCanvas2::*)(qint32, qint32));
    ASSERT_OPENGL_CANVAS_SIGNATURE(startUpdateCanvasProjection, KisUpdateInfoSP (KisOpenGLCanvas2::*)(const QRect &));
    ASSERT_OPENGL_CANVAS_SIGNATURE(updateCanvasProjection, QRect (KisOpenGLCanvas2::*)(KisUpdateInfoSP));
    ASSERT_OPENGL_CANVAS_SIGNATURE(updateCanvasProjection,
                                   QVector<QRect> (KisOpenGLCanvas2::*)(const QVector<KisUpdateInfoSP> &));
    ASSERT_OPENGL_CANVAS_SIGNATURE(updateCanvasImage, void (KisOpenGLCanvas2::*)(const QRect &));
    ASSERT_OPENGL_CANVAS_SIGNATURE(updateCanvasDecorations, void (KisOpenGLCanvas2::*)(const QRect &));
}

void KisOpenGLCanvas2SchemaContractTest::canvasStateSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(widget, QWidget * (KisOpenGLCanvas2::*)());
    ASSERT_OPENGL_CANVAS_SIGNATURE(isBusy, bool (KisOpenGLCanvas2::*)() const);
    ASSERT_OPENGL_CANVAS_SIGNATURE(setLodResetInProgress, void (KisOpenGLCanvas2::*)(bool));
    ASSERT_OPENGL_CANVAS_SIGNATURE(currentBitDepthMode, KisOpenGLCanvas2::BitDepthMode (KisOpenGLCanvas2::*)() const);
    ASSERT_OPENGL_CANVAS_SIGNATURE(currentBitDepthUserReport, QString (KisOpenGLCanvas2::*)() const);
}

void KisOpenGLCanvas2SchemaContractTest::configurationSlotSignaturesRemainStable()
{
    ASSERT_OPENGL_CANVAS_SIGNATURE(slotConfigChanged, void (KisOpenGLCanvas2::*)());
    ASSERT_OPENGL_CANVAS_SIGNATURE(slotPixelGridModeChanged, void (KisOpenGLCanvas2::*)());
    ASSERT_OPENGL_CANVAS_SIGNATURE(slotUpdateCursorColor, void (KisOpenGLCanvas2::*)());
}

#undef ASSERT_OPENGL_CANVAS_SIGNATURE

QTEST_GUILESS_MAIN(KisOpenGLCanvas2SchemaContractTest)

#include "KisOpenGLCanvas2SchemaContractTest.moc"
