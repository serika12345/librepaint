/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisGradientColorEditor.h>
#include <QTest>

#include <type_traits>

#define ASSERT_GRADIENT_COLOR_EDITOR_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGradientColorEditor::method)), signature>)

class KisGradientColorEditorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientColorEditorTypeAndLifetimeSchemaRemainsStable();
    void gradientColorEditorPositionSchemaRemainsStable();
    void gradientColorEditorColorSchemaRemainsStable();
    void gradientColorEditorTransparencySchemaRemainsStable();
    void gradientColorEditorOpacityAndGeometrySchemaRemainsStable();
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

QTEST_GUILESS_MAIN(KisGradientColorEditorSchemaContractTest)
#include "KisGradientColorEditorSchemaContractTest.moc"
