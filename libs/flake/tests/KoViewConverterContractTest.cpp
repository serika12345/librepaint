/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoViewConverter.h"
#include "KoViewTransformStillPoint.h"

#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTest>
#include <QTransform>

namespace
{
class LifetimeProbe : public KoViewConverter
{
public:
    explicit LifetimeProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~LifetimeProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class AxisZoomProbe : public KoViewConverter
{
public:
    void zoom(qreal *zoomX, qreal *zoomY) const override
    {
        *zoomX = 2.0;
        *zoomY = 4.0;
    }
};

class StillPointProbe : public KoViewConverter
{
public:
    QTransform viewToWidget() const override
    {
        return QTransform::fromTranslate(-10.0, -20.0);
    }

    QTransform widgetToView() const override
    {
        return QTransform::fromTranslate(10.0, 20.0);
    }
};
} // namespace

class KoViewConverterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndVirtualLifetime();
    void zoomStateNormalizesZero();
    void finiteValuesRoundTripAcrossCoordinateTypes();
    void matrixConversionsUseAxisZoomAndDefaultWidgetSpace();
    void stillPointsComposeVirtualSpaces();
};

void KoViewConverterContractTest::defaultStateAndVirtualLifetime()
{
    const KoViewConverter converter;
    QCOMPARE(converter.zoom(), 1.0);

    bool destroyed = false;
    KoViewConverter *probe = new LifetimeProbe(&destroyed);
    delete probe;
    QVERIFY(destroyed);
}

void KoViewConverterContractTest::zoomStateNormalizesZero()
{
    KoViewConverter converter;
    converter.setZoom(2.5);

    qreal zoomX = 0.0;
    qreal zoomY = 0.0;
    converter.zoom(&zoomX, &zoomY);
    QCOMPARE(converter.zoom(), 2.5);
    QCOMPARE(zoomX, 2.5);
    QCOMPARE(zoomY, 2.5);

    converter.setZoom(0.0);
    QCOMPARE(converter.zoom(), 1.0);

    converter.setZoom(1.0);
    QCOMPARE(converter.zoom(), 1.0);
}

void KoViewConverterContractTest::finiteValuesRoundTripAcrossCoordinateTypes()
{
    KoViewConverter converter;
    converter.setZoom(2.0);

    QCOMPARE(converter.documentToViewX(-3.5), -7.0);
    QCOMPARE(converter.documentToViewY(4.25), 8.5);
    QCOMPARE(converter.viewToDocumentX(-7.0), -3.5);
    QCOMPARE(converter.viewToDocumentY(8.5), 4.25);

    const QPointF documentPoint(-3.5, 4.25);
    const QPointF viewPoint(-7.0, 8.5);
    QCOMPARE(converter.documentToView(documentPoint), viewPoint);
    QCOMPARE(converter.viewToDocument(viewPoint), documentPoint);

    const QSizeF documentSize(5.5, 7.25);
    const QSizeF viewSize(11.0, 14.5);
    QCOMPARE(converter.documentToView(documentSize), viewSize);
    QCOMPARE(converter.viewToDocument(viewSize), documentSize);

    const QRectF documentRect(documentPoint, documentSize);
    const QRectF viewRect(viewPoint, viewSize);
    QCOMPARE(converter.documentToView(documentRect), viewRect);
    QCOMPARE(converter.viewToDocument(viewRect), documentRect);

    converter.setZoom(1.0);
    QCOMPARE(converter.documentToView(documentPoint), documentPoint);
    QCOMPARE(converter.viewToDocument(viewRect), viewRect);
    QCOMPARE(converter.documentToView(documentSize), documentSize);
    QCOMPARE(converter.viewToDocument(viewSize), viewSize);
    QCOMPARE(converter.documentToView(documentRect), documentRect);
    QCOMPARE(converter.viewToDocument(viewRect), viewRect);
}

void KoViewConverterContractTest::matrixConversionsUseAxisZoomAndDefaultWidgetSpace()
{
    AxisZoomProbe converter;

    QCOMPARE(converter.documentToView(), QTransform::fromScale(2.0, 4.0));
    QCOMPARE(converter.viewToDocument(), QTransform::fromScale(0.5, 0.25));

    const KoViewConverter defaultConverter;
    QCOMPARE(defaultConverter.viewToWidget(), QTransform());
    QCOMPARE(defaultConverter.widgetToView(), QTransform());
}

void KoViewConverterContractTest::stillPointsComposeVirtualSpaces()
{
    StillPointProbe converter;
    converter.setZoom(2.0);

    const QPointF widgetPoint(3.0, 4.0);
    const KoViewTransformStillPoint widgetStillPoint = converter.makeWidgetStillPoint(widgetPoint);
    QCOMPARE(widgetStillPoint.docPoint(), QPointF(6.5, 12.0));
    QCOMPARE(widgetStillPoint.viewPoint(), widgetPoint);

    const QPointF documentPoint(3.0, 4.0);
    const KoViewTransformStillPoint documentStillPoint = converter.makeDocStillPoint(documentPoint);
    QCOMPARE(documentStillPoint.docPoint(), documentPoint);
    QCOMPARE(documentStillPoint.viewPoint(), QPointF(-4.0, -12.0));
}

QTEST_GUILESS_MAIN(KoViewConverterContractTest)

#include "KoViewConverterContractTest.moc"
