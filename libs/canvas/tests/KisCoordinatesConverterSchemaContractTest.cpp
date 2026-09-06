/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoViewTransformStillPoint.h>
#include <kis_coordinates_converter.h>

#include <QTest>

#include <type_traits>

#define ASSERT_COORDINATE_CONVERSION_SIGNATURE(member)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<PointConversion>(&KisCoordinatesConverter::member<QPointF>)),    \
                                 PointConversion>)

class KisCoordinatesConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void coordinatesConverterIdentityAndCanvasStateSignaturesRemainStable();
    void coordinatesConverterZoomRotationAndMirrorSignaturesRemainStable();
    void coordinatesConverterTransformAndStillPointSignaturesRemainStable();
    void coordinatesConverterImageGeometrySignaturesRemainStable();
    void coordinatesConverterCheckerProjectionSignaturesRemainStable();
    void coordinateTraitsMappingRemainsStable();
    void imageViewportAndDocumentConversionSignaturesRemainStable();
    void documentFlakeAndWidgetConversionSignaturesRemainStable();
    void widgetAndViewportConversionSignaturesRemainStable();
    void imageAndWidgetConversionSignaturesRemainStable();
};

void KisCoordinatesConverterSchemaContractTest::coordinatesConverterIdentityAndCanvasStateSignaturesRemainStable()
{
    using Converter = KisCoordinatesConverter;

    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_base_of_v<KoZoomHandler, Converter>);
    static_assert(std::is_constructible_v<Converter>);
    static_assert(std::is_destructible_v<Converter>);
    static_assert(std::is_same_v<decltype(&Converter::getCanvasWidgetSize), QSizeF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::viewportDevicePixelSize), QSize (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::setCanvasWidgetSize), void (Converter::*)(QSizeF)>);
    static_assert(
        std::is_same_v<decltype(&Converter::setCanvasWidgetSizeKeepZoom), void (Converter::*)(const QSizeF &)>);
    static_assert(std::is_same_v<decltype(&Converter::setDevicePixelRatio), void (Converter::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Converter::devicePixelRatio), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::setDocumentOffset), void (Converter::*)(const QPointF &)>);
    static_assert(std::is_same_v<decltype(&Converter::documentOffset), QPoint (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::documentOffsetF), QPointF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::setVastScrolling), void (Converter::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Converter::vastScrolling), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::setExtraReferencesBounds), void (Converter::*)(const QRect &)>);
    static_assert(std::is_same_v<decltype(&Converter::setImage), void (Converter::*)(KisImageWSP)>);
    static_assert(
        std::is_same_v<decltype(&Converter::setImageBounds), void (Converter::*)(const QRect &, QPointF, QPointF)>);
    static_assert(std::is_same_v<decltype(&Converter::setImageResolution), void (Converter::*)(qreal, qreal)>);
}

void KisCoordinatesConverterSchemaContractTest::coordinatesConverterZoomRotationAndMirrorSignaturesRemainStable()
{
    using Converter = KisCoordinatesConverter;
    using StillPoint = std::optional<KoViewTransformStillPoint>;

    static_assert(std::is_same_v<decltype(&Converter::beginRotation), void (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::endRotation), void (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::enableNatureGestureFlag), void (Converter::*)()>);
    static_assert(std::is_same_v<decltype(&Converter::rotate), void (Converter::*)(const StillPoint &, qreal)>);
    static_assert(std::is_same_v<decltype(&Converter::mirror), void (Converter::*)(const StillPoint &, bool, bool)>);
    static_assert(std::is_same_v<decltype(&Converter::xAxisMirrored), bool (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::yAxisMirrored), bool (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::resetRotation), void (Converter::*)(const StillPoint &)>);
    static_assert(std::is_same_v<decltype(&Converter::rotationAngle), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::preferredTransformationCenter), QPointF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(static_cast<void (Converter::*)(qreal)>(&Converter::setZoom)),
                                 void (Converter::*)(qreal)>);
    static_assert(std::is_same_v<
                  decltype(static_cast<void (Converter::*)(KoZoomMode::Mode, qreal, qreal, qreal, const StillPoint &)>(
                      &Converter::setZoom)),
                  void (Converter::*)(KoZoomMode::Mode, qreal, qreal, qreal, const StillPoint &)>);
    static_assert(std::is_same_v<decltype(&Converter::zoomTo), void (Converter::*)(const QRectF &)>);
    static_assert(std::is_same_v<decltype(&Converter::effectiveZoom), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::effectivePhysicalZoom), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::minZoom), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::maxZoom), qreal (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::clampZoom), qreal (Converter::*)(qreal) const>);
    static_assert(std::is_same_v<decltype(&Converter::standardZoomLevels), QVector<qreal> (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::findNextZoom), qreal (*)(qreal, const QVector<qreal> &)>);
    static_assert(std::is_same_v<decltype(&Converter::findPrevZoom), qreal (*)(qreal, const QVector<qreal> &)>);
}

void KisCoordinatesConverterSchemaContractTest::coordinatesConverterTransformAndStillPointSignaturesRemainStable()
{
    using Converter = KisCoordinatesConverter;

    static_assert(std::is_same_v<decltype(&Converter::documentToFlakeTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::documentToWidgetTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::flakeToWidgetTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageToDocumentTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageToViewportTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageToWidgetTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::viewportToWidgetTransform), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::viewToWidget), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::widgetToView), QTransform (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::makeDocStillPoint),
                                 KoViewTransformStillPoint (Converter::*)(const QPointF &) const>);
    static_assert(std::is_same_v<decltype(&Converter::makeWidgetStillPoint),
                                 KoViewTransformStillPoint (Converter::*)(const QPointF &) const>);
    static_assert(std::is_same_v<decltype(&Converter::flakeCenterPoint), QPointF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::widgetCenterPoint), QPointF (Converter::*)() const>);
}

void KisCoordinatesConverterSchemaContractTest::coordinatesConverterImageGeometrySignaturesRemainStable()
{
    using Converter = KisCoordinatesConverter;

    static_assert(std::is_same_v<decltype(&Converter::imageCenterInWidgetPixel), QPointF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageRectInDocumentPixels), QRectF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageRectInImagePixels), QRect (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageRectInViewportPixels), QRectF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageRectInWidgetPixels), QRectF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageSizeInFlakePixels), QSizeF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::widgetRectInFlakePixels), QRectF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::widgetRectInImagePixels), QRectF (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::imageScale), void (Converter::*)(qreal *, qreal *) const>);
    static_assert(
        std::is_same_v<decltype(&Converter::imagePhysicalScale), void (Converter::*)(qreal *, qreal *) const>);
    static_assert(
        std::is_same_v<decltype(&Converter::snapToDevicePixel), QPointF (Converter::*)(const QPointF &) const>);
    static_assert(
        std::is_same_v<decltype(&Converter::snapWidgetSizeToDevicePixel), QSizeF (Converter::*)(const QSizeF &) const>);
    static_assert(std::is_same_v<decltype(&Converter::minimumOffset), QPoint (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::maximumOffset), QPoint (Converter::*)() const>);
}

void KisCoordinatesConverterSchemaContractTest::coordinatesConverterCheckerProjectionSignaturesRemainStable()
{
    using Converter = KisCoordinatesConverter;

    static_assert(
        std::is_same_v<decltype(&Converter::getOpenGLCheckersInfo),
                       void (Converter::*)(const QRectF &, QTransform *, QTransform *, QRectF *, QRectF *, bool)
                           const>);
    static_assert(std::is_same_v<decltype(&Converter::getQPainterCheckersInfo),
                                 void (Converter::*)(QTransform *, QPointF *, QPolygonF *, bool) const>);
}

void KisCoordinatesConverterSchemaContractTest::coordinateTraitsMappingRemainsStable()
{
    using PointTraits = _Private::Traits<QPointF>;
    using RectTraits = _Private::Traits<QRectF>;

    static_assert(std::is_class_v<PointTraits>);
    static_assert(std::is_same_v<typename PointTraits::Result, QPointF>);
    static_assert(std::is_same_v<typename RectTraits::Result, QRectF>);
    static_assert(std::is_same_v<typename _Private::Traits<QRect>::Result, QRectF>);
    static_assert(std::is_same_v<typename _Private::Traits<QPoint>::Result, QPointF>);
    static_assert(std::is_same_v<typename _Private::Traits<QPolygon>::Result, QPolygonF>);
    static_assert(std::is_same_v<typename _Private::Traits<QLine>::Result, QLineF>);
    static_assert(std::is_same_v<decltype(&PointTraits::map), QPointF (*)(const QTransform &, const QPointF &)>);
    static_assert(std::is_same_v<decltype(&RectTraits::map), QRectF (*)(const QTransform &, const QRectF &)>);

    QTransform transform;
    transform.translate(7.0, -3.0);
    transform.rotate(30.0);
    transform.scale(2.0, 0.5);

    const QPointF point(1.25, -4.5);
    const QRectF rect(-2.0, 3.0, 5.0, 7.0);
    QCOMPARE(PointTraits::map(transform, point), transform.map(point));
    QCOMPARE(RectTraits::map(transform, rect), transform.mapRect(rect));
}

void KisCoordinatesConverterSchemaContractTest::imageViewportAndDocumentConversionSignaturesRemainStable()
{
    using PointConversion = QPointF (KisCoordinatesConverter::*)(const QPointF &) const;

    ASSERT_COORDINATE_CONVERSION_SIGNATURE(imageToViewport);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(viewportToImage);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(imageToDocument);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(documentToImage);

    QVERIFY(true);
}

void KisCoordinatesConverterSchemaContractTest::documentFlakeAndWidgetConversionSignaturesRemainStable()
{
    using PointConversion = QPointF (KisCoordinatesConverter::*)(const QPointF &) const;

    ASSERT_COORDINATE_CONVERSION_SIGNATURE(documentToFlake);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(flakeToDocument);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(flakeToWidget);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(widgetToFlake);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(documentToWidget);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(widgetToDocument);

    QVERIFY(true);
}

void KisCoordinatesConverterSchemaContractTest::widgetAndViewportConversionSignaturesRemainStable()
{
    using PointConversion = QPointF (KisCoordinatesConverter::*)(const QPointF &) const;

    ASSERT_COORDINATE_CONVERSION_SIGNATURE(widgetToViewport);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(viewportToWidget);

    QVERIFY(true);
}

void KisCoordinatesConverterSchemaContractTest::imageAndWidgetConversionSignaturesRemainStable()
{
    using PointConversion = QPointF (KisCoordinatesConverter::*)(const QPointF &) const;

    ASSERT_COORDINATE_CONVERSION_SIGNATURE(imageToWidget);
    ASSERT_COORDINATE_CONVERSION_SIGNATURE(widgetToImage);

    QVERIFY(true);
}

#undef ASSERT_COORDINATE_CONVERSION_SIGNATURE

QTEST_APPLESS_MAIN(KisCoordinatesConverterSchemaContractTest)

#include "KisCoordinatesConverterSchemaContractTest.moc"
