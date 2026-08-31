/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/KisOptimizedBrushOutline.h"

#include <QPainterPath>
#include <QTest>

#include <type_traits>

namespace
{

constexpr qreal tolerance = 1e-6;

QVector<QPolygonF> polygons(const KisOptimizedBrushOutline &outline)
{
    QVector<QPolygonF> result;
    for (auto it = outline.begin(); it != outline.end(); ++it) {
        result.append(*it);
    }
    return result;
}

void comparePoint(const QPointF &actual, const QPointF &expected, qreal allowedError = tolerance)
{
    QVERIFY2(qAbs(actual.x() - expected.x()) <= allowedError, "x coordinate differs");
    QVERIFY2(qAbs(actual.y() - expected.y()) <= allowedError, "y coordinate differs");
}

void comparePolygon(const QPolygonF &actual, const QPolygonF &expected, qreal allowedError = tolerance)
{
    QCOMPARE(actual.size(), expected.size());
    for (qsizetype i = 0; i < actual.size(); ++i) {
        comparePoint(actual.at(i), expected.at(i), allowedError);
    }
}

void compareRect(const QRectF &actual, const QRectF &expected, qreal allowedError = tolerance)
{
    const QByteArray message = QStringLiteral("actual=(%1,%2 %3x%4), expected=(%5,%6 %7x%8)")
                                   .arg(actual.x())
                                   .arg(actual.y())
                                   .arg(actual.width())
                                   .arg(actual.height())
                                   .arg(expected.x())
                                   .arg(expected.y())
                                   .arg(expected.width())
                                   .arg(expected.height())
                                   .toUtf8();
    QVERIFY2(qAbs(actual.left() - expected.left()) <= allowedError, message.constData());
    QVERIFY2(qAbs(actual.top() - expected.top()) <= allowedError, message.constData());
    QVERIFY2(qAbs(actual.right() - expected.right()) <= allowedError, message.constData());
    QVERIFY2(qAbs(actual.bottom() - expected.bottom()) <= allowedError, message.constData());
}

void comparePolygons(const QVector<QPolygonF> &actual, const QVector<QPolygonF> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (qsizetype i = 0; i < actual.size(); ++i) {
        comparePolygon(actual.at(i), expected.at(i));
    }
}

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class KisOptimizedBrushOutlineContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyStateAndIteratorsPreservePolygonOrder();
    void pathPolygonConstructionAndBoundsOwnValues();
    void additionsInvalidateBoundsAndOwnAppendedGeometry();
    void translationsMapsAndMappedPreserveValueSemantics();
};

void KisOptimizedBrushOutlineContractTest::emptyStateAndIteratorsPreservePolygonOrder()
{
    static_assert(std::is_default_constructible_v<KisOptimizedBrushOutline>);
    static_assert(std::is_default_constructible_v<KisOptimizedBrushOutline::const_iterator>);

    KisOptimizedBrushOutline empty;
    QVERIFY(empty.isEmpty());
    QVERIFY(empty.begin() == empty.end());

    KisOptimizedBrushOutline::const_iterator defaultIterator;
    KisOptimizedBrushOutline::const_iterator anotherDefaultIterator;
    QVERIFY(defaultIterator == anotherDefaultIterator);

    const QPolygonF first{{1.0, 2.0}, {4.0, 2.0}, {4.0, 5.0}};
    const QPolygonF second{{-3.0, -4.0}, {-1.0, -4.0}, {-1.0, -2.0}};
    const KisOptimizedBrushOutline outline(QVector<QPolygonF>{first, second});

    QVERIFY(!outline.isEmpty());
    KisOptimizedBrushOutline::const_iterator firstIterator(&outline, 0);
    KisOptimizedBrushOutline::const_iterator secondIterator(&outline, 1);
    KisOptimizedBrushOutline::const_iterator explicitEnd(&outline, 2);
    QVERIFY(firstIterator == outline.begin());
    QVERIFY(explicitEnd == outline.end());
    comparePolygon(*firstIterator, first);
    comparePolygon(*secondIterator, second);
    comparePolygons(polygons(outline), QVector<QPolygonF>{first, second});
}

void KisOptimizedBrushOutlineContractTest::pathPolygonConstructionAndBoundsOwnValues()
{
    QPainterPath sourcePath;
    sourcePath.addRect(QRectF(1.0, 2.0, 4.0, 6.0));
    const KisOptimizedBrushOutline pathOutline(sourcePath);
    const QVector<QPolygonF> ownedPathPolygons = polygons(pathOutline);

    QCOMPARE(ownedPathPolygons.size(), 1);
    compareRect(ownedPathPolygons.first().boundingRect(), QRectF(1.0, 2.0, 4.0, 6.0));
    compareRect(pathOutline.boundingRect(), QRectF(1.0, 2.0, 4.0, 6.0));

    sourcePath = QPainterPath();
    sourcePath.addRect(QRectF(100.0, 200.0, 40.0, 60.0));
    comparePolygons(polygons(pathOutline), ownedPathPolygons);
    compareRect(pathOutline.boundingRect(), QRectF(1.0, 2.0, 4.0, 6.0));

    QPolygonF first{{10.0, 11.0}, {14.0, 11.0}, {14.0, 15.0}};
    QPolygonF second{{20.0, 21.0}, {24.0, 21.0}, {24.0, 25.0}};
    QVector<QPolygonF> sourcePolygons{first, second};
    const QRectF explicitBounds(-7.0, -8.0, 30.0, 40.0);
    const KisOptimizedBrushOutline polygonOutline(sourcePolygons, explicitBounds);

    sourcePolygons.first().first() = QPointF(1000.0, 2000.0);
    sourcePolygons.clear();
    comparePolygons(polygons(polygonOutline), QVector<QPolygonF>{first, second});
    compareRect(polygonOutline.boundingRect(), QRectF(-7.0, -8.0, 31.0, 40.0));
}

void KisOptimizedBrushOutlineContractTest::additionsInvalidateBoundsAndOwnAppendedGeometry()
{
    KisOptimizedBrushOutline outline;
    compareRect(outline.boundingRect(), QRectF());

    outline.addRect(QRectF(10.0, 20.0, 4.0, 3.0));
    compareRect(outline.boundingRect(), QRectF(10.0, 20.0, 4.0, 3.0));

    outline.addEllipse(QPointF(-5.0, 4.0), 2.0, 3.0);
    compareRect(outline.boundingRect(), QRectF(-7.0, 1.0, 21.0, 22.0), 1e-4);

    QPainterPath path;
    path.moveTo(30.0, -2.0);
    path.lineTo(34.0, -2.0);
    path.lineTo(32.0, 3.0);
    path.closeSubpath();
    outline.addPath(path);
    compareRect(outline.boundingRect(), QRectF(-7.0, -2.0, 41.0, 25.0), 1e-4);

    path = QPainterPath();
    path.addRect(QRectF(1000.0, 2000.0, 5.0, 6.0));

    const QPolygonF appendedFirst{{-20.0, -10.0}, {-18.0, -10.0}, {-18.0, -8.0}};
    const QPolygonF appendedSecond{{40.0, 50.0}, {42.0, 50.0}, {42.0, 52.0}};
    {
        KisOptimizedBrushOutline appended(QVector<QPolygonF>{appendedFirst, appendedSecond});
        appended.translate(3.0, -4.0);
        outline.addPath(appended);
        appended.translate(1000.0, 2000.0);
    }

    const QVector<QPolygonF> result = polygons(outline);
    QCOMPARE(result.size(), 5);
    compareRect(result.at(0).boundingRect(), QRectF(10.0, 20.0, 4.0, 3.0));
    compareRect(result.at(1).boundingRect(), QRectF(-7.0, 1.0, 4.0, 6.0), 1e-4);
    compareRect(result.at(2).boundingRect(), QRectF(30.0, -2.0, 4.0, 5.0));
    comparePolygon(result.at(3), QTransform::fromTranslate(3.0, -4.0).map(appendedFirst));
    comparePolygon(result.at(4), QTransform::fromTranslate(3.0, -4.0).map(appendedSecond));
    compareRect(outline.boundingRect(), QRectF(-17.0, -14.0, 62.0, 62.0), 1e-4);
}

void KisOptimizedBrushOutlineContractTest::translationsMapsAndMappedPreserveValueSemantics()
{
    const QPolygonF first{{1.0, 2.0}, {4.0, 2.0}, {4.0, 5.0}};
    const QPolygonF second{{-2.0, -1.0}, {0.0, -1.0}};
    const QVector<QPolygonF> original{first, second};
    const KisOptimizedBrushOutline source(original);

    KisOptimizedBrushOutline scalarTranslated = source;
    scalarTranslated.translate(3.0, -4.0);
    comparePolygons(polygons(scalarTranslated),
                    QVector<QPolygonF>{{{4.0, -2.0}, {7.0, -2.0}, {7.0, 1.0}}, {{1.0, -5.0}, {3.0, -5.0}}});

    KisOptimizedBrushOutline pointTranslated = source;
    pointTranslated.translate(QPointF(-5.0, 6.0));
    comparePolygons(polygons(pointTranslated),
                    QVector<QPolygonF>{{{-4.0, 8.0}, {-1.0, 8.0}, {-1.0, 11.0}}, {{-7.0, 5.0}, {-5.0, 5.0}}});

    KisOptimizedBrushOutline mappedInPlace = source;
    mappedInPlace.map(QTransform(2.0, 0.0, 0.0, 3.0, 5.0, -7.0));
    comparePolygons(polygons(mappedInPlace),
                    QVector<QPolygonF>{{{7.0, -1.0}, {13.0, -1.0}, {13.0, 8.0}}, {{1.0, -10.0}, {5.0, -10.0}}});

    const KisOptimizedBrushOutline mappedCopy = source.mapped(QTransform(-1.0, 0.0, 0.0, 1.0, 10.0, 4.0));
    comparePolygons(polygons(mappedCopy),
                    QVector<QPolygonF>{{{9.0, 6.0}, {6.0, 6.0}, {6.0, 9.0}}, {{12.0, 3.0}, {10.0, 3.0}}});
    comparePolygons(polygons(source), original);
}

QTEST_GUILESS_MAIN(KisOptimizedBrushOutlineContractTest)

#include "KisOptimizedBrushOutlineContractTest.moc"
