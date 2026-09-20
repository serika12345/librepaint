/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisAsynchronousStrokeUpdateHelper.h>

#include <kis_image_interfaces.h>
#include <kis_stroke.h>
#include <kis_stroke_strategy.h>

#include <QTest>

#include <memory>

namespace
{

class UpdateTestStrokeStrategy final : public KisStrokeStrategy
{
public:
    UpdateTestStrokeStrategy()
        : KisStrokeStrategy(QLatin1String("update-helper-test"))
    {
    }
};

class StrokesFacadeProbe final : public KisStrokesFacade
{
public:
    KisStrokeId startStroke(KisStrokeStrategy *) override
    {
        return {};
    }

    void addJob(KisStrokeId id, KisStrokeJobData *data) override
    {
        receivedIds.push_back(id);
        jobs.emplace_back(data);
    }

    void endStroke(KisStrokeId) override
    {
    }

    bool cancelStroke(KisStrokeId) override
    {
        return true;
    }

    QVector<KisStrokeId> receivedIds;
    std::vector<std::unique_ptr<KisStrokeJobData>> jobs;
};

KisStrokeId makeStrokeId(KisStrokeSP *owner)
{
    *owner = KisStrokeSP::create(new UpdateTestStrokeStrategy);
    return owner->toWeakRef();
}

bool forceUpdate(const StrokesFacadeProbe &facade, size_t index)
{
    const auto *const update = dynamic_cast<const KisAsynchronousStrokeUpdateHelper::UpdateData *>(facade.jobs.at(index).get());
    return update && update->forceUpdate;
}

} // namespace

class KisAsynchronousStrokeUpdateHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void periodicAndFinalUpdatesReachAnActiveStroke();
    void initializedTransformStrokeGetsFinalUpdateBeforeTimerStarts();
    void cancellingAStrokeStopsFurtherUpdates();
};

void KisAsynchronousStrokeUpdateHelperContractTest::periodicAndFinalUpdatesReachAnActiveStroke()
{
    // Consumer: move and freehand tools while a stroke is active.
    // Operation: start the asynchronous update stream and then finish the stroke.
    // Observable result: the active stroke receives a periodic update and one forced final update before the stream becomes inactive.
    // Failure impact: a drag or brush stroke can leave its canvas preview stale at the end of the user's gesture.
    StrokesFacadeProbe facade;
    KisStrokeSP stroke;
    KisAsynchronousStrokeUpdateHelper helper;

    helper.startUpdateStream(&facade, makeStrokeId(&stroke));
    QVERIFY(helper.isActive());
    QTRY_VERIFY_WITH_TIMEOUT(!facade.jobs.empty(), 1000);
    QVERIFY(!forceUpdate(facade, 0));
    QCOMPARE(facade.receivedIds.front().toStrongRef(), stroke);

    const size_t periodicUpdateCount = facade.jobs.size();
    helper.endUpdateStream();

    QCOMPARE(facade.jobs.size(), periodicUpdateCount + 1);
    QVERIFY(forceUpdate(facade, facade.jobs.size() - 1));
    QVERIFY(!helper.isActive());
}

void KisAsynchronousStrokeUpdateHelperContractTest::initializedTransformStrokeGetsFinalUpdateBeforeTimerStarts()
{
    // Consumer: the transform tool while its stroke initialization is still pending.
    // Operation: initialize the update stream and finish it before starting the periodic timer.
    // Observable result: the stroke receives exactly one forced final update and the helper becomes inactive.
    // Failure impact: ending a transform before initialization completes leaves the final preview or transform state unapplied.
    StrokesFacadeProbe facade;
    KisStrokeSP stroke;
    KisAsynchronousStrokeUpdateHelper helper;

    helper.initUpdateStreamLowLevel(&facade, makeStrokeId(&stroke));
    QVERIFY(helper.isActive());
    helper.endUpdateStream();

    QCOMPARE(facade.jobs.size(), size_t(1));
    QVERIFY(forceUpdate(facade, 0));
    QCOMPARE(facade.receivedIds.front().toStrongRef(), stroke);
    QVERIFY(!helper.isActive());
}

void KisAsynchronousStrokeUpdateHelperContractTest::cancellingAStrokeStopsFurtherUpdates()
{
    // Consumer: move, transform, and freehand tools when a gesture is cancelled.
    // Operation: start the update stream, wait for an update, and cancel the stroke.
    // Observable result: cancellation makes the helper inactive and no later timer update is delivered.
    // Failure impact: a cancelled gesture continues changing or refreshing the canvas after the user has abandoned it.
    StrokesFacadeProbe facade;
    KisStrokeSP stroke;
    KisAsynchronousStrokeUpdateHelper helper;

    helper.startUpdateStream(&facade, makeStrokeId(&stroke));
    QTRY_VERIFY_WITH_TIMEOUT(!facade.jobs.empty(), 1000);
    const size_t deliveredUpdateCount = facade.jobs.size();

    helper.cancelUpdateStream();
    QVERIFY(!helper.isActive());
    QTest::qWait(200);
    QCOMPARE(facade.jobs.size(), deliveredUpdateCount);
}

QTEST_GUILESS_MAIN(KisAsynchronousStrokeUpdateHelperContractTest)

#include "KisAsynchronousStrokeUpdateHelperContractTest.moc"
