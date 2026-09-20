/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMimeData>
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

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

class TestNodeModel : public KisNodeModel
{
public:
    TestNodeModel()
        : KisNodeModel(nullptr)
    {
    }

    QModelIndex testIndex(int row, int column = 0, quintptr id = 1) const
    {
        return createIndex(row, column, token<void>(id));
    }
};

const QMimeData *dropEnabledData = nullptr;
QModelIndex parentOfRemovedNode;
int takeParentCount = 0;
QList<QModelIndex> nodeFromIndexRequests;
KisNodeSP nodeFromIndexResult;
QList<const KisNode *> indexFromNodeRequests;
QModelIndex indexFromNodeResult;
QModelIndex currentActiveNodeIndex;
QList<QModelIndex> activeNodeIndexUpdates;
QList<const KisNode *> selectionUpdates;
int dummyColumnCount = 2;
QList<QModelIndex> remainingIndexes;
QList<QVariant> remainingValues;
QList<int> remainingRoles;
bool remainingResult = false;

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

void KisNodeModel::SetDataAccess::setDropEnabled(KisNodeModel *, const QMimeData *data)
{
    dropEnabledData = data;
}

QModelIndex KisNodeModel::SetDataAccess::takeParentOfRemovedNode(KisNodeModel *)
{
    ++takeParentCount;
    return parentOfRemovedNode;
}

KisNodeSP KisNodeModel::SetDataAccess::nodeFromIndex(const KisNodeModel *, const QModelIndex &index)
{
    nodeFromIndexRequests.append(index);
    return nodeFromIndexResult;
}

QModelIndex KisNodeModel::SetDataAccess::indexFromNode(const KisNodeModel *, const KisNodeSP &node)
{
    indexFromNodeRequests.append(node.data());
    return indexFromNodeResult;
}

QModelIndex KisNodeModel::SetDataAccess::activeNodeIndex(const KisNodeModel *)
{
    return currentActiveNodeIndex;
}

void KisNodeModel::SetDataAccess::setActiveNodeIndex(KisNodeModel *, const QModelIndex &index)
{
    activeNodeIndexUpdates.append(index);
    currentActiveNodeIndex = index;
}

void KisNodeModel::SetDataAccess::setSelectionAdapterActiveNode(KisNodeModel *, const KisNodeSP &node)
{
    selectionUpdates.append(node.data());
}

int KisNodeModel::SetDataAccess::dummyColumns(const KisNodeModel *)
{
    return dummyColumnCount;
}

bool KisNodeModel::SetDataAccess::setRemainingData(KisNodeModel *,
                                                   const QModelIndex &index,
                                                   const QVariant &value,
                                                   int role)
{
    remainingIndexes.append(index);
    remainingValues.append(value);
    remainingRoles.append(role);
    return remainingResult;
}

QVariant KisNodeModel::data(const QModelIndex &, int) const
{
    return {};
}

Qt::ItemFlags KisNodeModel::flags(const QModelIndex &) const
{
    return Qt::NoItemFlags;
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

void KisNodeModel::slotBeginRemoveDummy(KisNodeDummy *)
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

class KisNodeModelSetDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void dropEnabledRoleDelegatesMimeData();
    void activeRoleUpdatesSelectionAndChangedColumns();
    void repeatedActiveRoleDoesNotRepeatNotifications();
    void alternateActiveRoleRequestsIsolation();
    void removedNodeFallbackActivatesItsParent();
    void remainingRolesPreserveDelegateResult();
};

void KisNodeModelSetDataContractTest::init()
{
    dropEnabledData = nullptr;
    parentOfRemovedNode = {};
    takeParentCount = 0;
    nodeFromIndexRequests.clear();
    nodeFromIndexResult = {};
    indexFromNodeRequests.clear();
    indexFromNodeResult = {};
    currentActiveNodeIndex = {};
    activeNodeIndexUpdates.clear();
    selectionUpdates.clear();
    dummyColumnCount = 2;
    remainingIndexes.clear();
    remainingValues.clear();
    remainingRoles.clear();
    remainingResult = false;
}

void KisNodeModelSetDataContractTest::dropEnabledRoleDelegatesMimeData()
{
    TestNodeModel model;
    QMimeData data;

    QVERIFY(model.setData({}, QVariant::fromValue(static_cast<void *>(&data)), KisNodeModel::DropEnabled));
    QCOMPARE(dropEnabledData, &data);
    QCOMPARE(takeParentCount, 0);
    QVERIFY(remainingRoles.isEmpty());
}

void KisNodeModelSetDataContractTest::activeRoleUpdatesSelectionAndChangedColumns()
{
    TestNodeModel model;
    const QModelIndex requestedIndex = model.testIndex(3, 1, 7);
    const QModelIndex mappedIndex = model.testIndex(3, 0, 7);
    const KisNodeSP node = nodeToken(4);
    nodeFromIndexResult = node;
    indexFromNodeResult = mappedIndex;
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    QVERIFY(model.setData(requestedIndex, true, KisNodeModel::ActiveRole));

    QCOMPARE(takeParentCount, 0);
    QCOMPARE(nodeFromIndexRequests, QList<QModelIndex>({requestedIndex}));
    QCOMPARE(indexFromNodeRequests, QList<const KisNode *>({node.data()}));
    QCOMPARE(activeNodeIndexUpdates, QList<QModelIndex>({mappedIndex}));
    QCOMPARE(selectionUpdates, QList<const KisNode *>({node.data()}));
    QCOMPARE(changedSpy.count(), 1);
    const QList<QVariant> arguments = changedSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<QModelIndex>(), model.testIndex(3, 0, 7));
    QCOMPARE(arguments.at(1).value<QModelIndex>(), model.testIndex(3, 2, 7));
}

void KisNodeModelSetDataContractTest::repeatedActiveRoleDoesNotRepeatNotifications()
{
    TestNodeModel model;
    const QModelIndex requestedIndex = model.testIndex(5, 0, 8);
    const QModelIndex mappedIndex = model.testIndex(5, 0, 8);
    nodeFromIndexResult = nodeToken(5);
    indexFromNodeResult = mappedIndex;
    currentActiveNodeIndex = mappedIndex;
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    QVERIFY(model.setData(requestedIndex, true, KisNodeModel::ActiveRole));

    QVERIFY(activeNodeIndexUpdates.isEmpty());
    QVERIFY(selectionUpdates.isEmpty());
    QCOMPARE(changedSpy.count(), 0);
}

void KisNodeModelSetDataContractTest::alternateActiveRoleRequestsIsolation()
{
    TestNodeModel model;
    const QModelIndex requestedIndex = model.testIndex(6, 0, 9);
    nodeFromIndexResult = nodeToken(6);
    indexFromNodeResult = requestedIndex;
    currentActiveNodeIndex = requestedIndex;
    QSignalSpy isolationSpy(&model, &KisNodeModel::toggleIsolateActiveNode);
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    QVERIFY(model.setData(requestedIndex, true, KisNodeModel::AlternateActiveRole));

    QCOMPARE(activeNodeIndexUpdates, QList<QModelIndex>({requestedIndex}));
    QCOMPARE(selectionUpdates, QList<const KisNode *>({nodeFromIndexResult.data()}));
    QCOMPARE(isolationSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 1);
}

void KisNodeModelSetDataContractTest::removedNodeFallbackActivatesItsParent()
{
    TestNodeModel model;
    const QModelIndex parentIndex = model.testIndex(2, 0, 10);
    const QModelIndex mappedIndex = model.testIndex(2, 0, 11);
    parentOfRemovedNode = parentIndex;
    nodeFromIndexResult = nodeToken(7);
    indexFromNodeResult = mappedIndex;

    QVERIFY(model.setData({}, true, KisNodeModel::ActiveRole));

    QCOMPARE(takeParentCount, 1);
    QCOMPARE(nodeFromIndexRequests, QList<QModelIndex>({parentIndex}));
    QCOMPARE(activeNodeIndexUpdates, QList<QModelIndex>({mappedIndex}));
    QCOMPARE(selectionUpdates, QList<const KisNode *>({nodeFromIndexResult.data()}));
}

void KisNodeModelSetDataContractTest::remainingRolesPreserveDelegateResult()
{
    TestNodeModel model;
    const QModelIndex index = model.testIndex(4);
    const QVariant value = QStringLiteral("value");
    remainingResult = true;

    QVERIFY(model.setData(index, value, Qt::ToolTipRole));
    QCOMPARE(remainingIndexes, QList<QModelIndex>({index}));
    QCOMPARE(remainingValues, QList<QVariant>({value}));
    QCOMPARE(remainingRoles, QList<int>({Qt::ToolTipRole}));
    QCOMPARE(takeParentCount, 0);
}

QTEST_GUILESS_MAIN(KisNodeModelSetDataContractTest)

#include "KisNodeModelSetDataContractTest.moc"
