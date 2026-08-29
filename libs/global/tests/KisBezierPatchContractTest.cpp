/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierPatch.h"

#include <QDebug>
#include <QTest>

#include <array>

namespace
{
using SampleFunction = void (KisBezierPatch::*)(QSize &, QVector<QPointF> &, QVector<QPointF> &, const QPointF &) const;

constexpr QPointF globalMappingResult(321.25, -44.5);
constexpr QPointF localMappingResult(0.375, 0.625);

std::array<QPointF, 12> globalMappingPoints;
std::array<QPointF, 12> localMappingPoints;
QPointF globalMappingInput;
QPointF localMappingInput;
int globalMappingCalls = 0;
int localMappingCalls = 0;

KisBezierPatch makeLinearPatch()
{
    KisBezierPatch patch{};
    patch.originalRect = QRectF(10.0, 20.0, 200.0, 100.0);

    patch.points[KisBezierPatch::TL] = QPointF(0.0, 0.0);
    patch.points[KisBezierPatch::TL_HC] = QPointF(40.0, 0.0);
    patch.points[KisBezierPatch::TL_VC] = QPointF(0.0, 20.0);
    patch.points[KisBezierPatch::TR] = QPointF(120.0, 0.0);
    patch.points[KisBezierPatch::TR_HC] = QPointF(80.0, 0.0);
    patch.points[KisBezierPatch::TR_VC] = QPointF(120.0, 20.0);
    patch.points[KisBezierPatch::BL] = QPointF(0.0, 60.0);
    patch.points[KisBezierPatch::BL_HC] = QPointF(40.0, 60.0);
    patch.points[KisBezierPatch::BL_VC] = QPointF(0.0, 40.0);
    patch.points[KisBezierPatch::BR] = QPointF(120.0, 60.0);
    patch.points[KisBezierPatch::BR_HC] = QPointF(80.0, 60.0);
    patch.points[KisBezierPatch::BR_VC] = QPointF(120.0, 40.0);

    return patch;
}

KisBezierPatch makeAsymmetricPatch()
{
    KisBezierPatch patch = makeLinearPatch();
    patch.points[KisBezierPatch::TL_HC] = QPointF(35.0, -20.0);
    patch.points[KisBezierPatch::TR_HC] = QPointF(85.0, 15.0);
    patch.points[KisBezierPatch::TL_VC] = QPointF(-10.0, 15.0);
    patch.points[KisBezierPatch::TR_VC] = QPointF(135.0, 5.0);
    patch.points[KisBezierPatch::BL_HC] = QPointF(25.0, 45.0);
    patch.points[KisBezierPatch::BL_VC] = QPointF(20.0, 50.0);
    patch.points[KisBezierPatch::BR_HC] = QPointF(90.0, 80.0);
    patch.points[KisBezierPatch::BR_VC] = QPointF(105.0, 70.0);
    return patch;
}

struct SampleResult {
    QSize gridSize;
    QVector<QPointF> originalPoints;
    QVector<QPointF> transformedPoints;
};

SampleResult
samplePatch(const KisBezierPatch &patch, SampleFunction function, const QPointF &step, bool prependSentinel = false)
{
    SampleResult result{QSize(99, 88), {}, {}};
    if (prependSentinel) {
        result.originalPoints.append(QPointF(-9.0, -8.0));
        result.transformedPoints.append(QPointF(-7.0, -6.0));
    }
    (patch.*function)(result.gridSize, result.originalPoints, result.transformedPoints, step);
    return result;
}

void verifyLinearGrid(const SampleResult &result)
{
    const QVector<QPointF> expectedOriginal{
        QPointF(-9.0, -8.0),
        QPointF(10.0, 20.0),
        QPointF(110.0, 20.0),
        QPointF(210.0, 20.0),
        QPointF(10.0, 70.0),
        QPointF(110.0, 70.0),
        QPointF(210.0, 70.0),
        QPointF(10.0, 120.0),
        QPointF(110.0, 120.0),
        QPointF(210.0, 120.0),
    };
    const QVector<QPointF> expectedTransformed{
        QPointF(-7.0, -6.0),
        QPointF(0.0, 0.0),
        QPointF(60.0, 0.0),
        QPointF(120.0, 0.0),
        QPointF(0.0, 30.0),
        QPointF(60.0, 30.0),
        QPointF(120.0, 30.0),
        QPointF(0.0, 60.0),
        QPointF(60.0, 60.0),
        QPointF(120.0, 60.0),
    };

    QCOMPARE(result.gridSize, QSize(3, 3));
    QCOMPARE(result.originalPoints, expectedOriginal);
    QCOMPARE(result.transformedPoints, expectedTransformed);
}

bool pointsMatch(const QPointF &actual, const QPointF &expected, qreal tolerance = 1e-9)
{
    return qAbs(actual.x() - expected.x()) <= tolerance && qAbs(actual.y() - expected.y()) <= tolerance;
}

template<typename T>
QString debugText(const T &value)
{
    QString text;
    QDebug debug(&text);
    debug.nospace() << value;
    return text;
}
} // namespace

namespace KisBezierUtils
{
QPointF calculateGlobalPos(const std::array<QPointF, 12> &points, const QPointF &localPoint)
{
    globalMappingPoints = points;
    globalMappingInput = localPoint;
    ++globalMappingCalls;
    return globalMappingResult;
}

QPointF calculateLocalPos(const std::array<QPointF, 12> &points, const QPointF &globalPoint)
{
    localMappingPoints = points;
    localMappingInput = globalPoint;
    ++localMappingCalls;
    return localMappingResult;
}
} // namespace KisBezierUtils

class KisBezierPatchContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valueInitializationAndControlPointIndicesPreserveLayout();
    void boundingRectanglesPreserveSourceAndAllDestinationControls();
    void mappingMethodsForwardControlPointsAndInput();
    void regularGridAppendsLinearSamplesInRowMajorOrder();
    void svg2GridAppendsLinearSamplesInRowMajorOrder();
    void curvedPatchAlgorithmsKeepDistinctCenterSamples();
    void requestedStepUndercountsBothGridVariants();
    void debugOutputReportsBoundsAndCornersInOrder();
};

void KisBezierPatchContractTest::valueInitializationAndControlPointIndicesPreserveLayout()
{
    const KisBezierPatch patch{};
    QVERIFY(patch.originalRect.isEmpty());
    for (const QPointF &point : patch.points) {
        QCOMPARE(point, QPointF());
    }

    const std::array<KisBezierPatch::ControlPointType, 12> types{
        KisBezierPatch::TL,
        KisBezierPatch::TL_HC,
        KisBezierPatch::TL_VC,
        KisBezierPatch::TR,
        KisBezierPatch::TR_HC,
        KisBezierPatch::TR_VC,
        KisBezierPatch::BL,
        KisBezierPatch::BL_HC,
        KisBezierPatch::BL_VC,
        KisBezierPatch::BR,
        KisBezierPatch::BR_HC,
        KisBezierPatch::BR_VC,
    };
    for (std::size_t index = 0; index < types.size(); ++index) {
        QCOMPARE(int(types[index]), int(index));
    }
}

void KisBezierPatchContractTest::boundingRectanglesPreserveSourceAndAllDestinationControls()
{
    KisBezierPatch patch = makeLinearPatch();
    patch.points[KisBezierPatch::TL_HC] = QPointF(-15.0, -25.0);
    patch.points[KisBezierPatch::BR_HC] = QPointF(145.0, 75.0);

    QCOMPARE(patch.srcBoundingRect(), patch.originalRect);
    QCOMPARE(patch.dstBoundingRect(), QRectF(-15.0, -25.0, 160.0, 100.0));
}

void KisBezierPatchContractTest::mappingMethodsForwardControlPointsAndInput()
{
    const KisBezierPatch patch = makeAsymmetricPatch();
    const QPointF localInput(0.25, 0.75);
    const QPointF globalInput(42.0, 51.0);
    globalMappingCalls = 0;
    localMappingCalls = 0;

    QCOMPARE(patch.localToGlobal(localInput), globalMappingResult);
    QCOMPARE(globalMappingCalls, 1);
    QCOMPARE(globalMappingPoints, patch.points);
    QCOMPARE(globalMappingInput, localInput);

    QCOMPARE(patch.globalToLocal(globalInput), localMappingResult);
    QCOMPARE(localMappingCalls, 1);
    QCOMPARE(localMappingPoints, patch.points);
    QCOMPARE(localMappingInput, globalInput);
}

void KisBezierPatchContractTest::regularGridAppendsLinearSamplesInRowMajorOrder()
{
    const SampleResult result =
        samplePatch(makeLinearPatch(), &KisBezierPatch::sampleRegularGrid, QPointF(50.0, 25.0), true);
    verifyLinearGrid(result);
}

void KisBezierPatchContractTest::svg2GridAppendsLinearSamplesInRowMajorOrder()
{
    const SampleResult result =
        samplePatch(makeLinearPatch(), &KisBezierPatch::sampleRegularGridSVG2, QPointF(50.0, 25.0), true);
    verifyLinearGrid(result);
}

void KisBezierPatchContractTest::curvedPatchAlgorithmsKeepDistinctCenterSamples()
{
    const KisBezierPatch patch = makeAsymmetricPatch();
    const SampleResult regular = samplePatch(patch, &KisBezierPatch::sampleRegularGrid, QPointF(40.0, 30.0));
    const SampleResult svg2 = samplePatch(patch, &KisBezierPatch::sampleRegularGridSVG2, QPointF(40.0, 30.0));

    QCOMPARE(regular.gridSize, QSize(4, 4));
    QCOMPARE(svg2.gridSize, regular.gridSize);
    QCOMPARE(regular.originalPoints, svg2.originalPoints);
    QCOMPARE(regular.transformedPoints.size(), 16);
    QCOMPARE(svg2.transformedPoints.size(), 16);
    QVERIFY(pointsMatch(regular.originalPoints[5], QPointF(73.286575296899784, 52.118544900652317)));
    QVERIFY(pointsMatch(regular.transformedPoints[5], QPointF(38.8888888888889, 15.432098765432102)));
    QVERIFY(pointsMatch(svg2.transformedPoints[5], QPointF(38.8888888888889, 15.555555555555557)));
}

void KisBezierPatchContractTest::requestedStepUndercountsBothGridVariants()
{
    const KisBezierPatch patch = makeLinearPatch();
    const QPointF requestedStep(60.0, 30.0);
    const std::array<SampleFunction, 2> functions{
        &KisBezierPatch::sampleRegularGrid,
        &KisBezierPatch::sampleRegularGridSVG2,
    };

    for (SampleFunction function : functions) {
        const SampleResult result = samplePatch(patch, function, requestedStep);
        QCOMPARE(result.gridSize, QSize(2, 2));
        QCOMPARE(result.transformedPoints.size(), 4);
        QVERIFY(result.transformedPoints[1].x() - result.transformedPoints[0].x() > requestedStep.x());
        QVERIFY(result.transformedPoints[2].y() - result.transformedPoints[0].y() > requestedStep.y());
    }
}

void KisBezierPatchContractTest::debugOutputReportsBoundsAndCornersInOrder()
{
    const KisBezierPatch patch = makeLinearPatch();
    const QString output = debugText(patch);
    const QString sourceBounds = debugText(patch.originalRect);
    const QString destinationBounds = debugText(patch.dstBoundingRect());

    QVERIFY(output.startsWith(QStringLiteral("Patch ")));
    QVERIFY(output.contains(sourceBounds));
    QVERIFY(output.contains(QStringLiteral(" -> ")));
    QVERIFY(output.contains(destinationBounds));
    QVERIFY(output.contains(QLatin1Char('\n')));

    const int topLeft = output.indexOf(debugText(patch.points[KisBezierPatch::TL]));
    const int topRight = output.indexOf(debugText(patch.points[KisBezierPatch::TR]), topLeft + 1);
    const int bottomLeft = output.indexOf(debugText(patch.points[KisBezierPatch::BL]), topRight + 1);
    const int bottomRight = output.indexOf(debugText(patch.points[KisBezierPatch::BR]), bottomLeft + 1);
    QVERIFY(topLeft >= 0);
    QVERIFY(topRight > topLeft);
    QVERIFY(bottomLeft > topRight);
    QVERIFY(bottomRight > bottomLeft);
}

QTEST_GUILESS_MAIN(KisBezierPatchContractTest)

#include "KisBezierPatchContractTest.moc"
