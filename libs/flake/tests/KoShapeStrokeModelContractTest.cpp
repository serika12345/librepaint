/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoInsets.h"
#include "KoShapeStrokeModel.h"

#include <QPainter>
#include <QStringList>
#include <QTest>

class KoShape
{
};

namespace
{
class ShapeStrokeModelProbe : public KoShapeStrokeModel
{
public:
    explicit ShapeStrokeModelProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ShapeStrokeModelProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void strokeInsets(const KoShape *shape, KoInsets &insets) const override
    {
        events.append(QStringLiteral("strokeInsets"));
        observedShape = shape;
        observedInsets = &insets;
        insets.top = 1.5;
        insets.left = 2.5;
        insets.bottom = 3.5;
        insets.right = 4.5;
    }

    qreal strokeMaxMarkersInset(const KoShape *shape) const override
    {
        events.append(QStringLiteral("strokeMaxMarkersInset"));
        observedShape = shape;
        return 6.25;
    }

    bool hasTransparency() const override
    {
        events.append(QStringLiteral("hasTransparency"));
        return true;
    }

    void paint(const KoShape *shape, QPainter &painter) const override
    {
        events.append(QStringLiteral("paint"));
        observedShape = shape;
        observedPainter = &painter;
    }

    void paintMarkers(const KoShape *shape, QPainter &painter) const override
    {
        events.append(QStringLiteral("paintMarkers"));
        observedShape = shape;
        observedMarkerPainter = &painter;
    }

    bool compareFillTo(const KoShapeStrokeModel *other) override
    {
        events.append(QStringLiteral("compareFillTo"));
        fillComparedWith = other;
        return true;
    }

    bool compareStyleTo(const KoShapeStrokeModel *other) override
    {
        events.append(QStringLiteral("compareStyleTo"));
        styleComparedWith = other;
        return false;
    }

    bool isVisible() const override
    {
        events.append(QStringLiteral("isVisible"));
        return true;
    }

    mutable QStringList events;
    mutable const KoShape *observedShape = nullptr;
    mutable KoInsets *observedInsets = nullptr;
    mutable QPainter *observedPainter = nullptr;
    mutable QPainter *observedMarkerPainter = nullptr;
    const KoShapeStrokeModel *fillComparedWith = nullptr;
    const KoShapeStrokeModel *styleComparedWith = nullptr;

private:
    int *m_destructionCount;
};
} // namespace

class KoShapeStrokeModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dispatchesGeometryPaintingAndStateThroughBase();
    void dispatchesFillAndStyleComparisonsThroughBase();
    void destroysDerivedStrokeThroughBase();
};

void KoShapeStrokeModelContractTest::dispatchesGeometryPaintingAndStateThroughBase()
{
    ShapeStrokeModelProbe stroke;
    KoShape shape;
    KoInsets insets;
    QPainter painter;
    const KoShapeStrokeModel *interface = &stroke;

    interface->strokeInsets(&shape, insets);
    QCOMPARE(interface->strokeMaxMarkersInset(&shape), 6.25);
    QVERIFY(interface->hasTransparency());
    interface->paint(&shape, painter);
    interface->paintMarkers(&shape, painter);
    QVERIFY(interface->isVisible());

    QCOMPARE(insets.top, 1.5);
    QCOMPARE(insets.left, 2.5);
    QCOMPARE(insets.bottom, 3.5);
    QCOMPARE(insets.right, 4.5);
    QCOMPARE(stroke.observedShape, &shape);
    QCOMPARE(stroke.observedInsets, &insets);
    QCOMPARE(stroke.observedPainter, &painter);
    QCOMPARE(stroke.observedMarkerPainter, &painter);
    const QStringList expectedEvents = {
        QStringLiteral("strokeInsets"),
        QStringLiteral("strokeMaxMarkersInset"),
        QStringLiteral("hasTransparency"),
        QStringLiteral("paint"),
        QStringLiteral("paintMarkers"),
        QStringLiteral("isVisible"),
    };
    QCOMPARE(stroke.events, expectedEvents);
}

void KoShapeStrokeModelContractTest::dispatchesFillAndStyleComparisonsThroughBase()
{
    ShapeStrokeModelProbe stroke;
    ShapeStrokeModelProbe other;
    KoShapeStrokeModel *interface = &stroke;

    QVERIFY(interface->compareFillTo(&other));
    QVERIFY(!interface->compareStyleTo(&other));

    QCOMPARE(stroke.fillComparedWith, &other);
    QCOMPARE(stroke.styleComparedWith, &other);
    const QStringList expectedEvents = {
        QStringLiteral("compareFillTo"),
        QStringLiteral("compareStyleTo"),
    };
    QCOMPARE(stroke.events, expectedEvents);
}

void KoShapeStrokeModelContractTest::destroysDerivedStrokeThroughBase()
{
    int destructionCount = 0;
    KoShapeStrokeModel *stroke = new ShapeStrokeModelProbe(&destructionCount);

    delete stroke;

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoShapeStrokeModelContractTest)

#include "KoShapeStrokeModelContractTest.moc"
