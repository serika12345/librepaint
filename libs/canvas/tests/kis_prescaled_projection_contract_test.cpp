/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_prescaled_projection_contract_test.h"

#include <memory>

#include <QColor>
#include <QImage>
#include <QPainter>

#include <KoColorSpaceRegistry.h>

#include <KisCanvasState.h>
#include <kis_coordinates_converter.h>
#include <kis_image.h>
#include <kis_image_patch.h>
#include <kis_prescaled_projection.h>
#include <kis_projection_backend.h>
#include <kis_projection_update_info.h>
#include <simpletest.h>

namespace {
class TestProjectionBackend final : public KisProjectionBackend
{
public:
    void setImage(KisImageWSP image) override
    {
        m_image = image;
    }

    void setImageSize(qint32, qint32) override {}

    void setMonitorProfile(const KoColorProfile *,
                           KoColorConversionTransformation::Intent,
                           KoColorConversionTransformation::ConversionFlags) override
    {
    }

    void setChannelFlags(const QBitArray &) override {}
    void setDisplayFilter(QSharedPointer<KisProjectionPixelFilter>) override {}

    void updateCache(const QRect &dirtyImageRect) override
    {
        lastCachedImageRect = dirtyImageRect;
    }

    void recalculateCache(KisProjectionUpdateInfoSP) override {}

    KisImagePatch getNearestPatch(KisProjectionUpdateInfoSP) override
    {
        return KisImagePatch();
    }

    void drawFromOriginalImage(QPainter &painter,
                               KisProjectionUpdateInfoSP info) override
    {
        painter.fillRect(info->viewportRect, frameColor);
    }

    QColor frameColor {Qt::red};
    QRect lastCachedImageRect;

private:
    KisImageWSP m_image;
};

struct ProjectionFixture
{
    ProjectionFixture()
    {
        const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
        image = new KisImage(nullptr, 16, 16, colorSpace, "projection contract");
        image->setResolution(1.0, 1.0);

        converter.setImage(image);
        converter.setResolution(1.0, 1.0);
        converter.setZoom(1.0);
        converter.setCanvasWidgetSize(QSize(16, 16));

        auto ownedBackend = std::make_unique<TestProjectionBackend>();
        backend = ownedBackend.get();
        projection = std::make_unique<KisPrescaledProjection>(
            std::move(ownedBackend), QSize(8, 8));
        projection->setCoordinatesConverter(&converter);
        projection->setImage(image);
        projection->notifyCanvasSizeChanged(QSize(16, 16));
    }

    KisImageSP image;
    KisCoordinatesConverter converter;
    TestProjectionBackend *backend {nullptr};
    std::unique_ptr<KisPrescaledProjection> projection;
};
}

void KisPrescaledProjectionContractTest::testDirtyRegionProducesProjectionNotification()
{
    ProjectionFixture fixture;
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(0, 0), QColor(Qt::red));

    fixture.backend->frameColor = Qt::green;
    const QRect dirtyImageRect(4, 4, 4, 4);
    KisUpdateInfoSP update = fixture.projection->updateCache(dirtyImageRect);

    QCOMPARE(update->dirtyImageRect(), dirtyImageRect);
    QCOMPARE(fixture.backend->lastCachedImageRect, dirtyImageRect);

    fixture.projection->recalculateCache(update);

    QVERIFY(!update->dirtyViewportRect().isEmpty());
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(5, 5),
             QColor(Qt::green));
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(15, 15),
             QColor(Qt::red));
}

void KisPrescaledProjectionContractTest::testEmptyUpdatePreservesLastValidFrame()
{
    ProjectionFixture fixture;
    const QImage lastValidFrame = fixture.projection->prescaledQImage();

    fixture.backend->frameColor = Qt::blue;
    KisUpdateInfoSP update = fixture.projection->updateCache(QRect(20, 20, 4, 4));
    fixture.projection->recalculateCache(update);

    QVERIFY(update->dirtyImageRect().isEmpty());
    QVERIFY(update->dirtyViewportRect().isEmpty());
    QCOMPARE(fixture.projection->prescaledQImage(), lastValidFrame);
}

void KisPrescaledProjectionContractTest::testTwoTimesZoomExpandsDirtyViewport()
{
    ProjectionFixture fixture;
    fixture.converter.setZoom(2.0);
    fixture.converter.setDocumentOffset(QPoint(0, 0));
    fixture.projection->notifyCanvasStateChanged(KisCanvasState::fromConverter(fixture.converter));

    QCOMPARE(fixture.projection->prescaledQImage().size(), QSize(16, 16));

    fixture.backend->frameColor = Qt::green;
    const QRect dirtyImageRect(2, 3, 2, 2);
    QCOMPARE(fixture.converter.imageToViewport(QRectF(dirtyImageRect)), QRectF(4, 6, 4, 4));

    KisUpdateInfoSP update = fixture.projection->updateCache(dirtyImageRect);
    fixture.projection->recalculateCache(update);

    QCOMPARE(update->dirtyImageRect(), dirtyImageRect);
    QCOMPARE(update->dirtyViewportRect(), QRect(2, 4, 8, 8));
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(2, 4), QColor(Qt::green));
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(9, 11), QColor(Qt::green));
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(1, 4), QColor(Qt::red));
    QCOMPARE(fixture.projection->prescaledQImage().pixelColor(10, 11), QColor(Qt::red));
}

SIMPLE_TEST_MAIN(KisPrescaledProjectionContractTest)
