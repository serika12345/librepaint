/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisAsynchronousStrokeUpdateHelper.h>

#include <kis_image_interfaces.h>

#include <QPointer>
#include <QTest>

#include <memory>
#include <type_traits>

class KisStroke
{
};

void kis_assert_exception(const char *, const char *, int)
{
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

KisStrokesFacade::~KisStrokesFacade() = default;

namespace
{
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

KisStrokeId makeStrokeId(QSharedPointer<KisStroke> *owner)
{
    *owner = QSharedPointer<KisStroke>::create();
    return owner->toWeakRef();
}

bool invokeUpdate(KisAsynchronousStrokeUpdateHelper *helper, bool forceUpdate)
{
    return QMetaObject::invokeMethod(helper, "slotAsyncUpdateCame", Qt::DirectConnection, Q_ARG(bool, forceUpdate));
}
} // namespace

class KisAsynchronousStrokeUpdateHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateDataPreservesSchedulingAndForceAcrossClone();
    void constructionStartsInactiveAndQObjectLifetimeIsVirtual();
    void lowLevelInitializationAndStartDeliverPeriodicUpdates();
    void convenienceStartAndCustomFactoryForwardBorrowedState();
    void endForcesFinalUpdateAndCancelClearsState();
};

void KisAsynchronousStrokeUpdateHelperContractTest::updateDataPreservesSchedulingAndForceAcrossClone()
{
    static_assert(
        std::is_same_v<KisAsynchronousStrokeUpdateHelper::UpdateDataFactory, std::function<KisStrokeJobData *(bool)>>);

    KisAsynchronousStrokeUpdateHelper::UpdateData defaults(false);
    QVERIFY(!defaults.forceUpdate);
    QVERIFY(defaults.isSequential());
    QVERIFY(!defaults.isExclusive());

    KisAsynchronousStrokeUpdateHelper::UpdateData configured(true,
                                                             KisStrokeJobData::BARRIER,
                                                             KisStrokeJobData::EXCLUSIVE);
    QVERIFY(configured.forceUpdate);
    QVERIFY(configured.isBarrier());
    QVERIFY(configured.isExclusive());

    std::unique_ptr<KisStrokeJobData> clone(configured.createLodClone(4));
    auto *typedClone = dynamic_cast<KisAsynchronousStrokeUpdateHelper::UpdateData *>(clone.get());
    QVERIFY(typedClone);
    QVERIFY(typedClone != &configured);
    QVERIFY(typedClone->forceUpdate);
    QVERIFY(typedClone->isBarrier());
    QVERIFY(typedClone->isExclusive());
}

void KisAsynchronousStrokeUpdateHelperContractTest::constructionStartsInactiveAndQObjectLifetimeIsVirtual()
{
    QPointer<KisAsynchronousStrokeUpdateHelper> helper = new KisAsynchronousStrokeUpdateHelper;
    QVERIFY(!helper->isActive());

    QObject *base = helper;
    delete base;
    QVERIFY(helper.isNull());
}

void KisAsynchronousStrokeUpdateHelperContractTest::lowLevelInitializationAndStartDeliverPeriodicUpdates()
{
    StrokesFacadeProbe facade;
    QSharedPointer<KisStroke> stroke;
    const KisStrokeId id = makeStrokeId(&stroke);
    KisAsynchronousStrokeUpdateHelper helper;

    helper.initUpdateStreamLowLevel(&facade, id);
    QVERIFY(helper.isActive());
    helper.startUpdateStreamLowLevel();
    QTRY_VERIFY_WITH_TIMEOUT(!facade.jobs.empty(), 1000);
    const auto *job = dynamic_cast<const KisAsynchronousStrokeUpdateHelper::UpdateData *>(facade.jobs.front().get());
    QVERIFY(job);
    QVERIFY(!job->forceUpdate);
    QCOMPARE(facade.receivedIds.front().toStrongRef(), stroke);

    helper.cancelUpdateStream();
    QVERIFY(!helper.isActive());
}

void KisAsynchronousStrokeUpdateHelperContractTest::convenienceStartAndCustomFactoryForwardBorrowedState()
{
    StrokesFacadeProbe facade;
    QSharedPointer<KisStroke> stroke;
    const KisStrokeId id = makeStrokeId(&stroke);
    KisAsynchronousStrokeUpdateHelper helper;
    QList<bool> receivedForceValues;
    helper.setCustomUpdateDataFactory([&](bool forceUpdate) {
        receivedForceValues.push_back(forceUpdate);
        return new KisAsynchronousStrokeUpdateHelper::UpdateData(forceUpdate);
    });

    helper.startUpdateStream(&facade, id);
    QVERIFY(helper.isActive());
    QVERIFY(invokeUpdate(&helper, false));
    QCOMPARE(receivedForceValues, QList<bool>{false});
    QCOMPARE(facade.jobs.size(), size_t(1));
    QCOMPARE(facade.receivedIds.front().toStrongRef(), stroke);

    helper.cancelUpdateStream();
}

void KisAsynchronousStrokeUpdateHelperContractTest::endForcesFinalUpdateAndCancelClearsState()
{
    StrokesFacadeProbe facade;
    QSharedPointer<KisStroke> stroke;
    const KisStrokeId id = makeStrokeId(&stroke);
    KisAsynchronousStrokeUpdateHelper helper;
    QList<bool> receivedForceValues;
    helper.setCustomUpdateDataFactory([&](bool forceUpdate) {
        receivedForceValues.push_back(forceUpdate);
        return new KisAsynchronousStrokeUpdateHelper::UpdateData(forceUpdate);
    });

    helper.initUpdateStreamLowLevel(&facade, id);
    helper.endUpdateStream();
    QCOMPARE(receivedForceValues, QList<bool>{true});
    QCOMPARE(facade.jobs.size(), size_t(1));
    QVERIFY(!helper.isActive());

    helper.initUpdateStreamLowLevel(&facade, id);
    helper.cancelUpdateStream();
    QCOMPARE(facade.jobs.size(), size_t(1));
    QVERIFY(!helper.isActive());
}

QTEST_GUILESS_MAIN(KisAsynchronousStrokeUpdateHelperContractTest)
#include "KisAsynchronousStrokeUpdateHelperContractTest.moc"
