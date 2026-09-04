/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisGradientColorEditor.h>
#include <KisStopGradientEditor.h>
#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGradientColorEditor::method)), signature>)

#define ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisStopGradientEditor::method)), signature>)

#define ASSERT_GRADIENT_WIDGET_UTILITY_SIGNATURE(method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGradientWidgetsUtils::method)), signature>)

class KisGradientColorEditorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientColorEditorTypeAndLifetimeSchemaRemainsStable();
    void gradientColorEditorPositionSchemaRemainsStable();
    void gradientColorEditorColorSchemaRemainsStable();
    void gradientColorEditorTransparencySchemaRemainsStable();
    void gradientColorEditorOpacityAndGeometrySchemaRemainsStable();
    void stopGradientEditorTypeLifetimeAndSortSchemaRemainStable();
    void stopGradientEditorResourceSignaturesRemainStable();
    void stopGradientEditorPresentationAndNotificationSchemaRemainStable();
    void gradientWidgetUtilityTypesAndConversionSchemaRemainStable();
    void gradientWidgetUtilityPaintingSignaturesRemainStable();
};

void KisGradientColorEditorSchemaContractTest::gradientColorEditorTypeAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisGradientColorEditor>);
    static_assert(std::is_constructible_v<KisGradientColorEditor, QWidget *>);
    static_assert(std::is_default_constructible_v<KisGradientColorEditor>);
    static_assert(std::is_copy_constructible_v<KisGradientColorEditor>);
    static_assert(std::has_virtual_destructor_v<KisGradientColorEditor>);
}

void KisGradientColorEditorSchemaContractTest::gradientColorEditorPositionSchemaRemainsStable()
{
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(position, qreal (KisGradientColorEditor::*)() const);
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setPosition, void (KisGradientColorEditor::*)(qreal));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(positionChanged, void (KisGradientColorEditor::*)(qreal));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setUsePositionSlider, void (KisGradientColorEditor::*)(bool));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setPositionSliderEnabled, void (KisGradientColorEditor::*)(bool));
}

void KisGradientColorEditorSchemaContractTest::gradientColorEditorColorSchemaRemainsStable()
{
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(color, KoColor (KisGradientColorEditor::*)() const);
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setColor, void (KisGradientColorEditor::*)(KoColor));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(colorChanged, void (KisGradientColorEditor::*)(KoColor));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(colorType,
                                           KisGradientWidgetsUtils::ColorType (KisGradientColorEditor::*)() const);
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setColorType,
                                           void (KisGradientColorEditor::*)(KisGradientWidgetsUtils::ColorType));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(colorTypeChanged,
                                           void (KisGradientColorEditor::*)(KisGradientWidgetsUtils::ColorType));
}

void KisGradientColorEditorSchemaContractTest::gradientColorEditorTransparencySchemaRemainsStable()
{
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(transparent, bool (KisGradientColorEditor::*)() const);
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setTransparent, void (KisGradientColorEditor::*)(bool));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(transparentToggled, void (KisGradientColorEditor::*)(bool));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setUseTransParentCheckBox, void (KisGradientColorEditor::*)(bool));
}

void KisGradientColorEditorSchemaContractTest::gradientColorEditorOpacityAndGeometrySchemaRemainsStable()
{
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(opacity, qreal (KisGradientColorEditor::*)() const);
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(setOpacity, void (KisGradientColorEditor::*)(qreal));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(opacityChanged, void (KisGradientColorEditor::*)(qreal));
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(sizeHint, QSize (KisGradientColorEditor::*)() const);
    ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(minimumSizeHint, QSize (KisGradientColorEditor::*)() const);
}

void KisGradientColorEditorSchemaContractTest::stopGradientEditorTypeLifetimeAndSortSchemaRemainStable()
{
    using Editor = KisStopGradientEditor;

    static_assert(std::is_class_v<Editor>);
    static_assert(std::is_base_of_v<QWidget, Editor>);
    static_assert(std::is_base_of_v<Ui::KisWdgStopGradientEditor, Editor>);
    static_assert(std::is_enum_v<Editor::SortFlag>);
    static_assert(Editor::SORT_ASCENDING == 1);
    static_assert(Editor::EVEN_DISTRIBUTION == 2);
    static_assert(std::is_same_v<Editor::SortFlags, QFlags<Editor::SortFlag>>);
    static_assert(std::is_constructible_v<Editor, QWidget *>);
    static_assert(std::is_constructible_v<Editor,
                                          KoStopGradientSP,
                                          QWidget *,
                                          const char *,
                                          const QString &,
                                          KoCanvasResourcesInterfaceSP>);
}

void KisGradientColorEditorSchemaContractTest::stopGradientEditorResourceSignaturesRemainStable()
{
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(setGradient, void (KisStopGradientEditor::*)(KoStopGradientSP));
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(setCanvasResourcesInterface,
                                          void (KisStopGradientEditor::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(canvasResourcesInterface,
                                          KoCanvasResourcesInterfaceSP (KisStopGradientEditor::*)() const);
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(currentActiveStopColor,
                                          boost::optional<KoColor> (KisStopGradientEditor::*)() const);
}

void KisGradientColorEditorSchemaContractTest::stopGradientEditorPresentationAndNotificationSchemaRemainStable()
{
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(setCompactMode, void (KisStopGradientEditor::*)(bool));
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(notifyGlobalColorChanged, void (KisStopGradientEditor::*)(const KoColor &));
    ASSERT_STOP_GRADIENT_EDITOR_SIGNATURE(sigGradientChanged, void (KisStopGradientEditor::*)());
}

void KisGradientColorEditorSchemaContractTest::gradientWidgetUtilityTypesAndConversionSchemaRemainStable()
{
    using namespace KisGradientWidgetsUtils;

    static_assert(std::is_enum_v<ColorType>);
    static_assert(None == 0);
    static_assert(Foreground == 1);
    static_assert(Background == 2);
    static_assert(Custom == 3);
    static_assert(std::is_class_v<StopHandleColor>);
    static_assert(std::is_same_v<decltype(&StopHandleColor::type), ColorType StopHandleColor::*>);
    static_assert(std::is_same_v<decltype(&StopHandleColor::color), QColor StopHandleColor::*>);
    ASSERT_GRADIENT_WIDGET_UTILITY_SIGNATURE(segmentEndPointTypeToColorType,
                                             ColorType (*)(KoGradientSegmentEndpointType));
    ASSERT_GRADIENT_WIDGET_UTILITY_SIGNATURE(colorTypeToSegmentEndPointType,
                                             KoGradientSegmentEndpointType (*)(ColorType, bool));
    static_assert(std::is_same_v<decltype(colorTypeToSegmentEndPointType(None)), KoGradientSegmentEndpointType>);
}

void KisGradientColorEditorSchemaContractTest::gradientWidgetUtilityPaintingSignaturesRemainStable()
{
    using namespace KisGradientWidgetsUtils;

    ASSERT_GRADIENT_WIDGET_UTILITY_SIGNATURE(paintGradientBox,
                                             void (*)(QPainter &, KoAbstractGradientSP, const QRectF &));
    ASSERT_GRADIENT_WIDGET_UTILITY_SIGNATURE(
        paintMidPointHandle,
        void (*)(QPainter &, const QPointF &, qreal, bool, bool, bool, const QColor &, const QColor &, const QColor &));
    ASSERT_GRADIENT_WIDGET_UTILITY_SIGNATURE(paintStopHandle,
                                             void (*)(QPainter &,
                                                      const QPointF &,
                                                      const QSizeF &,
                                                      bool,
                                                      bool,
                                                      bool,
                                                      const QColor &,
                                                      const StopHandleColor &,
                                                      const StopHandleColor &));
    static_assert(std::is_same_v<decltype(paintStopHandle(std::declval<QPainter &>(),
                                                          std::declval<const QPointF &>(),
                                                          std::declval<const QSizeF &>(),
                                                          bool{},
                                                          bool{},
                                                          bool{},
                                                          std::declval<const QColor &>(),
                                                          std::declval<const StopHandleColor &>())),
                                 void>);
}

QTEST_GUILESS_MAIN(KisGradientColorEditorSchemaContractTest)
#include "KisGradientColorEditorSchemaContractTest.moc"
