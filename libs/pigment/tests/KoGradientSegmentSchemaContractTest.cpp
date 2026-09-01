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

QTEST_GUILESS_MAIN(KoGradientSegmentSchemaContractTest)

#include "KoGradientSegmentSchemaContractTest.moc"
