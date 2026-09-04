/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgTextShape.h"
#include "text/KoSvgTextShapeOutlineHelper.h"

#include <QTest>

#include <memory>
#include <type_traits>
#include <utility>

namespace
{
using TextShape = KoSvgTextShape;
using TextShapeFactory = KoSvgTextShapeFactory;
using TextShapeMemento = KoSvgTextShapeMemento;
using TextCursorChangeListener = TextShape::TextCursorChangeListener;
using TextShapeOutlineHelper = KoSvgTextShapeOutlineHelper;
using ShapeList = QList<KoShape *>;
using ShapePathList = QList<QPainterPath>;
using PaintOrderList = QVector<TextShape::PaintOrder>;
using PropertyIdSet = QSet<KoSvgTextProperties::PropertyId>;
using StyleMap = QMap<QString, QString>;

#define ASSERT_TEXT_SHAPE_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&TextShape::method)), signature>)
#define ASSERT_TEXT_SHAPE_STATIC_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&TextShape::method)), signature>)
#define ASSERT_TEXT_SHAPE_FACTORY_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&TextShapeFactory::method)), signature>)
#define ASSERT_TEXT_CURSOR_LISTENER_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&TextCursorChangeListener::method)), signature>)
#define ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&TextShapeOutlineHelper::method)), signature>)
} // namespace

class KoSvgTextShapeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgTextShapeTypeLifetimeGeometryAndRenderingSchemaRemainStable();
    void svgTextShapeContourPathAndLayoutSignaturesRemainStable();
    void svgTextShapeContentEditingAndTransformSignaturesRemainStable();
    void svgTextShapeAppearanceStateAndNotificationSignaturesRemainStable();
    void svgTextShapePersistenceMementoAndDiagnosticSignaturesRemainStable();
    void svgTextShapeFactoryTypeLifetimeAndCreationSchemaRemainStable();
    void svgTextShapeMementoAndListenerSchemaRemainStable();
    void svgTextShapeOutlineHelperTypeLifetimeAndRenderingSchemaRemainStable();
    void svgTextShapeOutlineHelperDisplayStateSignaturesRemainStable();
    void svgTextShapeOutlineHelperContourInteractionSignaturesRemainStable();
};

void KoSvgTextShapeSchemaContractTest::svgTextShapeTypeLifetimeGeometryAndRenderingSchemaRemainStable()
{
    static_assert(std::is_constructible_v<TextShape>);
    static_assert(std::is_constructible_v<TextShape, const TextShape &>);
    static_assert(std::has_virtual_destructor_v<TextShape>);

    ASSERT_TEXT_SHAPE_STATIC_SIGNATURE(defaultPlaceholderText, const QString &(*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(cloneShape, KoShape * (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(paint, void (TextShape::*)(QPainter &) const);
    ASSERT_TEXT_SHAPE_SIGNATURE(paintStroke, void (TextShape::*)(QPainter &) const);
    ASSERT_TEXT_SHAPE_SIGNATURE(paintDebug, void (TextShape::*)(QPainter &, TextShape::DebugElements) const);
    ASSERT_TEXT_SHAPE_SIGNATURE(outline, QPainterPath (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(outlineRect, QRectF (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(boundingRect, QRectF (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(size, QSizeF (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(setSize, void (TextShape::*)(const QSizeF &));
    ASSERT_TEXT_SHAPE_SIGNATURE(textOutline, KoShape * (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(textType, TextShape::TextType (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(setResolution, void (TextShape::*)(qreal, qreal));
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeContourPathAndLayoutSignaturesRemainStable()
{
    ASSERT_TEXT_SHAPE_SIGNATURE(addShapeContours, void (TextShape::*)(ShapeList, bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(addTextPathAtEnd, void (TextShape::*)(KoShape *));
    ASSERT_TEXT_SHAPE_STATIC_SIGNATURE(generateTextAreas,
                                       ShapePathList (*)(ShapeList, ShapeList, const KoSvgTextProperties &));
    ASSERT_TEXT_SHAPE_SIGNATURE(internalShapeManager, KoShapeManager * (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(moveShapeInsideToIndex, void (TextShape::*)(KoShape *, int));
    ASSERT_TEXT_SHAPE_SIGNATURE(relayout, void (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(removeShapesFromContours, void (TextShape::*)(ShapeList, bool, bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(setShapesInside, void (TextShape::*)(ShapeList));
    ASSERT_TEXT_SHAPE_SIGNATURE(setShapesSubtract, void (TextShape::*)(ShapeList));
    ASSERT_TEXT_SHAPE_SIGNATURE(setTextPathOnRange, bool (TextShape::*)(KoShape *, int, int));
    ASSERT_TEXT_SHAPE_SIGNATURE(shapeInContours, bool (TextShape::*)(KoShape *));
    ASSERT_TEXT_SHAPE_SIGNATURE(shapesInside, ShapeList (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(shapesSubtract, ShapeList (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(textPathsAtRange, ShapeList (TextShape::*)(int, int));
    ASSERT_TEXT_SHAPE_SIGNATURE(textWrappingAreas, ShapePathList (TextShape::*)() const);

    static_assert(
        std::is_same_v<decltype(std::declval<TextShape &>().addShapeContours(std::declval<ShapeList>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<TextShape &>().removeShapesFromContours(std::declval<ShapeList>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<TextShape &>().setTextPathOnRange(std::declval<KoShape *>())), bool>);
    static_assert(std::is_same_v<decltype(std::declval<TextShape &>().textPathsAtRange()), ShapeList>);
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeContentEditingAndTransformSignaturesRemainStable()
{
    ASSERT_TEXT_SHAPE_SIGNATURE(cleanUp, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(convertCharTransformsToPreformatted, void (TextShape::*)(bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(copyRange, std::unique_ptr<TextShape> (TextShape::*)(int, int) const);
    ASSERT_TEXT_SHAPE_SIGNATURE(insertRichText, bool (TextShape::*)(int, const TextShape *, bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(insertText, bool (TextShape::*)(int, QString));
    ASSERT_TEXT_SHAPE_SIGNATURE(mergePropertiesIntoRange,
                                void (TextShape::*)(int, int, KoSvgTextProperties, PropertyIdSet));
    ASSERT_TEXT_SHAPE_SIGNATURE(plainText, QString (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(removeText, bool (TextShape::*)(int &, int &));
    ASSERT_TEXT_SHAPE_SIGNATURE(removeTransformsFromRange, void (TextShape::*)(int, int));
    ASSERT_TEXT_SHAPE_SIGNATURE(setCharacterTransformsFromLayout, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(setCharacterTransformsOnRange,
                                bool (TextShape::*)(int, int, QVector<QPointF>, QVector<qreal>, bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(setPropertiesAtPos, void (TextShape::*)(int, KoSvgTextProperties));
    ASSERT_TEXT_SHAPE_SIGNATURE(textProperties, KoSvgTextProperties (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(
        underlines,
        QPainterPath (TextShape::*)(int, int, KoSvgText::TextDecorations, KoSvgText::TextDecorationStyle, qreal, bool));

    static_assert(std::is_same_v<decltype(std::declval<TextShape &>().convertCharTransformsToPreformatted()), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<TextShape &>().insertRichText(0, std::declval<const TextShape *>())),
                       bool>);
    static_assert(std::is_same_v<decltype(std::declval<TextShape &>()
                                              .mergePropertiesIntoRange(0, 0, std::declval<KoSvgTextProperties>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<TextShape &>().setCharacterTransformsOnRange(
                                     0,
                                     0,
                                     std::declval<QVector<QPointF>>(),
                                     std::declval<QVector<qreal>>())),
                                 bool>);
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeAppearanceStateAndNotificationSignaturesRemainStable()
{
    ASSERT_TEXT_SHAPE_SIGNATURE(background, QSharedPointer<KoShapeBackground> (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(direction, KoSvgText::Direction (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(endBulkAction, QRectF (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(fontMatchingDisabled, bool (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(notifyCursorPosChanged, void (TextShape::*)(int, int));
    ASSERT_TEXT_SHAPE_SIGNATURE(notifyMarkupChanged, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(paintOrder, PaintOrderList (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(relayoutIsBlocked, bool (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(setBackground, void (TextShape::*)(QSharedPointer<KoShapeBackground>));
    ASSERT_TEXT_SHAPE_SIGNATURE(setFontMatchingDisabled, void (TextShape::*)(bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(setPaintOrder, void (TextShape::*)(TextShape::PaintOrder, TextShape::PaintOrder));
    ASSERT_TEXT_SHAPE_SIGNATURE(setRelayoutBlocked, void (TextShape::*)(bool));
    ASSERT_TEXT_SHAPE_SIGNATURE(setStroke, void (TextShape::*)(KoShapeStrokeModelSP));
    ASSERT_TEXT_SHAPE_SIGNATURE(singleNode, bool (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(startBulkAction, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(stroke, KoShapeStrokeModelSP (TextShape::*)() const);
    ASSERT_TEXT_SHAPE_SIGNATURE(writingMode, KoSvgText::WritingMode (TextShape::*)() const);
}

void KoSvgTextShapeSchemaContractTest::svgTextShapePersistenceMementoAndDiagnosticSignaturesRemainStable()
{
    ASSERT_TEXT_SHAPE_SIGNATURE(debugParsing, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(enterNodeSubtree, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(getMemento, KoSvgTextShapeMementoSP (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(leaveNodeSubtree, void (TextShape::*)());
    ASSERT_TEXT_SHAPE_SIGNATURE(saveHtml, bool (TextShape::*)(HtmlSavingContext &));
    ASSERT_TEXT_SHAPE_SIGNATURE(saveSvg, bool (TextShape::*)(SvgSavingContext &));
    ASSERT_TEXT_SHAPE_SIGNATURE(setMemento, void (TextShape::*)(KoSvgTextShapeMementoSP));
    ASSERT_TEXT_SHAPE_SIGNATURE(setMemento, void (TextShape::*)(KoSvgTextShapeMementoSP, int, int));
    ASSERT_TEXT_SHAPE_SIGNATURE(shapeTypeSpecificStyles, StyleMap (TextShape::*)(SvgSavingContext &) const);
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeFactoryTypeLifetimeAndCreationSchemaRemainStable()
{
    static_assert(std::is_class_v<TextShapeFactory>);
    static_assert(std::is_base_of_v<KoShapeFactoryBase, TextShapeFactory>);
    static_assert(std::is_default_constructible_v<TextShapeFactory>);
    static_assert(std::has_virtual_destructor_v<TextShapeFactory>);

    ASSERT_TEXT_SHAPE_FACTORY_SIGNATURE(createDefaultShape,
                                        KoShape * (TextShapeFactory::*)(KoDocumentResourceManager *) const);
    ASSERT_TEXT_SHAPE_FACTORY_SIGNATURE(
        createShape,
        KoShape * (TextShapeFactory::*)(const KoProperties *, KoDocumentResourceManager *) const);
    ASSERT_TEXT_SHAPE_FACTORY_SIGNATURE(supports,
                                        bool (TextShapeFactory::*)(const QDomElement &, KoShapeLoadingContext &) const);

    static_assert(std::is_same_v<decltype(std::declval<const TextShapeFactory &>().createDefaultShape()), KoShape *>);
    static_assert(std::is_same_v<decltype(std::declval<const TextShapeFactory &>().createShape(
                                     std::declval<const KoProperties *>())),
                                 KoShape *>);
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeMementoAndListenerSchemaRemainStable()
{
    static_assert(std::is_default_constructible_v<TextShapeMemento>);
    static_assert(std::has_virtual_destructor_v<TextShapeMemento>);

    static_assert(std::is_class_v<TextCursorChangeListener>);
    static_assert(std::is_base_of_v<KoShape::ShapeChangeListener, TextCursorChangeListener>);
    static_assert(std::is_abstract_v<TextCursorChangeListener>);
    static_assert(std::has_virtual_destructor_v<TextCursorChangeListener>);
    ASSERT_TEXT_CURSOR_LISTENER_SIGNATURE(notifyCursorPosChanged, void (TextCursorChangeListener::*)(int, int));
    ASSERT_TEXT_CURSOR_LISTENER_SIGNATURE(notifyMarkupChanged, void (TextCursorChangeListener::*)());
    ASSERT_TEXT_CURSOR_LISTENER_SIGNATURE(notifyShapeChanged,
                                          void (TextCursorChangeListener::*)(KoShape::ChangeType, KoShape *));
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeOutlineHelperTypeLifetimeAndRenderingSchemaRemainStable()
{
    static_assert(std::is_class_v<TextShapeOutlineHelper>);
    static_assert(std::is_constructible_v<TextShapeOutlineHelper, KoCanvasBase *>);
    static_assert(std::is_destructible_v<TextShapeOutlineHelper>);
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(paint, void (TextShapeOutlineHelper::*)(QPainter *, const KoViewConverter &));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(decorationRect, QRectF (TextShapeOutlineHelper::*)());
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeOutlineHelperDisplayStateSignaturesRemainStable()
{
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(drawBoundingRect, bool (TextShapeOutlineHelper::*)() const);
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(drawShapeOutlines, bool (TextShapeOutlineHelper::*)() const);
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(drawTextWrappingArea, bool (TextShapeOutlineHelper::*)() const);
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(setDecorationThickness, void (TextShapeOutlineHelper::*)(int));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(setDrawBoundingRect, void (TextShapeOutlineHelper::*)(bool));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(setDrawShapeOutlines, void (TextShapeOutlineHelper::*)(bool));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(setDrawTextWrappingArea, void (TextShapeOutlineHelper::*)(bool));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(setHandleRadius, void (TextShapeOutlineHelper::*)(int));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(setTextAreasHovered, void (TextShapeOutlineHelper::*)(bool));
}

void KoSvgTextShapeSchemaContractTest::svgTextShapeOutlineHelperContourInteractionSignaturesRemainStable()
{
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(contourModeButtonHovered,
                                         KoSvgTextShape * (TextShapeOutlineHelper::*)(const QPointF &));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(toggleTextContourMode, void (TextShapeOutlineHelper::*)(KoSvgTextShape *));
    ASSERT_TEXT_OUTLINE_HELPER_SIGNATURE(updateTextContourMode, bool (TextShapeOutlineHelper::*)());
}

QTEST_APPLESS_MAIN(KoSvgTextShapeSchemaContractTest)

#include "KoSvgTextShapeSchemaContractTest.moc"
