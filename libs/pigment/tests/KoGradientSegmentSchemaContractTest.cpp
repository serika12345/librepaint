/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/KoSegmentGradient.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_GRADIENT_SEGMENT_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoGradientSegment::method)), signature>)
#define ASSERT_SEGMENT_GRADIENT_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoSegmentGradient::method)), signature>)
} // namespace

class KoGradientSegmentSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientSegmentInterpolationAndEndpointOrdinalsRemainStable();
    void gradientSegmentEndpointRecordSchemaRemainsStable();
    void gradientSegmentEndpointStateSignaturesRemainStable();
    void gradientSegmentConstructionAndInterpolationSignaturesRemainStable();
    void gradientSegmentEvaluationAndVariableColorSignaturesRemainStable();
    void segmentGradientTypeAndLifetimeSchemaRemainStable();
    void segmentGradientEvaluationAndResourceSignaturesRemainStable();
    void segmentGradientCreationAndCollectionSignaturesRemainStable();
    void segmentGradientEditingSignaturesRemainStable();
    void segmentGradientSerializationSignaturesRemainStable();
};

void KoGradientSegmentSchemaContractTest::gradientSegmentInterpolationAndEndpointOrdinalsRemainStable()
{
    static_assert(std::is_enum_v<decltype(INTERP_LINEAR)>);
    static_assert(std::is_enum_v<decltype(COLOR_INTERP_RGB)>);
    static_assert(!std::is_same_v<decltype(INTERP_LINEAR), decltype(COLOR_INTERP_RGB)>);

    static_assert(INTERP_LINEAR == 0);
    static_assert(INTERP_CURVED == 1);
    static_assert(INTERP_SINE == 2);
    static_assert(INTERP_SPHERE_INCREASING == 3);
    static_assert(INTERP_SPHERE_DECREASING == 4);

    static_assert(COLOR_INTERP_RGB == 0);
    static_assert(COLOR_INTERP_HSV_CCW == 1);
    static_assert(COLOR_INTERP_HSV_CW == 2);

    static_assert(std::is_enum_v<KoGradientSegmentEndpointType>);
    static_assert(COLOR_ENDPOINT == 0);
    static_assert(FOREGROUND_ENDPOINT == 1);
    static_assert(FOREGROUND_TRANSPARENT_ENDPOINT == 2);
    static_assert(BACKGROUND_ENDPOINT == 3);
    static_assert(BACKGROUND_TRANSPARENT_ENDPOINT == 4);
}

void KoGradientSegmentSchemaContractTest::gradientSegmentEndpointRecordSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoGradientSegmentEndpoint>);
    static_assert(std::is_constructible_v<KoGradientSegmentEndpoint, qreal, KoColor, KoGradientSegmentEndpointType>);
    static_assert(std::is_same_v<decltype(&KoGradientSegmentEndpoint::color), KoColor KoGradientSegmentEndpoint::*>);
    static_assert(std::is_same_v<decltype(&KoGradientSegmentEndpoint::offset), qreal KoGradientSegmentEndpoint::*>);
    static_assert(std::is_same_v<decltype(&KoGradientSegmentEndpoint::type),
                                 KoGradientSegmentEndpointType KoGradientSegmentEndpoint::*>);
}

void KoGradientSegmentSchemaContractTest::gradientSegmentEndpointStateSignaturesRemainStable()
{
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(endColor, const KoColor &(KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(endOffset, qreal (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(endType, KoGradientSegmentEndpointType (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(middleOffset, qreal (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setEndColor, void (KoGradientSegment::*)(const KoColor &));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setEndOffset, void (KoGradientSegment::*)(qreal));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setEndType, void (KoGradientSegment::*)(KoGradientSegmentEndpointType));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setMiddleOffset, void (KoGradientSegment::*)(qreal));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setStartColor, void (KoGradientSegment::*)(const KoColor &));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setStartOffset, void (KoGradientSegment::*)(qreal));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setStartType, void (KoGradientSegment::*)(KoGradientSegmentEndpointType));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(startColor, const KoColor &(KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(startOffset, qreal (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(startType, KoGradientSegmentEndpointType (KoGradientSegment::*)() const);
}

void KoGradientSegmentSchemaContractTest::gradientSegmentConstructionAndInterpolationSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoGradientSegment>);
    static_assert(std::is_constructible_v<KoGradientSegment,
                                          int,
                                          int,
                                          KoGradientSegmentEndpoint,
                                          KoGradientSegmentEndpoint,
                                          qreal>);

    ASSERT_GRADIENT_SEGMENT_SIGNATURE(colorInterpolation, int (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(interpolation, int (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(isValid, bool (KoGradientSegment::*)() const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(mirrorSegment, void (KoGradientSegment::*)());
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setColorInterpolation, void (KoGradientSegment::*)(int));
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setInterpolation, void (KoGradientSegment::*)(int));
}

void KoGradientSegmentSchemaContractTest::gradientSegmentEvaluationAndVariableColorSignaturesRemainStable()
{
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(colorAt, void (KoGradientSegment::*)(KoColor &, qreal) const);
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(hasVariableColors, bool (KoGradientSegment::*)());
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(length, qreal (KoGradientSegment::*)());
    ASSERT_GRADIENT_SEGMENT_SIGNATURE(setVariableColors, void (KoGradientSegment::*)(const KoColor &, const KoColor &));
}

void KoGradientSegmentSchemaContractTest::segmentGradientTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<KoSegmentGradientSP, QSharedPointer<KoSegmentGradient>>);
    static_assert(std::is_class_v<KoSegmentGradient>);
    static_assert(std::is_base_of_v<KoAbstractGradient, KoSegmentGradient>);
    static_assert(std::is_default_constructible_v<KoSegmentGradient>);
    static_assert(std::is_constructible_v<KoSegmentGradient, const QString &>);
    static_assert(std::is_copy_constructible_v<KoSegmentGradient>);
    static_assert(std::is_destructible_v<KoSegmentGradient>);
    static_assert(!std::is_copy_assignable_v<KoSegmentGradient>);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(clone, KoResourceSP (KoSegmentGradient::*)() const);
}

void KoGradientSegmentSchemaContractTest::segmentGradientEvaluationAndResourceSignaturesRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KoSegmentGradient::*)() const;

    ASSERT_SEGMENT_GRADIENT_SIGNATURE(colorAt, void (KoSegmentGradient::*)(KoColor &, qreal) const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(segmentAt, KoGradientSegment * (KoSegmentGradient::*)(qreal) const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(toQGradient, QGradient * (KoSegmentGradient::*)() const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(requiredCanvasResources, QList<int> (KoSegmentGradient::*)() const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(bakeVariableColors, void (KoSegmentGradient::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(updateVariableColors, void (KoSegmentGradient::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(resourceType, ResourceTypeSignature);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(defaultFileExtension, QString (KoSegmentGradient::*)() const);
}

void KoGradientSegmentSchemaContractTest::segmentGradientCreationAndCollectionSignaturesRemainStable()
{
    using CreateFromQColor = void (KoSegmentGradient::*)(int,
                                                         int,
                                                         double,
                                                         double,
                                                         double,
                                                         const QColor &,
                                                         const QColor &,
                                                         KoGradientSegmentEndpointType,
                                                         KoGradientSegmentEndpointType);
    using CreateFromKoColor = void (KoSegmentGradient::*)(int,
                                                          int,
                                                          double,
                                                          double,
                                                          double,
                                                          const KoColor &,
                                                          const KoColor &,
                                                          KoGradientSegmentEndpointType,
                                                          KoGradientSegmentEndpointType);

    ASSERT_SEGMENT_GRADIENT_SIGNATURE(createSegment, CreateFromQColor);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(createSegment, CreateFromKoColor);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(getHandlePositions, const QList<double> (KoSegmentGradient::*)() const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(getMiddleHandlePositions, const QList<double> (KoSegmentGradient::*)() const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(segments, const QList<KoGradientSegment *> &(KoSegmentGradient::*)() const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(setSegments, void (KoSegmentGradient::*)(const QList<KoGradientSegment *> &));

    static_assert(
        std::is_same_v<decltype(std::declval<KoSegmentGradient &>().createSegment(0,
                                                                                  0,
                                                                                  0.0,
                                                                                  1.0,
                                                                                  0.5,
                                                                                  std::declval<const QColor &>(),
                                                                                  std::declval<const QColor &>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoSegmentGradient &>().createSegment(0,
                                                                                  0,
                                                                                  0.0,
                                                                                  1.0,
                                                                                  0.5,
                                                                                  std::declval<const KoColor &>(),
                                                                                  std::declval<const KoColor &>())),
                       void>);
}

void KoGradientSegmentSchemaContractTest::segmentGradientEditingSignaturesRemainStable()
{
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(moveSegmentStartOffset, void (KoSegmentGradient::*)(KoGradientSegment *, double));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(moveSegmentEndOffset, void (KoSegmentGradient::*)(KoGradientSegment *, double));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(moveSegmentMiddleOffset,
                                      void (KoSegmentGradient::*)(KoGradientSegment *, double));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(splitSegment, void (KoSegmentGradient::*)(KoGradientSegment *));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(duplicateSegment, void (KoSegmentGradient::*)(KoGradientSegment *));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(mirrorSegment, void (KoSegmentGradient::*)(KoGradientSegment *));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(removeSegment, KoGradientSegment * (KoSegmentGradient::*)(KoGradientSegment *));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(collapseSegment, KoGradientSegment * (KoSegmentGradient::*)(KoGradientSegment *));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(removeSegmentPossible, bool (KoSegmentGradient::*)() const);
}

void KoGradientSegmentSchemaContractTest::segmentGradientSerializationSignaturesRemainStable()
{
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(loadFromDevice,
                                      bool (KoSegmentGradient::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(saveToDevice, bool (KoSegmentGradient::*)(QIODevice *) const);
    ASSERT_SEGMENT_GRADIENT_SIGNATURE(toXML, void (KoSegmentGradient::*)(QDomDocument &, QDomElement &) const);

    using FromXml = KoSegmentGradient (*)(const QDomElement &);
    static_assert(std::is_same_v<decltype(static_cast<FromXml>(&KoSegmentGradient::fromXML)), FromXml>);
}

QTEST_GUILESS_MAIN(KoGradientSegmentSchemaContractTest)

#include "KoGradientSegmentSchemaContractTest.moc"
