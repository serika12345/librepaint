/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathPoint.h"
#include "KoPathShape.h"

#include <KisHandlePainterHelper.h>
#include <KisHandleStyle.h>

#include <QImage>
#include <QPainter>
#include <QString>
#include <QTest>

#include <limits>
#include <type_traits>

void KoShape::notifyChanged()
{
    qFatal("KoShape::notifyChanged must not be used by this detached contract");
}

QRectF KoShape::shapeToDocument(const QRectF &) const
{
    qFatal("KoShape::shapeToDocument must not be used by this detached contract");
    return {};
}

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
using ShapeStorage = std::aligned_storage_t<sizeof(KoPathShape), alignof(KoPathShape)>;

KoPathShape *shapePointer(ShapeStorage &storage)
{
    return reinterpret_cast<KoPathShape *>(&storage);
}

KisHandleStyle distinctiveStyle()
{
    KisHandleStyle style;
    style.handleIterations.append(
        KisHandleStyle::IterationStyle(QPen(QColor(12, 23, 34), 1.0), QBrush(QColor(217, 31, 42))));
    style.lineIterations.append(KisHandleStyle::IterationStyle(QPen(QColor(25, 72, 210), 1.0), Qt::NoBrush));
    return style;
}

QImage renderPoint(KoPathPoint &point, KoPathPoint::PointTypes types, bool active = true)
{
    QImage image(96, 96, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setTransform(QTransform::fromTranslate(30.0, 30.0));

    KisHandlePainterHelper helper(&painter, 5.0);
    helper.setHandleStyle(distinctiveStyle());
    point.paint(helper, types, active);
    return image;
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
} // namespace

class KoPathPointContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumerationsExposeStableBitMasks();
    void constructionCopyAssignmentAndParentRules();
    void controlPointsAndPropertiesEnforceActivationRules();
    void mapsReversesAndExposesInvertedBoundingControlInclusion();
    void smoothnessUsesActiveOrNeighborTangents();
    void paintSelectsNodeAndActiveControlHandles();
};

void KoPathPointContractTest::enumerationsExposeStableBitMasks()
{
    static_assert(std::is_same_v<KoPathPoint::PointProperties, QFlags<KoPathPoint::PointProperty>>);
    static_assert(std::is_same_v<KoPathPoint::PointTypes, QFlags<KoPathPoint::PointType>>);

    QCOMPARE(int(KoPathPoint::Normal), 0);
    QCOMPARE(int(KoPathPoint::StartSubpath), 1);
    QCOMPARE(int(KoPathPoint::StopSubpath), 2);
    QCOMPARE(int(KoPathPoint::CloseSubpath), 8);
    QCOMPARE(int(KoPathPoint::IsSmooth), 16);
    QCOMPARE(int(KoPathPoint::IsSymmetric), 32);

    QCOMPARE(int(KoPathPoint::None), 0);
    QCOMPARE(int(KoPathPoint::Node), 1);
    QCOMPARE(int(KoPathPoint::ControlPoint1), 2);
    QCOMPARE(int(KoPathPoint::ControlPoint2), 4);
    QCOMPARE(int(KoPathPoint::All), 7);

    const KoPathPoint::PointProperties properties =
        KoPathPoint::StartSubpath | KoPathPoint::CloseSubpath | KoPathPoint::IsSmooth;
    QCOMPARE(int(properties), 25);
    const KoPathPoint::PointTypes types = KoPathPoint::Node | KoPathPoint::ControlPoint2;
    QCOMPARE(int(types), 5);
}

void KoPathPointContractTest::constructionCopyAssignmentAndParentRules()
{
    KoPathPoint defaultPoint;
    QCOMPARE(defaultPoint.point(), QPointF());
    QCOMPARE(defaultPoint.controlPoint1(), QPointF());
    QCOMPARE(defaultPoint.controlPoint2(), QPointF());
    QCOMPARE(defaultPoint.properties(), KoPathPoint::PointProperties(KoPathPoint::Normal));
    QVERIFY(!defaultPoint.activeControlPoint1());
    QVERIFY(!defaultPoint.activeControlPoint2());
    QCOMPARE(defaultPoint.parent(), nullptr);

    ShapeStorage firstStorage;
    ShapeStorage secondStorage;
    KoPathShape *const firstParent = shapePointer(firstStorage);
    KoPathShape *const secondParent = shapePointer(secondStorage);
    KoPathPoint original(firstParent, QPointF(4.5, -7.25), KoPathPoint::StartSubpath);
    QCOMPARE(original.point(), QPointF(4.5, -7.25));
    QCOMPARE(original.controlPoint1(), original.point());
    QCOMPARE(original.controlPoint2(), original.point());
    QCOMPARE(original.properties(), KoPathPoint::PointProperties(KoPathPoint::StartSubpath));
    QCOMPARE(original.parent(), firstParent);

    KoPathPoint detachedCopy(original);
    QVERIFY(detachedCopy == original);
    QCOMPARE(detachedCopy.parent(), nullptr);

    KoPathPoint reparentedCopy(original, secondParent);
    QVERIFY(reparentedCopy == original);
    QCOMPARE(reparentedCopy.parent(), secondParent);

    KoPathPoint assigned;
    assigned = original;
    QVERIFY(assigned == original);
    QCOMPARE(assigned.parent(), firstParent);

    assigned.setParent(secondParent);
    QCOMPARE(assigned.parent(), secondParent);
    QVERIFY(assigned == original);
    assigned.setParent(nullptr);
    original.setParent(nullptr);
    reparentedCopy.setParent(nullptr);
}

void KoPathPointContractTest::controlPointsAndPropertiesEnforceActivationRules()
{
    KoPathPoint point(nullptr, QPointF(10.0, 10.0));
    point.setPoint(QPointF(11.0, 12.0));
    QCOMPARE(point.point(), QPointF(11.0, 12.0));
    point.setPoint(QPointF(10.0, 10.0));

    point.setControlPoint1(QPointF(5.0, 10.0));
    point.setControlPoint2(QPointF(15.0, 10.0));
    QCOMPARE(point.controlPoint1(), QPointF(5.0, 10.0));
    QCOMPARE(point.controlPoint2(), QPointF(15.0, 10.0));
    QVERIFY(point.activeControlPoint1());
    QVERIFY(point.activeControlPoint2());

    point.setControlPoint1(QPointF(std::numeric_limits<qreal>::quiet_NaN(), 12.0));
    QCOMPARE(point.controlPoint1(), QPointF(5.0, 10.0));

    point.setProperty(KoPathPoint::IsSmooth);
    QVERIFY(point.properties() & KoPathPoint::IsSmooth);
    point.setProperty(KoPathPoint::IsSymmetric);
    QVERIFY(!(point.properties() & KoPathPoint::IsSmooth));
    QVERIFY(point.properties() & KoPathPoint::IsSymmetric);

    point.removeControlPoint1();
    QVERIFY(!point.activeControlPoint1());
    QVERIFY(!(point.properties() & KoPathPoint::IsSmooth));
    QVERIFY(!(point.properties() & KoPathPoint::IsSymmetric));
    point.removeControlPoint2();
    QVERIFY(!point.activeControlPoint2());

    point.setProperties(KoPathPoint::CloseSubpath);
    QCOMPARE(point.properties(), KoPathPoint::PointProperties(KoPathPoint::Normal));

    point.setControlPoint1(QPointF(5.0, 10.0));
    point.setControlPoint2(QPointF(15.0, 10.0));
    point.setProperties(KoPathPoint::StartSubpath | KoPathPoint::IsSmooth);
    QVERIFY(!point.activeControlPoint1());
    QVERIFY(point.activeControlPoint2());
    QVERIFY(!(point.properties() & KoPathPoint::IsSmooth));

    point.setProperties(KoPathPoint::StartSubpath | KoPathPoint::CloseSubpath | KoPathPoint::IsSmooth);
    QVERIFY(point.activeControlPoint1());
    QVERIFY(point.activeControlPoint2());
    QVERIFY(point.properties() & KoPathPoint::IsSmooth);

    point.setProperties(KoPathPoint::StopSubpath);
    QVERIFY(point.activeControlPoint1());
    QVERIFY(!point.activeControlPoint2());
    point.setProperties(KoPathPoint::StopSubpath | KoPathPoint::CloseSubpath);
    QVERIFY(point.activeControlPoint1());
    QVERIFY(point.activeControlPoint2());

    point.setProperties(KoPathPoint::StartSubpath | KoPathPoint::CloseSubpath | KoPathPoint::IsSmooth);
    point.unsetProperty(KoPathPoint::StartSubpath);
    QVERIFY(!(point.properties() & KoPathPoint::StartSubpath));
    QVERIFY(!(point.properties() & KoPathPoint::CloseSubpath));
    point.unsetProperty(KoPathPoint::IsSmooth);
    QVERIFY(!(point.properties() & KoPathPoint::IsSmooth));
}

void KoPathPointContractTest::mapsReversesAndExposesInvertedBoundingControlInclusion()
{
    KoPathPoint point(nullptr, QPointF(10.0, 10.0));
    point.setControlPoint1(QPointF(20.0, 20.0));
    point.setControlPoint2(QPointF(30.0, 15.0));

    QCOMPARE(point.boundingRect(true), QRectF(10.0, 10.0, 1.0, 1.0));
    QCOMPARE(point.boundingRect(false), QRectF(10.0, 10.0, 20.0, 10.0));

    point.map(QTransform::fromTranslate(4.0, -3.0));
    QCOMPARE(point.point(), QPointF(14.0, 7.0));
    QCOMPARE(point.controlPoint1(), QPointF(24.0, 17.0));
    QCOMPARE(point.controlPoint2(), QPointF(34.0, 12.0));

    KoPathPoint reversed(nullptr, QPointF(5.0, 5.0), KoPathPoint::StartSubpath | KoPathPoint::CloseSubpath);
    reversed.setControlPoint1(QPointF(2.0, 4.0));
    reversed.setControlPoint2(QPointF(8.0, 6.0));
    reversed.removeControlPoint2();
    QVERIFY(reversed.activeControlPoint1());
    QVERIFY(!reversed.activeControlPoint2());

    reversed.reverse();
    QCOMPARE(reversed.controlPoint1(), QPointF(8.0, 6.0));
    QCOMPARE(reversed.controlPoint2(), QPointF(2.0, 4.0));
    QVERIFY(!reversed.activeControlPoint1());
    QVERIFY(reversed.activeControlPoint2());
    QCOMPARE(reversed.properties(),
             KoPathPoint::PointProperties(KoPathPoint::StartSubpath | KoPathPoint::CloseSubpath));
}

void KoPathPointContractTest::smoothnessUsesActiveOrNeighborTangents()
{
    KoPathPoint controlled(nullptr, QPointF(1.0, 1.0));
    controlled.setControlPoint1(QPointF(0.0, 1.0));
    controlled.setControlPoint2(QPointF(2.0, 1.0));
    QVERIFY(controlled.isSmooth(nullptr, nullptr));

    controlled.setControlPoint2(QPointF(1.0, 2.0));
    QVERIFY(!controlled.isSmooth(nullptr, nullptr));

    KoPathPoint previous(nullptr, QPointF(0.0, 1.0));
    KoPathPoint middle(nullptr, QPointF(1.0, 1.0));
    KoPathPoint next(nullptr, QPointF(2.0, 1.0));
    QVERIFY(middle.isSmooth(&previous, &next));
    QVERIFY(!middle.isSmooth(nullptr, &next));

    KoPathPoint zeroTangent(nullptr, QPointF(3.0, 3.0));
    zeroTangent.setControlPoint1(zeroTangent.point());
    zeroTangent.setControlPoint2(QPointF(4.0, 3.0));
    QVERIFY(zeroTangent.isSmooth(nullptr, nullptr));
}

void KoPathPointContractTest::paintSelectsNodeAndActiveControlHandles()
{
    KoPathPoint normal(nullptr, QPointF(10.0, 10.0));
    const QImage normalNode = renderPoint(normal, KoPathPoint::Node);
    QVERIFY(!opaqueBounds(normalNode).isEmpty());

    KoPathPoint smooth(nullptr, QPointF(10.0, 10.0));
    smooth.setControlPoint1(QPointF(8.0, 10.0));
    smooth.setControlPoint2(QPointF(12.0, 10.0));
    smooth.setProperty(KoPathPoint::IsSmooth);
    const QImage smoothNode = renderPoint(smooth, KoPathPoint::Node);

    KoPathPoint symmetric(nullptr, QPointF(10.0, 10.0));
    symmetric.setControlPoint1(QPointF(8.0, 10.0));
    symmetric.setControlPoint2(QPointF(12.0, 10.0));
    symmetric.setProperty(KoPathPoint::IsSymmetric);
    const QImage symmetricNode = renderPoint(symmetric, KoPathPoint::Node);

    QVERIFY(normalNode != smoothNode);
    QVERIFY(smoothNode != symmetricNode);
    QVERIFY(normalNode != symmetricNode);

    KoPathPoint controls(nullptr, QPointF(10.0, 10.0));
    controls.setControlPoint1(QPointF(0.0, 10.0));
    controls.setControlPoint2(QPointF(20.0, 10.0));
    const QImage nodeOnly = renderPoint(controls, KoPathPoint::Node);
    const QImage allActive = renderPoint(controls, KoPathPoint::All);
    QVERIFY(opaqueBounds(allActive).width() > opaqueBounds(nodeOnly).width());
    QVERIFY(opaquePixelCount(allActive) > opaquePixelCount(nodeOnly));

    KoPathPoint inactive(nullptr, QPointF(10.0, 10.0));
    const QImage hiddenInactive = renderPoint(inactive, KoPathPoint::ControlPoint1, true);
    const QImage shownInactive = renderPoint(inactive, KoPathPoint::ControlPoint1, false);
    QVERIFY(opaqueBounds(hiddenInactive).isEmpty());
    QVERIFY(!opaqueBounds(shownInactive).isEmpty());
}

QTEST_GUILESS_MAIN(KoPathPointContractTest)

#include "KoPathPointContractTest.moc"
