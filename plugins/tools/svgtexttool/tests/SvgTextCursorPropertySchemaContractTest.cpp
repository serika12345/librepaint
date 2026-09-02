/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SvgTextCursor.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_CURSOR_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgTextCursor::method)), signature>)
#define ASSERT_PROPERTY_INTERFACE_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgTextCursorPropertyInterface::method)), signature>)
} // namespace

class SvgTextCursorPropertySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgTextCursorIdentityAndShapeSignaturesRemainStable();
    void svgTextCursorTypeSettingSignaturesRemainStable();
    void svgTextCursorPropertyBridgeSignaturesRemainStable();
    void svgTextCursorPropertyInterfaceQuerySignaturesRemainStable();
    void svgTextCursorPropertyInterfaceMutationSignaturesRemainStable();
};

void SvgTextCursorPropertySchemaContractTest::svgTextCursorIdentityAndShapeSignaturesRemainStable()
{
    static_assert(std::is_class_v<SvgTextCursor>);
    static_assert(std::is_constructible_v<SvgTextCursor, KoCanvasBase *>);
    static_assert(std::has_virtual_destructor_v<SvgTextCursor>);
    ASSERT_CURSOR_SIGNATURE(shape, KoSvgTextShape * (SvgTextCursor::*)() const);
    ASSERT_CURSOR_SIGNATURE(setShape, void (SvgTextCursor::*)(KoSvgTextShape *));
    ASSERT_CURSOR_SIGNATURE(notifyShapeChanged, void (SvgTextCursor::*)(KoShape::ChangeType, KoShape *));
}

void SvgTextCursorPropertySchemaContractTest::svgTextCursorTypeSettingSignaturesRemainStable()
{
    ASSERT_CURSOR_SIGNATURE(cursorTypeForTypeSetting, QCursor (SvgTextCursor::*)() const);
    ASSERT_CURSOR_SIGNATURE(handleName, QString (SvgTextCursor::*)(SvgTextCursor::TypeSettingModeHandle) const);
    ASSERT_CURSOR_SIGNATURE(paintDecorations,
                            void (SvgTextCursor::*)(QPainter &, QColor, int, qreal, KisHandlePalette));
    ASSERT_CURSOR_SIGNATURE(posForTypeSettingHandleAndRect,
                            int (SvgTextCursor::*)(SvgTextCursor::TypeSettingModeHandle, QRectF));
    ASSERT_CURSOR_SIGNATURE(setDominantBaselineFromHandle,
                            bool (SvgTextCursor::*)(SvgTextCursor::TypeSettingModeHandle));
    ASSERT_CURSOR_SIGNATURE(setDrawTypeSettingHandle, void (SvgTextCursor::*)(bool));
    ASSERT_CURSOR_SIGNATURE(setTypeSettingHandleHovered, void (SvgTextCursor::*)(SvgTextCursor::TypeSettingModeHandle));
    ASSERT_CURSOR_SIGNATURE(typeSettingHandleAtPos, SvgTextCursor::TypeSettingModeHandle (SvgTextCursor::*)(QRectF));
    ASSERT_CURSOR_SIGNATURE(updateTypeSettingDecorFromShape, void (SvgTextCursor::*)());

    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().setTypeSettingHandleHovered()), void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().paintDecorations(std::declval<QPainter &>(),
                                                                                           std::declval<QColor>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>()
                                              .paintDecorations(std::declval<QPainter &>(), std::declval<QColor>(), 2)),
                                 void>);
    static_assert(
        std::is_same_v<decltype(std::declval<SvgTextCursor &>()
                                    .paintDecorations(std::declval<QPainter &>(), std::declval<QColor>(), 2, 4.0)),
                       void>);
}

void SvgTextCursorPropertySchemaContractTest::svgTextCursorPropertyBridgeSignaturesRemainStable()
{
    using CurrentTextProperties = QPair<KoSvgTextProperties, KoSvgTextProperties> (SvgTextCursor::*)() const;
    ASSERT_CURSOR_SIGNATURE(currentTextProperties, CurrentTextProperties);
    ASSERT_CURSOR_SIGNATURE(
        mergePropertiesIntoSelection,
        void (SvgTextCursor::*)(KoSvgTextProperties, QSet<KoSvgTextProperties::PropertyId>, bool, bool));
    ASSERT_CURSOR_SIGNATURE(propertiesForRange, QList<KoSvgTextProperties> (SvgTextCursor::*)() const);
    ASSERT_CURSOR_SIGNATURE(propertiesForShape, QList<KoSvgTextProperties> (SvgTextCursor::*)() const);
    ASSERT_CURSOR_SIGNATURE(registerPropertyAction, bool (SvgTextCursor::*)(QAction *, const QString &));
    ASSERT_CURSOR_SIGNATURE(sigOpenGlyphPalette, void (SvgTextCursor::*)());
    ASSERT_CURSOR_SIGNATURE(textPropertyInterface, KoSvgTextPropertiesInterface * (SvgTextCursor::*)());
    ASSERT_CURSOR_SIGNATURE(updateCursorDecoration, void (SvgTextCursor::*)(QRectF));

    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().mergePropertiesIntoSelection(
                                     std::declval<KoSvgTextProperties>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().mergePropertiesIntoSelection(
                                     std::declval<KoSvgTextProperties>(),
                                     std::declval<QSet<KoSvgTextProperties::PropertyId>>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().mergePropertiesIntoSelection(
                                     std::declval<KoSvgTextProperties>(),
                                     std::declval<QSet<KoSvgTextProperties::PropertyId>>(),
                                     true)),
                                 void>);
}

void SvgTextCursorPropertySchemaContractTest::svgTextCursorPropertyInterfaceQuerySignaturesRemainStable()
{
    static_assert(std::is_class_v<SvgTextCursorPropertyInterface>);
    static_assert(std::is_constructible_v<SvgTextCursorPropertyInterface, SvgTextCursor *>);
    static_assert(std::has_virtual_destructor_v<SvgTextCursorPropertyInterface>);
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(characterPropertiesEnabled, bool (SvgTextCursorPropertyInterface::*)());
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(getCharacterProperties,
                                        QList<KoSvgTextProperties> (SvgTextCursorPropertyInterface::*)());
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(getInheritedProperties,
                                        KoSvgTextProperties (SvgTextCursorPropertyInterface::*)());
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(getSelectedProperties,
                                        QList<KoSvgTextProperties> (SvgTextCursorPropertyInterface::*)());
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(spanSelection, bool (SvgTextCursorPropertyInterface::*)());
}

void SvgTextCursorPropertySchemaContractTest::svgTextCursorPropertyInterfaceMutationSignaturesRemainStable()
{
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(emitCharacterSelectionChange, void (SvgTextCursorPropertyInterface::*)());
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(emitSelectionChange, void (SvgTextCursorPropertyInterface::*)());
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(
        setCharacterPropertiesOnSelected,
        void (SvgTextCursorPropertyInterface::*)(KoSvgTextProperties, QSet<KoSvgTextProperties::PropertyId>));
    ASSERT_PROPERTY_INTERFACE_SIGNATURE(
        setPropertiesOnSelected,
        void (SvgTextCursorPropertyInterface::*)(KoSvgTextProperties, QSet<KoSvgTextProperties::PropertyId>));

    static_assert(
        std::is_same_v<decltype(std::declval<SvgTextCursorPropertyInterface &>().setCharacterPropertiesOnSelected(
                           std::declval<KoSvgTextProperties>())),
                       void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursorPropertyInterface &>().setPropertiesOnSelected(
                                     std::declval<KoSvgTextProperties>())),
                                 void>);
}

QTEST_APPLESS_MAIN(SvgTextCursorPropertySchemaContractTest)

#include "SvgTextCursorPropertySchemaContractTest.moc"
