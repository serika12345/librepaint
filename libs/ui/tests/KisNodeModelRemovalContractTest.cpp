/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSignalSpy>
#include <QTest>

#include "nodes/kis_node_model.h"

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

namespace
{

QStringList events;
int preparationCount = 0;

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

class TestNodeModel : public KisNodeModel
{
public:
    using RemovalPlan = RemovalAccess::Plan;

    TestNodeModel()
        : KisNodeModel(nullptr)
    {
    }

    using KisNodeModel::slotBeginRemoveDummy;

    QModelIndex testIndex(int row) const
    {
        return createIndex(row, 0);
    }
};

TestNodeModel::RemovalPlan plannedRemoval;

} // namespace

void *KisNodeModel::LifecycleAccess::createPrivateState(int)
{
    return token<void>(1);
}

void KisNodeModel::LifecycleAccess::connectUpdateCompressor(KisNodeModel *, void *)
{
}

void KisNodeModel::LifecycleAccess::connectThumbnailCache(KisNodeModel *, void *)
{
}

void KisNodeModel::LifecycleAccess::destroyPrivateState(void *)
{
}

bool KisNodeModel::StructureAccess::hasDummiesFacade(const KisNodeModel *)
{
    return false;
}

KisNodeDummy *KisNodeModel::StructureAccess::dummyFromRow(const KisNodeModel *, int, const QModelIndex &)
{
    return nullptr;
}

KisNodeDummy *KisNodeModel::StructureAccess::dummyFromIndex(const KisNodeModel *, const QModelIndex &)
{
    return nullptr;
}

KisNodeDummy *KisNodeModel::StructureAccess::parentDummy(KisNodeDummy *)
{
    return nullptr;
}

QModelIndex KisNodeModel::StructureAccess::indexFromDummy(const KisNodeModel *, KisNodeDummy *)
{
    return {};
}

int KisNodeModel::StructureAccess::rowCount(const KisNodeModel *, const QModelIndex &)
{
    return 0;
}

int KisNodeModel::StructureAccess::dummyColumns(const KisNodeModel *)
{
    return 0;
}

bool KisNodeModel::StructureAccess::hasMatchingModel(const KisNodeModel *model, const QModelIndex &index)
{
    return index.model() == model;
}

KisNodeModel::RemovalAccess::Plan KisNodeModel::RemovalAccess::prepare(KisNodeModel *, KisNodeDummy *)
{
    ++preparationCount;
    events.append(QStringLiteral("prepare"));
    return plannedRemoval;
}

void KisNodeModel::RemovalAccess::disconnectDummy(KisNodeModel *, KisNodeDummy *)
{
    events.append(QStringLiteral("disconnect"));
}

void KisNodeModel::RemovalAccess::beginRemoval(KisNodeModel *, const Plan &)
{
    events.append(QStringLiteral("begin"));
}

void KisNodeModel::RemovalAccess::notifyNodeRemoved(KisNodeModel *, KisNodeDummy *)
{
    events.append(QStringLiteral("notify"));
}

QVariant KisNodeModel::data(const QModelIndex &, int) const
{
    return {};
}

Qt::ItemFlags KisNodeModel::flags(const QModelIndex &) const
{
    return Qt::NoItemFlags;
}

bool KisNodeModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

QStringList KisNodeModel::mimeTypes() const
{
    return {};
}

QMimeData *KisNodeModel::mimeData(const QModelIndexList &) const
{
    return nullptr;
}

bool KisNodeModel::dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)
{
    return false;
}

bool KisNodeModel::canDropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &) const
{
    return false;
}

Qt::DropActions KisNodeModel::supportedDragActions() const
{
    return Qt::IgnoreAction;
}

Qt::DropActions KisNodeModel::supportedDropActions() const
{
    return Qt::IgnoreAction;
}

KisModelIndexConverterBase *KisNodeModel::createIndexConverter()
{
    return nullptr;
}

void KisNodeModel::setShowGlobalSelection(bool)
{
}

void KisNodeModel::slotBeginInsertDummy(KisNodeDummy *, int, const QString &)
{
}

void KisNodeModel::slotEndInsertDummy(KisNodeDummy *)
{
}

void KisNodeModel::slotEndRemoveDummy()
{
}

void KisNodeModel::slotDummyChanged(KisNodeDummy *)
{
}

void KisNodeModel::slotIsolatedModeChanged()
{
}

void KisNodeModel::slotNodeDisplayModeChanged(bool, bool)
{
}

void KisNodeModel::processUpdateQueue()
{
}

void KisNodeModel::progressPercentageChanged(int, const KisNodeSP)
{
}

void KisNodeModel::slotLayerThumbnailUpdated(KisNodeSP)
{
}

class KisNodeModelRemovalContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void nullRemovalRequestHasNoObservableEffect();
    void hiddenDummyOnlyInvalidatesThumbnailState();
    void visibleDummyAnnouncesExactRowsBeforeRemovalBegins();
};

void KisNodeModelRemovalContractTest::init()
{
    events.clear();
    plannedRemoval = {};
    preparationCount = 0;
}

void KisNodeModelRemovalContractTest::nullRemovalRequestHasNoObservableEffect()
{
    TestNodeModel model;
    QSignalSpy spy(&model, &KisNodeModel::sigBeforeBeginRemoveRows);

    model.slotBeginRemoveDummy(nullptr);

    QCOMPARE(preparationCount, 0);
    QVERIFY(events.isEmpty());
    QCOMPARE(spy.count(), 0);
}

void KisNodeModelRemovalContractTest::hiddenDummyOnlyInvalidatesThumbnailState()
{
    TestNodeModel model;
    QSignalSpy spy(&model, &KisNodeModel::sigBeforeBeginRemoveRows);

    model.slotBeginRemoveDummy(token<KisNodeDummy>(2));

    QCOMPARE(events, QStringList({QStringLiteral("prepare"), QStringLiteral("notify")}));
    QCOMPARE(spy.count(), 0);
}

void KisNodeModelRemovalContractTest::visibleDummyAnnouncesExactRowsBeforeRemovalBegins()
{
    TestNodeModel model;
    plannedRemoval.parentIndex = model.testIndex(0);
    plannedRemoval.itemIndex = model.testIndex(2);
    QSignalSpy spy(&model, &KisNodeModel::sigBeforeBeginRemoveRows);
    connect(&model, &KisNodeModel::sigBeforeBeginRemoveRows, this, [] {
        events.append(QStringLiteral("signal"));
    });

    model.slotBeginRemoveDummy(token<KisNodeDummy>(3));

    QCOMPARE(events,
             QStringList({QStringLiteral("prepare"),
                          QStringLiteral("disconnect"),
                          QStringLiteral("signal"),
                          QStringLiteral("begin"),
                          QStringLiteral("notify")}));
    QCOMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).value<QModelIndex>(), plannedRemoval.parentIndex);
    QCOMPARE(arguments.at(1).toInt(), 2);
    QCOMPARE(arguments.at(2).toInt(), 2);
}

QTEST_GUILESS_MAIN(KisNodeModelRemovalContractTest)

#include "KisNodeModelRemovalContractTest.moc"
