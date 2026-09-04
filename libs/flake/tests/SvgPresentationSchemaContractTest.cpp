/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>

#include <svg/KoShapePainter.h>
#include <svg/SvgGraphicContext.h>
#include <svg/SvgStyleWriter.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_SVG_GRAPHICS_CONTEXT_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgGraphicsContext::method)), signature>)

#define ASSERT_SHAPE_PAINTER_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapePainter::method)), signature>)

#define ASSERT_SVG_STYLE_WRITER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgStyleWriter::method)), signature>)
} // namespace

class SvgPresentationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgGraphicsContextConstructionAndInheritedFillSchemaRemainStable();
    void shapePainterTypeLifetimeAndConstructionSchemaRemainStable();
    void shapePainterConfigurationAndRenderingSignaturesRemainStable();
    void svgStyleWriterTypeAndShapePersistenceSchemaRemainStable();
    void svgStyleWriterPaintStyleSignaturesRemainStable();
};

void SvgPresentationSchemaContractTest::svgGraphicsContextConstructionAndInheritedFillSchemaRemainStable()
{
    static_assert(std::is_default_constructible_v<SvgGraphicsContext>);
    static_assert(std::is_copy_constructible_v<SvgGraphicsContext>);
    ASSERT_SVG_GRAPHICS_CONTEXT_SIGNATURE(workaroundClearInheritedFillProperties, void (SvgGraphicsContext::*)());
}

void SvgPresentationSchemaContractTest::shapePainterTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapePainter>);
    static_assert(std::is_default_constructible_v<KoShapePainter>);
    static_assert(std::is_destructible_v<KoShapePainter>);
}

void SvgPresentationSchemaContractTest::shapePainterConfigurationAndRenderingSignaturesRemainStable()
{
    ASSERT_SHAPE_PAINTER_SIGNATURE(contentRect, QRectF (KoShapePainter::*)() const);
    ASSERT_SHAPE_PAINTER_SIGNATURE(internalShapeManager, KoShapeManager * (KoShapePainter::*)() const);
    ASSERT_SHAPE_PAINTER_SIGNATURE(paint, void (KoShapePainter::*)(QImage &));
    ASSERT_SHAPE_PAINTER_SIGNATURE(paint, void (KoShapePainter::*)(QPainter &));
    ASSERT_SHAPE_PAINTER_SIGNATURE(paint, void (KoShapePainter::*)(QPainter &, const QRect &, const QRectF &));
    ASSERT_SHAPE_PAINTER_SIGNATURE(setShapes, void (KoShapePainter::*)(const QList<KoShape *> &));
    ASSERT_SHAPE_PAINTER_SIGNATURE(setUpdateFunction, void (KoShapePainter::*)(std::function<void(const QRectF &)>));
}

void SvgPresentationSchemaContractTest::svgStyleWriterTypeAndShapePersistenceSchemaRemainStable()
{
    static_assert(std::is_class_v<SvgStyleWriter>);
    ASSERT_SVG_STYLE_WRITER_SIGNATURE(embedShape, QString (*)(const KoShape *, SvgSavingContext &));
    ASSERT_SVG_STYLE_WRITER_SIGNATURE(saveMetadata, void (*)(const KoShape *, SvgSavingContext &));
    ASSERT_SVG_STYLE_WRITER_SIGNATURE(saveSvgStyle, void (*)(KoShape *, SvgSavingContext &));
}

void SvgPresentationSchemaContractTest::svgStyleWriterPaintStyleSignaturesRemainStable()
{
    ASSERT_SVG_STYLE_WRITER_SIGNATURE(
        saveSvgBasicStyle,
        void (*)(bool, qreal, QVector<KoShape::PaintOrder>, bool, SvgSavingContext &, bool));
    ASSERT_SVG_STYLE_WRITER_SIGNATURE(
        saveSvgFill,
        void (*)(QSharedPointer<KoShapeBackground>, bool, QRectF, QSizeF, QTransform, SvgSavingContext &));
    ASSERT_SVG_STYLE_WRITER_SIGNATURE(saveSvgStroke, void (*)(KoShapeStrokeModelSP, SvgSavingContext &));

    static_assert(
        std::is_same_v<decltype(SvgStyleWriter::saveSvgBasicStyle(std::declval<bool>(),
                                                                  std::declval<qreal>(),
                                                                  std::declval<QVector<KoShape::PaintOrder>>(),
                                                                  std::declval<bool>(),
                                                                  std::declval<SvgSavingContext &>())),
                       void>);
}

QTEST_APPLESS_MAIN(SvgPresentationSchemaContractTest)

#include "SvgPresentationSchemaContractTest.moc"
