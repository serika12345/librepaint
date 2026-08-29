/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2026 LibrePaint contributors

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoCurveFit.h"
#include "KoCurveFitPathShapeWriter_p.h"

#include <QTest>
#include <QVector>

#include <cmath>

namespace
{

enum class PathCommandType {
    Move,
    Curve
};

struct PathCommand {
    PathCommandType type;
    QPointF point1;
    QPointF point2;
    QPointF point3;

    bool operator==(const PathCommand &other) const
    {
        return type == other.type && point1 == other.point1 && point2 == other.point2 && point3 == other.point3;
    }
};

QVector<PathCommand> pathCommands;
alignas(void *) unsigned char pathStorage = 0;
KoPathShape *const recordedPath = reinterpret_cast<KoPathShape *>(&pathStorage);

bool pointsAreNear(const QPointF &actual, const QPointF &expected)
{
    return std::hypot(actual.x() - expected.x(), actual.y() - expected.y()) < 1.0e-9;
}

int curveCount(const QVector<PathCommand> &commands)
{
    int count = 0;
    for (const PathCommand &command : commands) {
        count += command.type == PathCommandType::Curve;
    }
    return count;
}

} // namespace

namespace KoCurveFitPathShapeWriter
{

KoPathShape *createPath()
{
    pathCommands.clear();
    return recordedPath;
}

void moveTo(KoPathShape *path, const QPointF &point)
{
    QCOMPARE(path, recordedPath);
    pathCommands.append({PathCommandType::Move, point, {}, {}});
}

void curveTo(KoPathShape *path, const QPointF &controlPoint1, const QPointF &controlPoint2, const QPointF &endPoint)
{
    QCOMPARE(path, recordedPath);
    pathCommands.append({PathCommandType::Curve, controlPoint1, controlPoint2, endPoint});
}

} // namespace KoCurveFitPathShapeWriter

class KoCurveFitContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyAndMinimumInputsProduceExpectedPathCommands();
    void straightSamplesProduceSingleCubicSegment();
    void errorThresholdControlsCurvedSegmentCount();
};

void KoCurveFitContractTest::emptyAndMinimumInputsProduceExpectedPathCommands()
{
    QCOMPARE(bezierFit({}, 0.1F), recordedPath);
    QVERIFY(pathCommands.isEmpty());

    QCOMPARE(bezierFit({QPointF(4.0, -2.0)}, 0.1F), recordedPath);
    QVERIFY(pathCommands.isEmpty());

    QCOMPARE(bezierFit({QPointF(0.0, 0.0), QPointF(9.0, 0.0)}, 0.1F), recordedPath);
    QCOMPARE(pathCommands.size(), 2);
    QCOMPARE(pathCommands.at(0).type, PathCommandType::Move);
    QCOMPARE(pathCommands.at(0).point1, QPointF(0.0, 0.0));
    QCOMPARE(pathCommands.at(1).type, PathCommandType::Curve);
    QVERIFY(pointsAreNear(pathCommands.at(1).point1, QPointF(3.0, 0.0)));
    QVERIFY(pointsAreNear(pathCommands.at(1).point2, QPointF(6.0, 0.0)));
    QCOMPARE(pathCommands.at(1).point3, QPointF(9.0, 0.0));
}

void KoCurveFitContractTest::straightSamplesProduceSingleCubicSegment()
{
    const QList<QPointF> points{
        QPointF(-6.0, -3.0),
        QPointF(-2.0, -1.0),
        QPointF(2.0, 1.0),
        QPointF(6.0, 3.0),
    };

    QCOMPARE(bezierFit(points, 0.01F), recordedPath);
    QCOMPARE(pathCommands.size(), 2);
    QCOMPARE(curveCount(pathCommands), 1);
    QCOMPARE(pathCommands.at(0).point1, points.first());
    QVERIFY(pointsAreNear(pathCommands.at(1).point1, QPointF(-2.0, -1.0)));
    QVERIFY(pointsAreNear(pathCommands.at(1).point2, QPointF(2.0, 1.0)));
    QCOMPARE(pathCommands.at(1).point3, points.last());
}

void KoCurveFitContractTest::errorThresholdControlsCurvedSegmentCount()
{
    const QList<QPointF> points{
        QPointF(0.0, 0.0),
        QPointF(1.0, 4.0),
        QPointF(2.0, -4.0),
        QPointF(3.0, 4.0),
        QPointF(4.0, 0.0),
    };

    QCOMPARE(bezierFit(points, 100.0F), recordedPath);
    const QVector<PathCommand> looseCommands = pathCommands;
    QCOMPARE(curveCount(looseCommands), 1);
    QCOMPARE(looseCommands.first().point1, points.first());
    QCOMPARE(looseCommands.last().point3, points.last());

    QCOMPARE(bezierFit(points, 0.05F), recordedPath);
    const QVector<PathCommand> tightCommands = pathCommands;
    QVERIFY(curveCount(tightCommands) > curveCount(looseCommands));
    QCOMPARE(tightCommands.first().point1, points.first());
    QCOMPARE(tightCommands.last().point3, points.last());

    QCOMPARE(bezierFit(points, 0.05F), recordedPath);
    QVERIFY(pathCommands == tightCommands);
}

QTEST_GUILESS_MAIN(KoCurveFitContractTest)

#include "KoCurveFitContractTest.moc"
