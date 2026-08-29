/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisHandlePainterHelper.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QString>
#include <QTest>

#include <optional>
#include <type_traits>
#include <utility>

void kis_assert_exception(const char *, const char *, int)
{
    qFatal("Unexpected handle decomposition assertion");
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    qFatal("Unexpected handle painter assertion");
}

QString __methodName(const char *prettyFunction)
{
    return QString::fromLatin1(prettyFunction);
}

namespace
{
const QColor handleFill(217, 31, 42, 255);
const QColor handleOutline(12, 23, 34, 255);
const QColor lineColor(25, 72, 210, 255);

KisHandleStyle distinctiveStyle()
{
    KisHandleStyle style;
    style.handleIterations.append(KisHandleStyle::IterationStyle(QPen(handleOutline, 1.0), QBrush(handleFill)));
    style.lineIterations.append(KisHandleStyle::IterationStyle(QPen(lineColor, 1.0), Qt::NoBrush));
    return style;
}

QRect opaqueBounds(const QImage &image)
{
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                bounds |= QRect(x, y, 1, 1);
            }
        }
    }
    return bounds;
}

int opaquePixelCount(const QImage &image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            count += qAlpha(image.pixel(x, y)) > 0;
        }
    }
    return count;
}

template<typename Draw>
QImage renderHandle(Draw draw, qreal handleRadius = 6.0)
{
    QImage image(160, 160, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setTransform(QTransform::fromTranslate(20.0, 30.0));

    KisHandlePainterHelper helper(&painter, handleRadius);
    helper.setHandleStyle(distinctiveStyle());
    draw(helper);
    return image;
}

void verifyCenteredNear(const QRect &bounds, const QPoint &center, int tolerance = 1)
{
    QVERIFY(!bounds.isEmpty());
    QVERIFY(qAbs(bounds.center().x() - center.x()) <= tolerance);
    QVERIFY(qAbs(bounds.center().y() - center.y()) <= tolerance);
}
} // namespace

class KisHandlePainterHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionMoveAndRestoration();
    void rectangularHandlesUseRadiusOffsetAndStyle();
    void circularAndGradientHandlesUseConfiguredGeometry();
    void linePrimitivesTransformDocumentCoordinates();
    void pixmapUsesTransformedPositionAndFixedOffset();
};

void KisHandlePainterHelperContractTest::constructionMoveAndRestoration()
{
    static_assert(!std::is_constructible<KisHandlePainterHelper, KisHandlePainterHelper &>::value,
                  "the handle painter helper must remain non-copyable");

    QImage image(80, 80, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const QTransform painterTransform = QTransform::fromTranslate(12.0, 18.0);
    painter.setTransform(painterTransform);

    std::optional<KisHandlePainterHelper> movedHelper;
    {
        KisHandlePainterHelper original(&painter, 5.0, 2);
        QCOMPARE(painter.transform(), QTransform());
        movedHelper.emplace(std::move(original));
    }
    QCOMPARE(painter.transform(), QTransform());
    movedHelper.reset();
    QCOMPARE(painter.transform(), painterTransform);

    const QTransform explicitRestore = QTransform::fromTranslate(3.0, 7.0);
    painter.setTransform(painterTransform);
    {
        KisHandlePainterHelper helper(&painter, explicitRestore, 4.0, 1);
        QCOMPARE(painter.transform(), QTransform());
    }
    QCOMPARE(painter.transform(), explicitRestore);
}

void KisHandlePainterHelperContractTest::rectangularHandlesUseRadiusOffsetAndStyle()
{
    const QImage customRadius = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleRect(QPointF(10.0, 10.0), 4.0);
    });
    verifyCenteredNear(opaqueBounds(customRadius), QPoint(30, 40));
    QCOMPARE(customRadius.pixelColor(30, 40), handleFill);

    const QImage offset = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleRect(QPointF(10.0, 10.0), 4.0, QPoint(7, 9));
    });
    verifyCenteredNear(opaqueBounds(offset), QPoint(37, 49));

    const QImage configuredRadius = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleRect(QPointF(10.0, 10.0));
    });
    QVERIFY(opaqueBounds(configuredRadius).width() > opaqueBounds(customRadius).width());

    const QColor fillColor(41, 190, 83, 255);
    const QImage filled = renderHandle([fillColor](KisHandlePainterHelper &helper) {
        helper.fillHandleRect(QPointF(10.0, 10.0), 3.0, fillColor, QPoint(-5, 6));
    });
    verifyCenteredNear(opaqueBounds(filled), QPoint(25, 46));
    QCOMPARE(filled.pixelColor(25, 46), fillColor);
}

void KisHandlePainterHelperContractTest::circularAndGradientHandlesUseConfiguredGeometry()
{
    const QImage customCircle = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleCircle(QPointF(10.0, 10.0), 4.0);
    });
    const QImage configuredCircle = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleCircle(QPointF(10.0, 10.0));
    });
    const QImage smallCircle = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleSmallCircle(QPointF(10.0, 10.0));
    });
    verifyCenteredNear(opaqueBounds(customCircle), QPoint(30, 40));
    QCOMPARE(customCircle.pixelColor(30, 40), handleFill);
    QVERIFY(opaqueBounds(configuredCircle).width() > opaqueBounds(customCircle).width());
    QVERIFY(opaqueBounds(smallCircle).width() < opaqueBounds(configuredCircle).width());

    const QImage gradient = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawGradientHandle(QPointF(10.0, 10.0), 4.0);
    });
    const QImage configuredGradient = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawGradientHandle(QPointF(10.0, 10.0));
    });
    const QImage cross = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawGradientCrossHandle(QPointF(10.0, 10.0), 8.0);
    });
    verifyCenteredNear(opaqueBounds(gradient), QPoint(30, 40));
    QVERIFY(opaqueBounds(configuredGradient).width() > opaqueBounds(gradient).width());
    QVERIFY(opaquePixelCount(cross) > opaquePixelCount(gradient));
}

void KisHandlePainterHelperContractTest::linePrimitivesTransformDocumentCoordinates()
{
    const QImage handleLine = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawHandleLine(QLineF(QPointF(5.0, 5.0), QPointF(25.0, 5.0)), 3.0, QVector<qreal>{2.0, 1.0}, 0.5);
    });
    QVERIFY(opaqueBounds(handleLine).contains(QPoint(35, 35)));
    QVERIFY(opaqueBounds(handleLine).contains(QPoint(45, 35)));

    const QPolygonF rubberPolygon{QPointF(5.0, 5.0), QPointF(25.0, 5.0), QPointF(25.0, 15.0)};
    const QImage rubber = renderHandle([rubberPolygon](KisHandlePainterHelper &helper) {
        helper.drawRubberLine(rubberPolygon);
    });
    QVERIFY(opaqueBounds(rubber).contains(QPoint(45, 35)));

    const QLineF connection(QPointF(5.0, 10.0), QPointF(25.0, 10.0));
    const QImage lineOverload = renderHandle([connection](KisHandlePainterHelper &helper) {
        helper.drawConnectionLine(connection);
    });
    const QImage pointOverload = renderHandle([connection](KisHandlePainterHelper &helper) {
        helper.drawConnectionLine(connection.p1(), connection.p2());
    });
    QCOMPARE(lineOverload, pointOverload);
    QVERIFY(opaqueBounds(lineOverload).contains(QPoint(45, 40)));

    QPainterPath path;
    path.moveTo(5.0, 5.0);
    path.cubicTo(10.0, 20.0, 20.0, 20.0, 25.0, 5.0);
    const QImage mappedPath = renderHandle([path](KisHandlePainterHelper &helper) {
        helper.drawPath(path);
    });
    QVERIFY(opaqueBounds(mappedPath).left() >= 25);
    QVERIFY(opaqueBounds(mappedPath).top() >= 35);

    const QImage arrow = renderHandle([](KisHandlePainterHelper &helper) {
        helper.drawGradientArrow(QPointF(5.0, 15.0), QPointF(45.0, 15.0), 5.0);
    });
    QVERIFY(opaqueBounds(arrow).contains(QPoint(45, 45)));
    QVERIFY(opaqueBounds(arrow).height() > opaqueBounds(lineOverload).height());
}

void KisHandlePainterHelperContractTest::pixmapUsesTransformedPositionAndFixedOffset()
{
    QPixmap pixmap(4, 4);
    const QColor pixmapColor(184, 37, 209, 255);
    pixmap.fill(pixmapColor);

    const QImage image = renderHandle([pixmap](KisHandlePainterHelper &helper) {
        helper.drawPixmap(pixmap, QPointF(10.0, 10.0), 8, QRectF(0.0, 0.0, 4.0, 4.0));
    });

    QCOMPARE(opaqueBounds(image), QRect(26, 76, 8, 8));
    QCOMPARE(image.pixelColor(30, 80), pixmapColor);
}

QTEST_MAIN(KisHandlePainterHelperContractTest)

#include "KisHandlePainterHelperContractTest.moc"
