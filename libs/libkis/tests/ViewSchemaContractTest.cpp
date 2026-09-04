/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Notifier.h>
#include <QTest>
#include <Scratchpad.h>
#include <View.h>
#include <Window.h>
#include <type_traits>
#include <utility>

#define ASSERT_VIEW_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&View::method)), signature>)

#define ASSERT_SCRATCHPAD_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Scratchpad::method)), signature>)

#define ASSERT_WINDOW_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Window::method)), signature>)

#define ASSERT_NOTIFIER_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Notifier::method)), signature>)

class ViewSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void viewOwnershipLifetimeAndEqualitySchemaRemainsStable();
    void viewBrushGeometrySignaturesRemainStable();
    void viewDepositAndCompositeSignaturesRemainStable();
    void viewInputModePolicySignaturesRemainStable();
    void viewHdrPresentationSignaturesRemainStable();
    void viewDocumentAndVisibilitySignaturesRemainStable();
    void viewResourceSelectionSignaturesRemainStable();
    void viewColorStateAndNotificationSignaturesRemainStable();
    void viewCoordinateTransformSignaturesRemainStable();
    void viewInteractionNotificationSignaturesRemainStable();
    void scratchpadTypeLifetimeAndModeSchemaRemainStable();
    void scratchpadFillSignaturesRemainStable();
    void scratchpadZoomAndScaleSignaturesRemainStable();
    void scratchpadPanImageAndBoundsSignaturesRemainStable();
    void scratchpadNotificationSignaturesRemainStable();
    void windowTypeLifetimeAndEqualitySchemaRemainStable();
    void windowViewAndActivationSignaturesRemainStable();
    void windowActionAndNotificationSignaturesRemainStable();
    void notifierTypeLifetimeAndActivationSchemaRemainStable();
    void notifierApplicationAndUiNotificationSignaturesRemainStable();
};

void ViewSchemaContractTest::viewOwnershipLifetimeAndEqualitySchemaRemainsStable()
{
    static_assert(std::is_class_v<View>);
    static_assert(std::is_base_of_v<QObject, View>);
    static_assert(std::is_constructible_v<View, KisView *>);
    static_assert(std::is_constructible_v<View, KisView *, QObject *>);
    static_assert(std::has_virtual_destructor_v<View>);
    ASSERT_VIEW_SIGNATURE(operator==, bool (View::*)(const View &) const);
    ASSERT_VIEW_SIGNATURE(operator!=, bool (View::*)(const View &) const);
}

void ViewSchemaContractTest::viewBrushGeometrySignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(brushSize, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBrushSize, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(brushRotation, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBrushRotation, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(brushFade, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBrushFade, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(patternSize, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setPatternSize, void (View::*)(qreal));
}

void ViewSchemaContractTest::viewDepositAndCompositeSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(paintingFlow, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setPaintingFlow, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(paintingOpacity, qreal (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setPaintingOpacity, void (View::*)(qreal));
    ASSERT_VIEW_SIGNATURE(currentBlendingMode, QString (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setCurrentBlendingMode, void (View::*)(const QString &));
}

void ViewSchemaContractTest::viewInputModePolicySignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(disablePressure, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setDisablePressure, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(eraserMode, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setEraserMode, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(globalAlphaLock, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setGlobalAlphaLock, void (View::*)(bool));
}

void ViewSchemaContractTest::viewHdrPresentationSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(HDRExposure, float (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setHDRExposure, void (View::*)(float));
    ASSERT_VIEW_SIGNATURE(HDRGamma, float (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setHDRGamma, void (View::*)(float));
}

void ViewSchemaContractTest::viewDocumentAndVisibilitySignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(window, Window * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(document, Document * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setDocument, void (View::*)(Document *));
    ASSERT_VIEW_SIGNATURE(visible, bool (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setVisible, void (View::*)());
    ASSERT_VIEW_SIGNATURE(canvas, Canvas * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(selectedNodes, QList<Node *> (View::*)() const);
}

void ViewSchemaContractTest::viewResourceSelectionSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(activateResource, void (View::*)(Resource *));
    ASSERT_VIEW_SIGNATURE(currentBrushPreset, Resource * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setCurrentBrushPreset, void (View::*)(Resource *));
    ASSERT_VIEW_SIGNATURE(currentPattern, Resource * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setCurrentPattern, void (View::*)(Resource *));
    ASSERT_VIEW_SIGNATURE(currentGradient, Resource * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setCurrentGradient, void (View::*)(Resource *));
}

void ViewSchemaContractTest::viewColorStateAndNotificationSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(foregroundColor, ManagedColor * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setForeGroundColor, void (View::*)(ManagedColor *));
    ASSERT_VIEW_SIGNATURE(backgroundColor, ManagedColor * (View::*)() const);
    ASSERT_VIEW_SIGNATURE(setBackGroundColor, void (View::*)(ManagedColor *));
    ASSERT_VIEW_SIGNATURE(foregroundColorChanged, void (View::*)());
    ASSERT_VIEW_SIGNATURE(backgroundColorChanged, void (View::*)());
}

void ViewSchemaContractTest::viewCoordinateTransformSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(flakeToDocumentTransform, QTransform (View::*)() const);
    ASSERT_VIEW_SIGNATURE(flakeToCanvasTransform, QTransform (View::*)() const);
    ASSERT_VIEW_SIGNATURE(flakeToImageTransform, QTransform (View::*)() const);
}

void ViewSchemaContractTest::viewInteractionNotificationSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(currentBrushPresetChanged, void (View::*)());
    ASSERT_VIEW_SIGNATURE(currentToolChanged, void (View::*)(const QString &));
    ASSERT_VIEW_SIGNATURE(showFloatingMessage, void (View::*)(const QString &, const QIcon &, int, int));
}

void ViewSchemaContractTest::scratchpadTypeLifetimeAndModeSchemaRemainStable()
{
    static_assert(std::is_class_v<Scratchpad>);
    static_assert(std::is_base_of_v<QWidget, Scratchpad>);
    static_assert(std::is_constructible_v<Scratchpad, View *, const QColor &>);
    static_assert(std::is_constructible_v<Scratchpad, View *, const QColor &, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Scratchpad>);
    ASSERT_SCRATCHPAD_SIGNATURE(setFillColor, void (Scratchpad::*)(QColor));
    ASSERT_SCRATCHPAD_SIGNATURE(setMode, void (Scratchpad::*)(QString));
    ASSERT_SCRATCHPAD_SIGNATURE(setModeManually, void (Scratchpad::*)(bool));
}

void ViewSchemaContractTest::scratchpadFillSignaturesRemainStable()
{
    ASSERT_SCRATCHPAD_SIGNATURE(clear, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(fillDefault, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(fillBackground, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(fillForeground, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(fillTransparent, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(fillDocument, void (Scratchpad::*)(bool));
    ASSERT_SCRATCHPAD_SIGNATURE(fillLayer, void (Scratchpad::*)(bool));
    ASSERT_SCRATCHPAD_SIGNATURE(
        fillGradient,
        void (Scratchpad::*)(const QPoint &, const QPoint &, const QString &, const QString &, bool, bool));
    ASSERT_SCRATCHPAD_SIGNATURE(fillPattern, void (Scratchpad::*)(QTransform));

    static_assert(std::is_same_v<decltype(std::declval<Scratchpad &>().fillDocument()), void>);
    static_assert(std::is_same_v<decltype(std::declval<Scratchpad &>().fillLayer()), void>);
    static_assert(std::is_same_v<decltype(std::declval<Scratchpad &>().fillGradient()), void>);
    static_assert(std::is_same_v<decltype(std::declval<Scratchpad &>().fillPattern()), void>);
}

void ViewSchemaContractTest::scratchpadZoomAndScaleSignaturesRemainStable()
{
    ASSERT_SCRATCHPAD_SIGNATURE(linkCanvasZoom, void (Scratchpad::*)(bool));
    ASSERT_SCRATCHPAD_SIGNATURE(canvasZoomLink, bool (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(setCanvasZoomLink, void (Scratchpad::*)(bool));
    ASSERT_SCRATCHPAD_SIGNATURE(scale, qreal (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(setScale, bool (Scratchpad::*)(qreal) const);
    ASSERT_SCRATCHPAD_SIGNATURE(scaleToFit, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(scaleReset, void (Scratchpad::*)());
}

void ViewSchemaContractTest::scratchpadPanImageAndBoundsSignaturesRemainStable()
{
    ASSERT_SCRATCHPAD_SIGNATURE(panTo, void (Scratchpad::*)(qint32, qint32));
    ASSERT_SCRATCHPAD_SIGNATURE(panCenter, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(loadScratchpadImage, void (Scratchpad::*)(QImage));
    ASSERT_SCRATCHPAD_SIGNATURE(copyScratchpadImageData, QImage (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(viewportBounds, QRect (Scratchpad::*)() const);
    ASSERT_SCRATCHPAD_SIGNATURE(contentBounds, QRect (Scratchpad::*)() const);
}

void ViewSchemaContractTest::scratchpadNotificationSignaturesRemainStable()
{
    ASSERT_SCRATCHPAD_SIGNATURE(scaleChanged, void (Scratchpad::*)(qreal));
    ASSERT_SCRATCHPAD_SIGNATURE(contentChanged, void (Scratchpad::*)());
    ASSERT_SCRATCHPAD_SIGNATURE(viewportChanged, void (Scratchpad::*)(const QRect));
}

void ViewSchemaContractTest::windowTypeLifetimeAndEqualitySchemaRemainStable()
{
    static_assert(std::is_class_v<Window>);
    static_assert(std::is_base_of_v<QObject, Window>);
    static_assert(std::is_constructible_v<Window, KisMainWindow *>);
    static_assert(std::is_constructible_v<Window, KisMainWindow *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Window>);
    ASSERT_WINDOW_SIGNATURE(operator==, bool (Window::*)(const Window &) const);
    ASSERT_WINDOW_SIGNATURE(operator!=, bool (Window::*)(const Window &) const);
}

void ViewSchemaContractTest::windowViewAndActivationSignaturesRemainStable()
{
    ASSERT_WINDOW_SIGNATURE(qwindow, QMainWindow * (Window::*)() const);
    ASSERT_WINDOW_SIGNATURE(dockers, QList<QDockWidget *> (Window::*)() const);
    ASSERT_WINDOW_SIGNATURE(views, QList<View *> (Window::*)() const);
    ASSERT_WINDOW_SIGNATURE(addView, View * (Window::*)(Document *));
    ASSERT_WINDOW_SIGNATURE(showView, void (Window::*)(View *));
    ASSERT_WINDOW_SIGNATURE(activeView, View * (Window::*)() const);
    ASSERT_WINDOW_SIGNATURE(activate, void (Window::*)());
    ASSERT_WINDOW_SIGNATURE(close, void (Window::*)());
}

void ViewSchemaContractTest::windowActionAndNotificationSignaturesRemainStable()
{
    ASSERT_WINDOW_SIGNATURE(createAction, QAction * (Window::*)(const QString &, const QString &, const QString &));
    ASSERT_WINDOW_SIGNATURE(windowClosed, void (Window::*)());
    ASSERT_WINDOW_SIGNATURE(themeChanged, void (Window::*)());
    ASSERT_WINDOW_SIGNATURE(activeViewChanged, void (Window::*)());

    static_assert(
        std::is_same_v<decltype(std::declval<Window &>().createAction(std::declval<const QString &>())), QAction *>);
    static_assert(std::is_same_v<decltype(std::declval<Window &>().createAction(std::declval<const QString &>(),
                                                                                std::declval<const QString &>())),
                                 QAction *>);
}

void ViewSchemaContractTest::notifierTypeLifetimeAndActivationSchemaRemainStable()
{
    static_assert(std::is_class_v<Notifier>);
    static_assert(std::is_base_of_v<QObject, Notifier>);
    static_assert(std::is_default_constructible_v<Notifier>);
    static_assert(std::is_constructible_v<Notifier, QObject *>);
    static_assert(std::has_virtual_destructor_v<Notifier>);
    ASSERT_NOTIFIER_SIGNATURE(active, bool (Notifier::*)() const);
    ASSERT_NOTIFIER_SIGNATURE(setActive, void (Notifier::*)(bool));
}

void ViewSchemaContractTest::notifierApplicationAndUiNotificationSignaturesRemainStable()
{
    ASSERT_NOTIFIER_SIGNATURE(applicationClosing, void (Notifier::*)());
    ASSERT_NOTIFIER_SIGNATURE(imageCreated, void (Notifier::*)(Document *));
    ASSERT_NOTIFIER_SIGNATURE(imageSaved, void (Notifier::*)(const QString &));
    ASSERT_NOTIFIER_SIGNATURE(imageClosed, void (Notifier::*)(const QString &));
    ASSERT_NOTIFIER_SIGNATURE(viewCreated, void (Notifier::*)(View *));
    ASSERT_NOTIFIER_SIGNATURE(viewClosed, void (Notifier::*)(View *));
    ASSERT_NOTIFIER_SIGNATURE(windowIsBeingCreated, void (Notifier::*)(Window *));
    ASSERT_NOTIFIER_SIGNATURE(windowCreated, void (Notifier::*)());
    ASSERT_NOTIFIER_SIGNATURE(configurationChanged, void (Notifier::*)());
}

QTEST_GUILESS_MAIN(ViewSchemaContractTest)
#include "ViewSchemaContractTest.moc"
