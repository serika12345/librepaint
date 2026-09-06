/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_prescaled_projection.h>
#include <kis_projection_backend.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PROJECTION_BACKEND_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisProjectionBackend::method)), signature>)

#define ASSERT_PRESCALED_PROJECTION_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPrescaledProjection::method)), signature>)
} // namespace

class KisProjectionLifecycleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void projectionBackendTypeAndConfigurationSchemaRemainStable();
    void projectionBackendCacheAndDrawingSchemaRemainStable();
    void prescaledProjectionTypeAndLifetimeSchemaRemainStable();
    void prescaledProjectionImageAndDisplayStateSchemaRemainStable();
    void prescaledProjectionCacheAndScalingSchemaRemainStable();
};

void KisProjectionLifecycleSchemaContractTest::projectionBackendTypeAndConfigurationSchemaRemainStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;

    static_assert(std::is_class_v<KisProjectionBackend>);
    static_assert(std::is_abstract_v<KisProjectionBackend>);
    static_assert(std::has_virtual_destructor_v<KisProjectionBackend>);
    ASSERT_PROJECTION_BACKEND_SIGNATURE(setImage, void (KisProjectionBackend::*)(KisImageWSP));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(setImageSize, void (KisProjectionBackend::*)(qint32, qint32));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(
        setMonitorProfile,
        void (KisProjectionBackend::*)(const KoColorProfile *, Intent, ConversionFlags));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(setChannelFlags, void (KisProjectionBackend::*)(const QBitArray &));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(setDisplayFilter,
                                        void (KisProjectionBackend::*)(QSharedPointer<KisProjectionPixelFilter>));

    QVERIFY(true);
}

void KisProjectionLifecycleSchemaContractTest::projectionBackendCacheAndDrawingSchemaRemainStable()
{
    ASSERT_PROJECTION_BACKEND_SIGNATURE(updateCache, void (KisProjectionBackend::*)(const QRect &));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(recalculateCache, void (KisProjectionBackend::*)(KisProjectionUpdateInfoSP));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(alignSourceRect, void (KisProjectionBackend::*)(QRect &, qreal));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(getNearestPatch,
                                        KisImagePatch (KisProjectionBackend::*)(KisProjectionUpdateInfoSP));
    ASSERT_PROJECTION_BACKEND_SIGNATURE(drawFromOriginalImage,
                                        void (KisProjectionBackend::*)(QPainter &, KisProjectionUpdateInfoSP));

    QVERIFY(true);
}

void KisProjectionLifecycleSchemaContractTest::prescaledProjectionTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisPrescaledProjection>);
    static_assert(std::is_same_v<KisPrescaledProjectionSP, KisSharedPtr<KisPrescaledProjection>>);
    static_assert(
        std::is_constructible_v<KisPrescaledProjection, std::unique_ptr<KisProjectionBackend>, const QSize &>);
    static_assert(std::has_virtual_destructor_v<KisPrescaledProjection>);

    QVERIFY(true);
}

void KisProjectionLifecycleSchemaContractTest::prescaledProjectionImageAndDisplayStateSchemaRemainStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;

    ASSERT_PRESCALED_PROJECTION_SIGNATURE(setImage, void (KisPrescaledProjection::*)(KisImageWSP));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(setCoordinatesConverter,
                                          void (KisPrescaledProjection::*)(KisCoordinatesConverter *));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(notifyCanvasStateChanged,
                                          void (KisPrescaledProjection::*)(const KisCanvasState &));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(
        setMonitorProfile,
        void (KisPrescaledProjection::*)(const KoColorProfile *, Intent, ConversionFlags));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(setChannelFlags, void (KisPrescaledProjection::*)(const QBitArray &));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(setDisplayFilter,
                                          void (KisPrescaledProjection::*)(QSharedPointer<KisProjectionPixelFilter>));

    QVERIFY(true);
}

void KisProjectionLifecycleSchemaContractTest::prescaledProjectionCacheAndScalingSchemaRemainStable()
{
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(notifyCanvasSizeChanged, void (KisPrescaledProjection::*)(const QSize &));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(setUpdatePatchSize, void (KisPrescaledProjection::*)(const QSize &));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(slotImageSizeChanged, void (KisPrescaledProjection::*)(qint32, qint32));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(updateCache, KisUpdateInfoSP (KisPrescaledProjection::*)(const QRect &));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(recalculateCache, void (KisPrescaledProjection::*)(KisUpdateInfoSP));
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(preScale, void (KisPrescaledProjection::*)());
    ASSERT_PRESCALED_PROJECTION_SIGNATURE(prescaledQImage, QImage (KisPrescaledProjection::*)() const);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisProjectionLifecycleSchemaContractTest)

#include "KisProjectionLifecycleSchemaContractTest.moc"
