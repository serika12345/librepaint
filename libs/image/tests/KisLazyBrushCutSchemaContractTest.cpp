/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lazybrush/KisWatershedWorker.h"
#include "lazybrush/kis_multiway_cut.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_SIGNATURE(type, method, ...)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)

} // namespace

class KisLazyBrushCutSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void watershedTypeConstructionAndLifetimeSchemaRemainStable();
    void watershedExecutionSignaturesRemainStable();
    void watershedTestingEdgeSignaturesRemainStable();
    void watershedTestingRemovalSignatureRemainsStable();
    void multiwayCutTypeConstructionAndLifetimeSchemaRemainStable();
    void multiwayCutOperationSignaturesRemainStable();
};

void KisLazyBrushCutSchemaContractTest::watershedTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Worker = KisWatershedWorker;

    static_assert(std::is_class_v<Worker>);
    static_assert(std::is_constructible_v<Worker, KisPaintDeviceSP, KisPaintDeviceSP, const QRect &>);
    static_assert(std::is_constructible_v<Worker, KisPaintDeviceSP, KisPaintDeviceSP, const QRect &, KoUpdater *>);
    static_assert(std::is_destructible_v<Worker>);

    QVERIFY(true);
}

void KisLazyBrushCutSchemaContractTest::watershedExecutionSignaturesRemainStable()
{
    using Worker = KisWatershedWorker;

    ASSERT_SIGNATURE(Worker, addKeyStroke, void (Worker::*)(KisPaintDeviceSP, const KoColor &));
    ASSERT_SIGNATURE(Worker, run, void (Worker::*)(qreal));
    static_assert(std::is_same_v<decltype(std::declval<Worker &>().run()), void>);
}

void KisLazyBrushCutSchemaContractTest::watershedTestingEdgeSignaturesRemainStable()
{
    using Worker = KisWatershedWorker;

    ASSERT_SIGNATURE(Worker, testingGroupPositiveEdge, int (Worker::*)(qint32, quint8));
    ASSERT_SIGNATURE(Worker, testingGroupNegativeEdge, int (Worker::*)(qint32, quint8));
    ASSERT_SIGNATURE(Worker, testingGroupForeignEdge, int (Worker::*)(qint32, quint8));
    ASSERT_SIGNATURE(Worker, testingGroupAllyEdge, int (Worker::*)(qint32, quint8));
    ASSERT_SIGNATURE(Worker, testingGroupConflicts, int (Worker::*)(qint32, quint8, qint32));
}

void KisLazyBrushCutSchemaContractTest::watershedTestingRemovalSignatureRemainsStable()
{
    using Worker = KisWatershedWorker;

    ASSERT_SIGNATURE(Worker, testingTryRemoveGroup, void (Worker::*)(qint32, quint8));
}

void KisLazyBrushCutSchemaContractTest::multiwayCutTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Cut = KisMultiwayCut;

    static_assert(std::is_class_v<Cut>);
    static_assert(std::is_constructible_v<Cut, KisPaintDeviceSP, KisPaintDeviceSP, const QRect &>);
    static_assert(std::is_destructible_v<Cut>);

    QVERIFY(true);
}

void KisLazyBrushCutSchemaContractTest::multiwayCutOperationSignaturesRemainStable()
{
    using Cut = KisMultiwayCut;

    ASSERT_SIGNATURE(Cut, addKeyStroke, void (Cut::*)(KisPaintDeviceSP, const KoColor &));
    ASSERT_SIGNATURE(Cut, run, void (Cut::*)());
    ASSERT_SIGNATURE(Cut, srcDevice, KisPaintDeviceSP (Cut::*)() const);
    ASSERT_SIGNATURE(Cut, dstDevice, KisPaintDeviceSP (Cut::*)() const);
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisLazyBrushCutSchemaContractTest)

#include "KisLazyBrushCutSchemaContractTest.moc"
