/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoViewTransformStillPoint.h>
#include <kis_coordinates_converter.h>

#include <QTest>



class KisCoordinatesConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void coordinateTraitsMappingRemainsStable();
};

void KisCoordinatesConverterSchemaContractTest::coordinateTraitsMappingRemainsStable()
{
    using PointTraits = _Private::Traits<QPointF>;
    using RectTraits = _Private::Traits<QRectF>;


    QTransform transform;
    transform.translate(7.0, -3.0);
    transform.rotate(30.0);
    transform.scale(2.0, 0.5);

    const QPointF point(1.25, -4.5);
    const QRectF rect(-2.0, 3.0, 5.0, 7.0);
    QCOMPARE(PointTraits::map(transform, point), transform.map(point));
    QCOMPARE(RectTraits::map(transform, rect), transform.mapRect(rect));
}

QTEST_APPLESS_MAIN(KisCoordinatesConverterSchemaContractTest)

#include "KisCoordinatesConverterSchemaContractTest.moc"
