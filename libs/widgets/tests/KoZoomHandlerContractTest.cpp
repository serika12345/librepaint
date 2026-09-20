/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <memory>

#include "KoZoomHandler.h"

namespace
{
class DestructionProbe : public KoZoomHandler
{
public:
    explicit DestructionProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~DestructionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};
} // namespace

class KoZoomHandlerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndVirtualLifetime();
    void settingsUpdateAxisScale();
    void axisAndPointConversionsRoundTrip();
    void rectangleAndSizeConversionsRoundTrip();
};

void KoZoomHandlerContractTest::defaultStateAndVirtualLifetime()
{
    KoZoomHandler handler;

    QCOMPARE(handler.resolutionX(), 1.0);
    QCOMPARE(handler.resolutionY(), 1.0);
    QCOMPARE(handler.zoomedResolutionX(), 1.0);
    QCOMPARE(handler.zoomedResolutionY(), 1.0);
    QCOMPARE(handler.zoomFactorX(), 1.0);
    QCOMPARE(handler.zoomFactorY(), 1.0);
    QCOMPARE(handler.zoomInPercent(), 100);
    QCOMPARE(handler.zoomMode(), KoZoomMode::ZOOM_CONSTANT);
    QCOMPARE(handler.zoomMarginSize(), 0);

    bool destroyed = false;
    {
        std::unique_ptr<KoViewConverter> converter(new DestructionProbe(&destroyed));
    }
    QVERIFY(destroyed);
}

void KoZoomHandlerContractTest::settingsUpdateAxisScale()
{
    KoZoomHandler handler;

    handler.setDpi(144, 216);
    QVERIFY(qAbs(handler.resolutionX() - 2.0) < 1e-12);
    QVERIFY(qAbs(handler.resolutionY() - 3.0) < 1e-12);

    handler.setZoom(0.25);
    QCOMPARE(handler.zoomInPercent(), 25);
    QVERIFY(qAbs(handler.zoomedResolutionX() - 0.5) < 1e-12);
    QVERIFY(qAbs(handler.zoomedResolutionY() - 0.75) < 1e-12);

    handler.setResolution(2.0, 4.0);
    QCOMPARE(handler.resolutionX(), 2.0);
    QCOMPARE(handler.resolutionY(), 4.0);
    QCOMPARE(handler.zoomedResolutionX(), 0.5);
    QCOMPARE(handler.zoomedResolutionY(), 1.0);

    handler.setZoomedResolution(5.0, 8.0);
    QCOMPARE(handler.zoomedResolutionX(), 5.0);
    QCOMPARE(handler.zoomedResolutionY(), 8.0);
    QCOMPARE(handler.zoomFactorX(), 2.5);
    QCOMPARE(handler.zoomFactorY(), 2.0);

    qreal zoomX = 0.0;
    qreal zoomY = 0.0;
    handler.zoom(&zoomX, &zoomY);
    QCOMPARE(zoomX, 5.0);
    QCOMPARE(zoomY, 8.0);

    handler.setZoomMode(KoZoomMode::ZOOM_PAGE);
    QCOMPARE(handler.zoomMode(), KoZoomMode::ZOOM_PAGE);
    handler.setZoomMarginSize(24);
    QCOMPARE(handler.zoomMarginSize(), 24);

    handler.setZoom(1.0 + 1e-13);
    QCOMPARE(handler.zoom(), 1.0);
    QCOMPARE(handler.zoomedResolutionX(), handler.resolutionX());
    QCOMPARE(handler.zoomedResolutionY(), handler.resolutionY());
}

void KoZoomHandlerContractTest::axisAndPointConversionsRoundTrip()
{
    KoZoomHandler handler;
    handler.setResolution(2.0, 4.0);
    handler.setZoom(0.5);

    QCOMPARE(handler.zoomItX(3.0), 3.0);
    QCOMPARE(handler.zoomItY(3.0), 6.0);
    QCOMPARE(handler.unzoomItX(3.0), 3.0);
    QCOMPARE(handler.unzoomItY(6.0), 3.0);

    const QPointF documentPoint(3.0, -2.0);
    const QPointF viewPoint(3.0, -4.0);
    QCOMPARE(handler.documentToView(documentPoint), viewPoint);
    QCOMPARE(handler.viewToDocument(viewPoint), documentPoint);
    QCOMPARE(handler.documentToViewX(documentPoint.x()), viewPoint.x());
    QCOMPARE(handler.documentToViewY(documentPoint.y()), viewPoint.y());
    QCOMPARE(handler.viewToDocumentX(viewPoint.x()), documentPoint.x());
    QCOMPARE(handler.viewToDocumentY(viewPoint.y()), documentPoint.y());
}

void KoZoomHandlerContractTest::rectangleAndSizeConversionsRoundTrip()
{
    KoZoomHandler handler;
    handler.setResolution(2.0, 4.0);
    handler.setZoom(0.5);

    const QRectF documentRect(1.0, 2.0, 3.0, 4.0);
    const QRectF viewRect(1.0, 4.0, 3.0, 8.0);
    QCOMPARE(handler.documentToView(documentRect), viewRect);
    QCOMPARE(handler.viewToDocument(viewRect), documentRect);

    const QSizeF documentSize(5.0, 6.0);
    const QSizeF viewSize(5.0, 12.0);
    QCOMPARE(handler.documentToView(documentSize), viewSize);
    QCOMPARE(handler.viewToDocument(viewSize), documentSize);
}

QTEST_APPLESS_MAIN(KoZoomHandlerContractTest)

#include "KoZoomHandlerContractTest.moc"
