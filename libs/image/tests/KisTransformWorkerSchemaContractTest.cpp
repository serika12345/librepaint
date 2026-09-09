/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cage_transform_worker.h"
#include "kis_transform_worker.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TRANSFORM_WORKER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTransformWorker::method)), signature>)
#define ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCageTransformWorker::method)), signature>)
} // namespace

class KisTransformWorkerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void mirroringSignaturesRemainStable();
    void offsetAndExecutionSignaturesRemainStable();
    void transformAndSelectionSignaturesRemainStable();
    void subpixelTranslationSignaturesRemainStable();
    void cageTypeConstructionAndLifetimeSchemaRemainStable();
    void cageSetupAndExecutionSignaturesRemainStable();
    void cageRectangleAndImageSignaturesRemainStable();
};

void KisTransformWorkerSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Worker = KisTransformWorker;

    static_assert(std::is_class_v<Worker>);
    static_assert(std::is_constructible_v<Worker,
                                          KisPaintDeviceSP,
                                          double,
                                          double,
                                          double,
                                          double,
                                          double,
                                          qreal,
                                          qreal,
                                          KoUpdaterPtr,
                                          KisFilterStrategy *>);
    static_assert(std::is_destructible_v<Worker>);

    QVERIFY(true);
}

void KisTransformWorkerSchemaContractTest::mirroringSignaturesRemainStable()
{
    ASSERT_TRANSFORM_WORKER_SIGNATURE(mirror, void (*)(KisPaintDeviceSP, qreal, Qt::Orientation));
    ASSERT_TRANSFORM_WORKER_SIGNATURE(mirrorX, void (*)(KisPaintDeviceSP));
    ASSERT_TRANSFORM_WORKER_SIGNATURE(mirrorX, void (*)(KisPaintDeviceSP, qreal));
    ASSERT_TRANSFORM_WORKER_SIGNATURE(mirrorY, void (*)(KisPaintDeviceSP));
    ASSERT_TRANSFORM_WORKER_SIGNATURE(mirrorY, void (*)(KisPaintDeviceSP, qreal));
}

void KisTransformWorkerSchemaContractTest::offsetAndExecutionSignaturesRemainStable()
{
    using Worker = KisTransformWorker;

    ASSERT_TRANSFORM_WORKER_SIGNATURE(offset, void (*)(KisPaintDeviceSP, const QPoint &, const QRect &));
    ASSERT_TRANSFORM_WORKER_SIGNATURE(run, bool (Worker::*)());
    ASSERT_TRANSFORM_WORKER_SIGNATURE(runPartial, bool (Worker::*)(const QRect &));
}

void KisTransformWorkerSchemaContractTest::transformAndSelectionSignaturesRemainStable()
{
    using Worker = KisTransformWorker;

    ASSERT_TRANSFORM_WORKER_SIGNATURE(transform, QTransform (Worker::*)() const);
    ASSERT_TRANSFORM_WORKER_SIGNATURE(transformPixelSelectionOutline, void (Worker::*)(KisPixelSelectionSP) const);
}

void KisTransformWorkerSchemaContractTest::subpixelTranslationSignaturesRemainStable()
{
    using Worker = KisTransformWorker;

    ASSERT_TRANSFORM_WORKER_SIGNATURE(forceSubPixelTranslation, bool (Worker::*)() const);
    ASSERT_TRANSFORM_WORKER_SIGNATURE(setForceSubPixelTranslation, void (Worker::*)(bool));
}

void KisTransformWorkerSchemaContractTest::cageTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Worker = KisCageTransformWorker;

    static_assert(std::is_class_v<Worker>);
    static_assert(std::is_constructible_v<Worker, const QRect &, const QVector<QPointF> &, KoUpdater *>);
    static_assert(std::is_constructible_v<Worker, const QRect &, const QVector<QPointF> &, KoUpdater *, int>);
    static_assert(
        std::is_constructible_v<Worker, const QImage &, const QPointF &, const QVector<QPointF> &, KoUpdater *>);
    static_assert(
        std::is_constructible_v<Worker, const QImage &, const QPointF &, const QVector<QPointF> &, KoUpdater *, int>);
    static_assert(std::is_destructible_v<Worker>);

    QVERIFY(true);
}

void KisTransformWorkerSchemaContractTest::cageSetupAndExecutionSignaturesRemainStable()
{
    using Worker = KisCageTransformWorker;

    ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(prepareTransform, void (Worker::*)());
    ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(setTransformedCage, void (Worker::*)(const QVector<QPointF> &));
    ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(run, void (Worker::*)(KisPaintDeviceSP, KisPaintDeviceSP));
}

void KisTransformWorkerSchemaContractTest::cageRectangleAndImageSignaturesRemainStable()
{
    using Worker = KisCageTransformWorker;

    ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(approxChangeRect, QRect (Worker::*)(const QRect &));
    ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(approxNeedRect, QRect (Worker::*)(const QRect &, const QRect &));
    ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE(runOnQImage, QImage (Worker::*)(QPointF *));
}

#undef ASSERT_TRANSFORM_WORKER_SIGNATURE
#undef ASSERT_CAGE_TRANSFORM_WORKER_SIGNATURE

QTEST_GUILESS_MAIN(KisTransformWorkerSchemaContractTest)

#include "KisTransformWorkerSchemaContractTest.moc"
