/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <color/kis_display_color_transform.h>
#include <kis_shared_ptr.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_TRANSFORM_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDisplayColorTransform::method)), signature>)
} // namespace

class KisDisplayColorTransformSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayColorTransformIdentityAndLifecycleSchemaRemainsStable();
    void displayColorTransformConfigurationSignaturesRemainStable();
    void displayColorTransformColorConversionSignaturesRemainStable();
    void displayColorTransformImageConversionSignaturesRemainStable();
    void displayColorTransformComponentModelSignaturesRemainStable();
};

void KisDisplayColorTransformSchemaContractTest::displayColorTransformIdentityAndLifecycleSchemaRemainsStable()
{
    using Transform = KisDisplayColorTransform;
    using Options =
        std::pair<KoColorConversionTransformation::Intent, KoColorConversionTransformation::ConversionFlags>;

    static_assert(std::is_class_v<Transform>);
    static_assert(std::is_same_v<Transform::ConversionOptions, Options>);
    static_assert(std::is_constructible_v<Transform>);
    static_assert(std::is_destructible_v<Transform>);
}

void KisDisplayColorTransformSchemaContractTest::displayColorTransformConfigurationSignaturesRemainStable()
{
    using Transform = KisDisplayColorTransform;

    ASSERT_TRANSFORM_SIGNATURE(setDisplayConfiguration,
                               void (Transform::*)(const KoColorProfile *,
                                                   const KoColorProfile *,
                                                   KoColorConversionTransformation::Intent,
                                                   KoColorConversionTransformation::ConversionFlags));
    ASSERT_TRANSFORM_SIGNATURE(setInputColorSpace, void (Transform::*)(const KoColorSpace *));
    ASSERT_TRANSFORM_SIGNATURE(setPaintingColorSpace, void (Transform::*)(const KoColorSpace *));
    ASSERT_TRANSFORM_SIGNATURE(setDisplayFilter, void (Transform::*)(QSharedPointer<KisDisplayColorFilter>));
    ASSERT_TRANSFORM_SIGNATURE(paintingColorSpace, const KoColorSpace *(Transform::*)() const);
    ASSERT_TRANSFORM_SIGNATURE(displayFilter, QSharedPointer<KisDisplayColorFilter> (Transform::*)() const);
    ASSERT_TRANSFORM_SIGNATURE(conversionOptions, Transform::ConversionOptions (Transform::*)() const);
    ASSERT_TRANSFORM_SIGNATURE(usesDisplayFilter, bool (Transform::*)() const);
}

void KisDisplayColorTransformSchemaContractTest::displayColorTransformColorConversionSignaturesRemainStable()
{
    using Transform = KisDisplayColorTransform;

    ASSERT_TRANSFORM_SIGNATURE(toQColor, QColor (Transform::*)(const KoColor &, bool) const);
    ASSERT_TRANSFORM_SIGNATURE(approximateFromRenderedQColor, KoColor (Transform::*)(const QColor &) const);
    ASSERT_TRANSFORM_SIGNATURE(canSkipDisplayConversion, bool (Transform::*)(const KoColorSpace *) const);
    ASSERT_TRANSFORM_SIGNATURE(applyDisplayFiltering, KoColor (Transform::*)(const KoColor &, const KoID &) const);
    ASSERT_TRANSFORM_SIGNATURE(applyDisplayFilteringF32,
                               void (Transform::*)(KisFixedPaintDeviceSP, const KoColorSpace *) const);
    ASSERT_TRANSFORM_SIGNATURE(captureVisualRepresentation, KoColor (Transform::*)(const KoColor &) const);
    ASSERT_TRANSFORM_SIGNATURE(restorePaintingColor, KoColor (Transform::*)(const KoColor &) const);

    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>().toQColor(std::declval<const KoColor &>())), QColor>);
}

void KisDisplayColorTransformSchemaContractTest::displayColorTransformImageConversionSignaturesRemainStable()
{
    using Transform = KisDisplayColorTransform;

    ASSERT_TRANSFORM_SIGNATURE(toQImage, QImage (Transform::*)(KisPaintDeviceSP, bool) const);
    ASSERT_TRANSFORM_SIGNATURE(toQImage,
                               QImage (Transform::*)(const KoColorSpace *, const quint8 *, QSize, bool) const);
    ASSERT_TRANSFORM_SIGNATURE(convertColorToDisplayColorSpace, QColor (Transform::*)(const KoColor &, bool) const);
    ASSERT_TRANSFORM_SIGNATURE(convertImageToDisplayColorSpace,
                               QImage (Transform::*)(KisPaintDeviceSP, QRect, bool) const);

    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>().toQImage(std::declval<KisPaintDeviceSP>())), QImage>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>().toQImage(std::declval<const KoColorSpace *>(),
                                                                           std::declval<const quint8 *>(),
                                                                           std::declval<QSize>())),
                       QImage>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().convertColorToDisplayColorSpace(
                                     std::declval<const KoColor &>())),
                                 QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().convertImageToDisplayColorSpace(
                                     std::declval<KisPaintDeviceSP>())),
                                 QImage>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().convertImageToDisplayColorSpace(
                                     std::declval<KisPaintDeviceSP>(),
                                     std::declval<QRect>())),
                                 QImage>);
}

void KisDisplayColorTransformSchemaContractTest::displayColorTransformComponentModelSignaturesRemainStable()
{
    using Transform = KisDisplayColorTransform;

    ASSERT_TRANSFORM_SIGNATURE(fromHsiF, KoColor (Transform::*)(qreal, qreal, qreal) const);
    ASSERT_TRANSFORM_SIGNATURE(fromHslF, KoColor (Transform::*)(qreal, qreal, qreal, qreal) const);
    ASSERT_TRANSFORM_SIGNATURE(fromHsv, KoColor (Transform::*)(int, int, int, int) const);
    ASSERT_TRANSFORM_SIGNATURE(fromHsvF, KoColor (Transform::*)(qreal, qreal, qreal, qreal) const);
    ASSERT_TRANSFORM_SIGNATURE(fromHsyF, KoColor (Transform::*)(qreal, qreal, qreal, qreal, qreal, qreal, qreal) const);
    ASSERT_TRANSFORM_SIGNATURE(getHsiF, void (Transform::*)(const KoColor &, qreal *, qreal *, qreal *) const);
    ASSERT_TRANSFORM_SIGNATURE(getHslF, void (Transform::*)(const KoColor &, qreal *, qreal *, qreal *, qreal *) const);
    ASSERT_TRANSFORM_SIGNATURE(getHsv, void (Transform::*)(const KoColor &, int *, int *, int *, int *) const);
    ASSERT_TRANSFORM_SIGNATURE(getHsvF, void (Transform::*)(const KoColor &, qreal *, qreal *, qreal *, qreal *) const);
    ASSERT_TRANSFORM_SIGNATURE(
        getHsyF,
        void (Transform::*)(const KoColor &, qreal *, qreal *, qreal *, qreal, qreal, qreal, qreal) const);

    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().fromHslF(0.0, 0.0, 0.0)), KoColor>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().fromHsv(0, 0, 0)), KoColor>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().fromHsvF(0.0, 0.0, 0.0)), KoColor>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().fromHsyF(0.0, 0.0, 0.0)), KoColor>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>().fromHsyF(0.0, 0.0, 0.0, 0.2)), KoColor>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>().fromHsyF(0.0, 0.0, 0.0, 0.2, 0.7)), KoColor>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>().fromHsyF(0.0, 0.0, 0.0, 0.2, 0.7, 0.1)), KoColor>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>()
                                              .getHslF(std::declval<const KoColor &>(), nullptr, nullptr, nullptr)),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>()
                                              .getHsv(std::declval<const KoColor &>(), nullptr, nullptr, nullptr)),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>()
                                              .getHsvF(std::declval<const KoColor &>(), nullptr, nullptr, nullptr)),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<const Transform &>()
                                              .getHsyF(std::declval<const KoColor &>(), nullptr, nullptr, nullptr)),
                                 void>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>()
                                    .getHsyF(std::declval<const KoColor &>(), nullptr, nullptr, nullptr, 0.2)),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Transform &>()
                                    .getHsyF(std::declval<const KoColor &>(), nullptr, nullptr, nullptr, 0.2, 0.7)),
                       void>);
    static_assert(std::is_same_v<
                  decltype(std::declval<const Transform &>()
                               .getHsyF(std::declval<const KoColor &>(), nullptr, nullptr, nullptr, 0.2, 0.7, 0.1)),
                  void>);
}

QTEST_APPLESS_MAIN(KisDisplayColorTransformSchemaContractTest)

#include "KisDisplayColorTransformSchemaContractTest.moc"
