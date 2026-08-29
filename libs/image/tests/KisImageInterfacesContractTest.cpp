/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_interfaces.h"

#include <QRect>
#include <QSharedPointer>
#include <QTest>

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

namespace
{
template<typename T>
QSharedPointer<T> makeNonOwningSharedPointer(void *storage)
{
    return QSharedPointer<T>(reinterpret_cast<T *>(storage), [](T *) { });
}

class StrokesFacadeProbe final : public KisStrokesFacade
{
public:
    explicit StrokesFacadeProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~StrokesFacadeProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    KisStrokeId startStroke(KisStrokeStrategy *strokeStrategy) override
    {
        startedStrategy = strokeStrategy;
        return startResult;
    }

    void addJob(KisStrokeId id, KisStrokeJobData *data) override
    {
        addedStroke = id;
        addedData = data;
    }

    void endStroke(KisStrokeId id) override
    {
        endedStroke = id;
    }

    bool cancelStroke(KisStrokeId id) override
    {
        cancelledStroke = id;
        return cancelResult;
    }

    bool *destroyed = nullptr;
    KisStrokeStrategy *startedStrategy = nullptr;
    KisStrokeId startResult;
    KisStrokeId addedStroke;
    KisStrokeJobData *addedData = nullptr;
    KisStrokeId endedStroke;
    KisStrokeId cancelledStroke;
    bool cancelResult = false;
};

class UpdatesFacadeProbe final : public KisUpdatesFacade
{
public:
    explicit UpdatesFacadeProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~UpdatesFacadeProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    void blockUpdates() override
    {
        ++blockCalls;
    }

    void unblockUpdates() override
    {
        ++unblockCalls;
    }

    void disableUIUpdates() override
    {
        ++disableUiCalls;
    }

    QVector<QRect> enableUIUpdates() override
    {
        ++enableUiCalls;
        return enabledUiRects;
    }

    bool hasUpdatesRunning() const override
    {
        ++hasUpdatesCalls;
        return updatesRunning;
    }

    void notifyBatchUpdateStarted() override
    {
        ++batchStartedCalls;
    }

    void notifyBatchUpdateEnded() override
    {
        ++batchEndedCalls;
    }

    void notifyUIUpdateCompleted(const QRect &rc) override
    {
        ++uiCompletedCalls;
        completedRect = rc;
    }

    QRect bounds() const override
    {
        ++boundsCalls;
        return boundsResult;
    }

    void disableDirtyRequests() override
    {
        ++disableDirtyCalls;
    }

    void enableDirtyRequests() override
    {
        ++enableDirtyCalls;
    }

    void refreshGraphAsync(KisNodeSP root,
                           const QVector<QRect> &rects,
                           const QRect &cropRect,
                           KisProjectionUpdateFlags flags) override
    {
        ++refreshCalls;
        refreshedRoot = root.data();
        refreshedRects = rects;
        refreshedCropRect = cropRect;
        refreshedFlags = flags;
    }

    KisProjectionUpdatesFilterCookie addProjectionUpdatesFilter(KisProjectionUpdatesFilterSP filter) override
    {
        addedFilter = filter;
        return addedFilterCookie;
    }

    KisProjectionUpdatesFilterSP removeProjectionUpdatesFilter(KisProjectionUpdatesFilterCookie cookie) override
    {
        removedFilterCookie = cookie;
        return removedFilter;
    }

    KisProjectionUpdatesFilterCookie currentProjectionUpdatesFilter() const override
    {
        ++currentFilterCalls;
        return currentFilterCookie;
    }

    void resetRefreshObservation()
    {
        boundsCalls = 0;
        refreshCalls = 0;
        refreshedRoot = nullptr;
        refreshedRects.clear();
        refreshedCropRect = QRect();
        refreshedFlags = KisProjectionUpdateFlag::None;
    }

    bool *destroyed = nullptr;
    int blockCalls = 0;
    int unblockCalls = 0;
    int disableUiCalls = 0;
    int enableUiCalls = 0;
    mutable int hasUpdatesCalls = 0;
    int batchStartedCalls = 0;
    int batchEndedCalls = 0;
    int uiCompletedCalls = 0;
    mutable int boundsCalls = 0;
    int disableDirtyCalls = 0;
    int enableDirtyCalls = 0;
    int refreshCalls = 0;
    mutable int currentFilterCalls = 0;
    QVector<QRect> enabledUiRects;
    bool updatesRunning = false;
    QRect completedRect;
    QRect boundsResult;
    KisNode *refreshedRoot = nullptr;
    QVector<QRect> refreshedRects;
    QRect refreshedCropRect;
    KisProjectionUpdateFlags refreshedFlags;
    KisProjectionUpdatesFilterSP addedFilter;
    KisProjectionUpdatesFilterCookie addedFilterCookie = nullptr;
    KisProjectionUpdatesFilterCookie removedFilterCookie = nullptr;
    KisProjectionUpdatesFilterSP removedFilter;
    KisProjectionUpdatesFilterCookie currentFilterCookie = nullptr;
};

class ProjectionUpdateListenerProbe final : public KisProjectionUpdateListener
{
public:
    explicit ProjectionUpdateListenerProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~ProjectionUpdateListenerProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    void notifyProjectionUpdated(const QRect &rc) override
    {
        ++notificationCalls;
        notifiedRect = rc;
    }

    bool *destroyed = nullptr;
    int notificationCalls = 0;
    QRect notifiedRect;
};

class StrokeUndoFacadeProbe final : public KisStrokeUndoFacade
{
public:
    explicit StrokeUndoFacadeProbe(bool *destroyed = nullptr)
        : destroyed(destroyed)
    {
    }

    ~StrokeUndoFacadeProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    KisPostExecutionUndoAdapter *postExecutionUndoAdapter() const override
    {
        return undoAdapter;
    }

    const KUndo2Command *lastExecutedCommand() const override
    {
        return lastCommand;
    }

    bool *destroyed = nullptr;
    KisPostExecutionUndoAdapter *undoAdapter = nullptr;
    const KUndo2Command *lastCommand = nullptr;
};
} // namespace

class KisImageInterfacesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strokeFacadeDispatchesJobsAndCancellation();
    void updateFacadeDispatchesLifecycleAndFilters();
    void refreshOverloadsNormalizeRectsAndFlags();
    void projectionListenerDispatchesRectAndDestroysVirtually();
    void strokeUndoFacadeDispatchesPointersAndDestroysVirtually();
};

void KisImageInterfacesContractTest::strokeFacadeDispatchesJobsAndCancellation()
{
    alignas(void *) unsigned char strategyStorage = 0;
    alignas(void *) unsigned char dataStorage = 0;
    alignas(void *) unsigned char startedStrokeStorage = 0;
    alignas(void *) unsigned char suppliedStrokeStorage = 0;
    const QSharedPointer<KisStroke> startedStroke = makeNonOwningSharedPointer<KisStroke>(&startedStrokeStorage);
    const QSharedPointer<KisStroke> suppliedStroke = makeNonOwningSharedPointer<KisStroke>(&suppliedStrokeStorage);
    const KisStrokeId startedStrokeId(startedStroke);
    const KisStrokeId suppliedStrokeId(suppliedStroke);

    StrokesFacadeProbe probe;
    probe.startResult = startedStrokeId;
    probe.cancelResult = true;
    KisStrokesFacade *facade = &probe;

    QCOMPARE(facade->startStroke(reinterpret_cast<KisStrokeStrategy *>(&strategyStorage)).toStrongRef().data(),
             startedStroke.data());
    QCOMPARE(probe.startedStrategy, reinterpret_cast<KisStrokeStrategy *>(&strategyStorage));

    facade->addJob(suppliedStrokeId, reinterpret_cast<KisStrokeJobData *>(&dataStorage));
    QCOMPARE(probe.addedStroke.toStrongRef().data(), suppliedStroke.data());
    QCOMPARE(probe.addedData, reinterpret_cast<KisStrokeJobData *>(&dataStorage));

    facade->endStroke(suppliedStrokeId);
    QCOMPARE(probe.endedStroke.toStrongRef().data(), suppliedStroke.data());
    QVERIFY(facade->cancelStroke(suppliedStrokeId));
    QCOMPARE(probe.cancelledStroke.toStrongRef().data(), suppliedStroke.data());

    bool destroyed = false;
    facade = new StrokesFacadeProbe(&destroyed);
    delete facade;
    QVERIFY(destroyed);
}

void KisImageInterfacesContractTest::updateFacadeDispatchesLifecycleAndFilters()
{
    UpdatesFacadeProbe probe;
    probe.enabledUiRects = {QRect(1, 2, 3, 4), QRect(5, 6, 7, 8)};
    probe.updatesRunning = true;
    probe.boundsResult = QRect(10, 20, 30, 40);
    alignas(void *) unsigned char addedFilterStorage = 0;
    alignas(void *) unsigned char removedFilterStorage = 0;
    const KisProjectionUpdatesFilterSP addedFilter =
        makeNonOwningSharedPointer<KisProjectionUpdatesFilter>(&addedFilterStorage);
    const KisProjectionUpdatesFilterSP removedFilter =
        makeNonOwningSharedPointer<KisProjectionUpdatesFilter>(&removedFilterStorage);
    alignas(void *) unsigned char addedCookieStorage = 0;
    alignas(void *) unsigned char currentCookieStorage = 0;
    probe.addedFilterCookie = &addedCookieStorage;
    probe.removedFilter = removedFilter;
    probe.currentFilterCookie = &currentCookieStorage;
    KisUpdatesFacade *facade = &probe;

    facade->blockUpdates();
    facade->unblockUpdates();
    facade->disableUIUpdates();
    QCOMPARE(facade->enableUIUpdates(), probe.enabledUiRects);
    QVERIFY(facade->hasUpdatesRunning());
    facade->notifyBatchUpdateStarted();
    facade->notifyBatchUpdateEnded();
    const QRect completedRect(11, 12, 13, 14);
    facade->notifyUIUpdateCompleted(completedRect);
    QCOMPARE(facade->bounds(), probe.boundsResult);
    facade->disableDirtyRequests();
    facade->enableDirtyRequests();

    QCOMPARE(probe.blockCalls, 1);
    QCOMPARE(probe.unblockCalls, 1);
    QCOMPARE(probe.disableUiCalls, 1);
    QCOMPARE(probe.enableUiCalls, 1);
    QCOMPARE(probe.hasUpdatesCalls, 1);
    QCOMPARE(probe.batchStartedCalls, 1);
    QCOMPARE(probe.batchEndedCalls, 1);
    QCOMPARE(probe.uiCompletedCalls, 1);
    QCOMPARE(probe.completedRect, completedRect);
    QCOMPARE(probe.boundsCalls, 1);
    QCOMPARE(probe.disableDirtyCalls, 1);
    QCOMPARE(probe.enableDirtyCalls, 1);

    QCOMPARE(facade->addProjectionUpdatesFilter(addedFilter), &addedCookieStorage);
    QCOMPARE(probe.addedFilter.data(), addedFilter.data());
    QCOMPARE(facade->removeProjectionUpdatesFilter(&currentCookieStorage).data(), removedFilter.data());
    QCOMPARE(probe.removedFilterCookie, &currentCookieStorage);
    QCOMPARE(facade->currentProjectionUpdatesFilter(), &currentCookieStorage);
    QCOMPARE(probe.currentFilterCalls, 1);

    bool destroyed = false;
    facade = new UpdatesFacadeProbe(&destroyed);
    delete facade;
    QVERIFY(destroyed);
}

void KisImageInterfacesContractTest::refreshOverloadsNormalizeRectsAndFlags()
{
    UpdatesFacadeProbe probe;
    const QRect boundsRect(10, 20, 300, 400);
    probe.boundsResult = boundsRect;
    KisUpdatesFacade *facade = &probe;

    facade->refreshGraphAsync();
    QCOMPARE(probe.boundsCalls, 2);
    QCOMPARE(probe.refreshCalls, 1);
    QVERIFY(!probe.refreshedRoot);
    QCOMPARE(probe.refreshedRects, QVector<QRect>{boundsRect});
    QCOMPARE(probe.refreshedCropRect, boundsRect);
    QCOMPARE(probe.refreshedFlags, KisProjectionUpdateFlags(KisProjectionUpdateFlag::None));

    alignas(void *) unsigned char nodeStorage = 0;
    const KisNodeSP root(reinterpret_cast<KisNode *>(&nodeStorage));
    const KisProjectionUpdateFlags flags =
        KisProjectionUpdateFlag::NoFilthy | KisProjectionUpdateFlag::DontInvalidateFrames;

    probe.resetRefreshObservation();
    facade->refreshGraphAsync(root, flags);
    QCOMPARE(probe.boundsCalls, 2);
    QCOMPARE(probe.refreshCalls, 1);
    QCOMPARE(probe.refreshedRoot, root.data());
    QCOMPARE(probe.refreshedRects, QVector<QRect>{boundsRect});
    QCOMPARE(probe.refreshedCropRect, boundsRect);
    QCOMPARE(probe.refreshedFlags, flags);

    const QRect updateRect(1, 2, 30, 40);
    probe.resetRefreshObservation();
    facade->refreshGraphAsync(root, updateRect, KisProjectionUpdateFlag::NoFilthy);
    QCOMPARE(probe.boundsCalls, 1);
    QCOMPARE(probe.refreshCalls, 1);
    QCOMPARE(probe.refreshedRoot, root.data());
    QCOMPARE(probe.refreshedRects, QVector<QRect>{updateRect});
    QCOMPARE(probe.refreshedCropRect, boundsRect);
    QCOMPARE(probe.refreshedFlags, KisProjectionUpdateFlags(KisProjectionUpdateFlag::NoFilthy));

    const QRect cropRect(5, 6, 70, 80);
    probe.resetRefreshObservation();
    facade->refreshGraphAsync(root, updateRect, cropRect, KisProjectionUpdateFlag::DontInvalidateFrames);
    QCOMPARE(probe.boundsCalls, 0);
    QCOMPARE(probe.refreshCalls, 1);
    QCOMPARE(probe.refreshedRoot, root.data());
    QCOMPARE(probe.refreshedRects, QVector<QRect>{updateRect});
    QCOMPARE(probe.refreshedCropRect, cropRect);
    QCOMPARE(probe.refreshedFlags, KisProjectionUpdateFlags(KisProjectionUpdateFlag::DontInvalidateFrames));

    const QVector<QRect> updateRects{QRect(1, 1, 2, 2), QRect(3, 3, 4, 4)};
    probe.resetRefreshObservation();
    facade->refreshGraphAsync(root, updateRects, cropRect, flags);
    QCOMPARE(probe.boundsCalls, 0);
    QCOMPARE(probe.refreshCalls, 1);
    QCOMPARE(probe.refreshedRoot, root.data());
    QCOMPARE(probe.refreshedRects, updateRects);
    QCOMPARE(probe.refreshedCropRect, cropRect);
    QCOMPARE(probe.refreshedFlags, flags);
}

void KisImageInterfacesContractTest::projectionListenerDispatchesRectAndDestroysVirtually()
{
    ProjectionUpdateListenerProbe probe;
    KisProjectionUpdateListener *listener = &probe;
    const QRect updatedRect(2, 4, 6, 8);
    listener->notifyProjectionUpdated(updatedRect);
    QCOMPARE(probe.notificationCalls, 1);
    QCOMPARE(probe.notifiedRect, updatedRect);

    bool destroyed = false;
    listener = new ProjectionUpdateListenerProbe(&destroyed);
    delete listener;
    QVERIFY(destroyed);
}

void KisImageInterfacesContractTest::strokeUndoFacadeDispatchesPointersAndDestroysVirtually()
{
    alignas(void *) unsigned char adapterStorage = 0;
    alignas(void *) unsigned char commandStorage = 0;
    StrokeUndoFacadeProbe probe;
    probe.undoAdapter = reinterpret_cast<KisPostExecutionUndoAdapter *>(&adapterStorage);
    probe.lastCommand = reinterpret_cast<KUndo2Command *>(&commandStorage);
    KisStrokeUndoFacade *facade = &probe;

    QCOMPARE(facade->postExecutionUndoAdapter(), probe.undoAdapter);
    QCOMPARE(facade->lastExecutedCommand(), probe.lastCommand);

    bool destroyed = false;
    facade = new StrokeUndoFacadeProbe(&destroyed);
    delete facade;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisImageInterfacesContractTest)

#include "KisImageInterfacesContractTest.moc"
