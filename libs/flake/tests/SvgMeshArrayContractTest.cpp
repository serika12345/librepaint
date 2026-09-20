/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "svg/SvgMeshArray.h"

#include <QTest>

#include <algorithm>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

namespace
{
void comparePosition(const SvgMeshPosition &actual, int row, int col, SvgMeshPatch::Type segmentType)
{
    QCOMPARE(actual.row, row);
    QCOMPARE(actual.col, col);
    QCOMPARE(actual.segmentType, segmentType);
}

QList<QPair<QString, QColor>> squareStops()
{
    return {{QStringLiteral("L 1 0"), QColor(Qt::red)},
            {QStringLiteral("L 1 1"), QColor(Qt::green)},
            {QStringLiteral("L 0 1"), QColor(Qt::blue)},
            {QStringLiteral("L 0 0"), QColor(Qt::yellow)}};
}
} // namespace

class SvgMeshArrayContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void positionAndEmptyArrayStateAreStable();
    void defaultMeshProvidesNormalizedTopology();
    void completePatchDescriptionsAreAcceptedAndInvalidCountsAreRejected();
    void connectedPathsDescribeAnInteriorCorner();
    void mutationsPropagateAcrossSharedTopology();
    void transformAndCopyKeepIndependentArrays();
};

void SvgMeshArrayContractTest::positionAndEmptyArrayStateAreStable()
{
    const SvgMeshPosition emptyPosition;
    QCOMPARE(emptyPosition.row, -1);
    QCOMPARE(emptyPosition.col, -1);
    QCOMPARE(emptyPosition.segmentType, SvgMeshPatch::Size);
    QVERIFY(!emptyPosition.isValid());

    const SvgMeshPosition position(2, 3, SvgMeshPatch::Left);
    comparePosition(position, 2, 3, SvgMeshPatch::Left);
    QVERIFY(position.isValid());

    SvgMeshArray array;
    QCOMPARE(array.numRows(), 0);
    QCOMPARE(array.numColumns(), 0);

    array.newRow();
    QCOMPARE(array.numRows(), 1);
    QCOMPARE(array.numColumns(), 0);
}

void SvgMeshArrayContractTest::defaultMeshProvidesNormalizedTopology()
{
    SvgMeshArray array;
    array.createDefaultMesh(2, 2, QColor(Qt::cyan), QSizeF(200.0, 100.0));

    QCOMPARE(array.numRows(), 2);
    QCOMPARE(array.numColumns(), 2);
    QCOMPARE(array.boundingRect(), QRectF(0.0, 0.0, 1.0, 1.0));

    const SvgMeshStop topLeft = array.getStop(SvgMeshPatch::Top, 0, 0);
    QCOMPARE(topLeft.point, QPointF(0.0, 0.0));
    QCOMPARE(topLeft.color, QColor(Qt::white));

    const SvgMeshPosition topRightPosition(0, 0, SvgMeshPatch::Right);
    const SvgMeshStop topRight = array.getStop(topRightPosition);
    QCOMPARE(topRight.point, QPointF(0.5, 0.0));
    QCOMPARE(topRight.color, QColor(Qt::cyan));

    const SvgMeshPath topPath = array.getPath(SvgMeshPatch::Top, 0, 0);
    QCOMPARE(topPath.front(), QPointF(0.0, 0.0));
    QCOMPARE(topPath.back(), QPointF(0.5, 0.0));
    QCOMPARE(array.getPath(SvgMeshPosition(0, 0, SvgMeshPatch::Top)), topPath);

    SvgMeshPatch *patch = array.getPatch(0, 0);
    QVERIFY(patch);
    QCOMPARE(patch->getStop(SvgMeshPatch::Right).point, topRight.point);
    QCOMPARE(patch->getStop(SvgMeshPatch::Right).color, topRight.color);
}

void SvgMeshArrayContractTest::completePatchDescriptionsAreAcceptedAndInvalidCountsAreRejected()
{
    SvgMeshArray array;
    array.newRow();

    QList<QPair<QString, QColor>> tooFew{{QStringLiteral("L 1 0"), QColor(Qt::red)}};
    QVERIFY(!array.addPatch(tooFew, QPointF(0.0, 0.0)));

    QList<QPair<QString, QColor>> tooMany = squareStops();
    tooMany.append({QStringLiteral("L 2 2"), QColor(Qt::black)});
    QVERIFY(!array.addPatch(tooMany, QPointF(0.0, 0.0)));
    QCOMPARE(array.numColumns(), 0);

    QVERIFY(array.addPatch(squareStops(), QPointF(0.0, 0.0)));
    QCOMPARE(array.numColumns(), 1);
    QCOMPARE(array.getStop(SvgMeshPatch::Top, 0, 0).color, QColor(Qt::red));
    QCOMPARE(array.getStop(SvgMeshPatch::Right, 0, 0).color, QColor(Qt::green));
    QCOMPARE(array.getStop(SvgMeshPatch::Bottom, 0, 0).color, QColor(Qt::blue));
    QCOMPARE(array.getStop(SvgMeshPatch::Left, 0, 0).color, QColor(Qt::yellow));
    QCOMPARE(array.boundingRect(), QRectF(0.0, 0.0, 1.0, 1.0));
}

void SvgMeshArrayContractTest::connectedPathsDescribeAnInteriorCorner()
{
    SvgMeshArray array;
    array.createDefaultMesh(2, 2, QColor(Qt::cyan), QSizeF(1.0, 1.0));

    const QVector<SvgMeshPosition> connected = array.getConnectedPaths(SvgMeshPosition(1, 1, SvgMeshPatch::Top));
    QCOMPARE(connected.size(), 5);
    comparePosition(connected[0], 1, 0, SvgMeshPatch::Top);
    comparePosition(connected[1], 0, 0, SvgMeshPatch::Right);
    comparePosition(connected[2], 0, 1, SvgMeshPatch::Left);
    comparePosition(connected[3], 1, 1, SvgMeshPatch::Left);
    comparePosition(connected[4], 1, 1, SvgMeshPatch::Top);
}

void SvgMeshArrayContractTest::mutationsPropagateAcrossSharedTopology()
{
    SvgMeshArray array;
    array.createDefaultMesh(2, 2, QColor(Qt::cyan), QSizeF(1.0, 1.0));

    const SvgMeshPath newPath{QPointF(0.0, 0.5), QPointF(0.1, 0.4), QPointF(0.4, 0.6), QPointF(0.5, 0.5)};
    array.modifyHandle(SvgMeshPosition(1, 0, SvgMeshPatch::Top), newPath);
    QCOMPARE(array.getPath(SvgMeshPatch::Top, 1, 0), newPath);

    SvgMeshPath reversed = newPath;
    std::reverse(reversed.begin(), reversed.end());
    QCOMPARE(array.getPath(SvgMeshPatch::Bottom, 0, 0), reversed);

    const SvgMeshPosition corner(1, 1, SvgMeshPatch::Top);
    const QPointF newCorner(0.6, 0.45);
    array.modifyCorner(corner, newCorner);

    const QVector<SvgMeshPosition> sharedCorners{
        SvgMeshPosition(1, 0, SvgMeshPatch::Right),
        SvgMeshPosition(0, 0, SvgMeshPatch::Bottom),
        SvgMeshPosition(0, 1, SvgMeshPatch::Left),
        corner,
    };
    for (const SvgMeshPosition &position : sharedCorners) {
        QCOMPARE(array.getStop(position).point, newCorner);
    }

    array.modifyColor(corner, QColor(Qt::magenta));
    for (const SvgMeshPosition &position : sharedCorners) {
        QCOMPARE(array.getStop(position).color, QColor(Qt::magenta));
    }
}

void SvgMeshArrayContractTest::transformAndCopyKeepIndependentArrays()
{
    SvgMeshArray original;
    original.createDefaultMesh(1, 1, QColor(Qt::cyan), QSizeF(4.0, 3.0));

    SvgMeshArray copy(original);
    QVERIFY(copy.getPatch(0, 0) != original.getPatch(0, 0));
    QCOMPARE(copy.getPath(SvgMeshPatch::Top, 0, 0), original.getPath(SvgMeshPatch::Top, 0, 0));
    QCOMPARE(copy.boundingRect(), original.boundingRect());

    copy.modifyColor(SvgMeshPosition(0, 0, SvgMeshPatch::Top), QColor(Qt::darkMagenta));
    QCOMPARE(copy.getStop(SvgMeshPatch::Top, 0, 0).color, QColor(Qt::darkMagenta));
    QCOMPARE(original.getStop(SvgMeshPatch::Top, 0, 0).color, QColor(Qt::white));

    copy.setTransform(QTransform::fromTranslate(2.0, -3.0));
    QCOMPARE(copy.boundingRect(), QRectF(2.0, -3.0, 1.0, 1.0));
    QCOMPARE(original.boundingRect(), QRectF(0.0, 0.0, 1.0, 1.0));
}

QTEST_GUILESS_MAIN(SvgMeshArrayContractTest)

#include "SvgMeshArrayContractTest.moc"
