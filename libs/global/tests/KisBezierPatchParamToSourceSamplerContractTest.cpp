/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierPatchParamToSourceSampler.h"

#include <QTest>

namespace
{

KisBezierPatch makePatchWithOpposingBoundaryProgress()
{
    KisBezierPatch patch;
    patch.originalRect = QRectF(10.0, 20.0, 200.0, 100.0);

    patch.points[KisBezierPatch::TL] = QPointF(0.0, 0.0);
    patch.points[KisBezierPatch::TL_HC] = QPointF(100.0, 0.0);
    patch.points[KisBezierPatch::TL_VC] = QPointF(0.0, 100.0);

    patch.points[KisBezierPatch::TR] = QPointF(100.0, 0.0);
    patch.points[KisBezierPatch::TR_HC] = QPointF(100.0, 0.0);
    patch.points[KisBezierPatch::TR_VC] = QPointF(100.0, 0.0);

    patch.points[KisBezierPatch::BL] = QPointF(0.0, 100.0);
    patch.points[KisBezierPatch::BL_HC] = QPointF(0.0, 100.0);
    patch.points[KisBezierPatch::BL_VC] = QPointF(0.0, 100.0);

    patch.points[KisBezierPatch::BR] = QPointF(100.0, 100.0);
    patch.points[KisBezierPatch::BR_HC] = QPointF(0.0, 100.0);
    patch.points[KisBezierPatch::BR_VC] = QPointF(100.0, 0.0);

    return patch;
}

} // namespace

class KisBezierPatchParamToSourceSamplerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mapsBoundaryProgressIntoSourceRectangle();
};

void KisBezierPatchParamToSourceSamplerContractTest::mapsBoundaryProgressIntoSourceRectangle()
{
    using Sampler = KisBezierPatchParamToSourceSampler;

    const KisBezierPatch patch = makePatchWithOpposingBoundaryProgress();
    const Sampler sampler(patch);

    const Sampler::Range xRange = sampler.xRange(0.5);
    QCOMPARE(xRange.start, 35.0);
    QCOMPARE(xRange.end, 185.0);

    const Sampler::Range yRange = sampler.yRange(0.5);
    QCOMPARE(yRange.start, 32.5);
    QCOMPARE(yRange.end, 107.5);

    QCOMPARE(sampler.point(0.5, 0.5), QPointF(110.0, 70.0));
    QCOMPARE(sampler.point(QPointF(0.5, 0.5)), QPointF(110.0, 70.0));
}

QTEST_GUILESS_MAIN(KisBezierPatchParamToSourceSamplerContractTest)

#include "KisBezierPatchParamToSourceSamplerContractTest.moc"
