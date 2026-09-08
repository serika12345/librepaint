/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_transform_worker.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TRANSFORM_WORKER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTransformWorker::method)), signature>)
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

#undef ASSERT_TRANSFORM_WORKER_SIGNATURE

QTEST_GUILESS_MAIN(KisTransformWorkerSchemaContractTest)

#include "KisTransformWorkerSchemaContractTest.moc"
