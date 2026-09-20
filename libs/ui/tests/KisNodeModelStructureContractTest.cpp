/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_model.h"
#pragma pop_macro("Q_OBJECT")

namespace
{

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

bool dummiesFacadeAvailable = false;
int dummyColumnCount = 0;
int rowCountValue = 0;
QList<KisNodeDummy *> dummyFromRowValues;
QList<KisNodeDummy *> dummyFromIndexValues;
QList<KisNodeDummy *> parentDummyValues;
QList<QModelIndex> indexFromDummyValues;
QList<int> dummyFromRowRequests;
QList<KisNodeDummy *> indexFromDummyRequests;

} // namespace

bool KisNodeModel::StructureAccess::hasDummiesFacade(const KisNodeModel *)
{
    return dummiesFacadeAvailable;
}

KisNodeDummy *KisNodeModel::StructureAccess::dummyFromRow(const KisNodeModel *, int row, const QModelIndex &)
{
    dummyFromRowRequests.append(row);
    return dummyFromRowValues.isEmpty() ? nullptr : dummyFromRowValues.takeFirst();
}

KisNodeDummy *KisNodeModel::StructureAccess::dummyFromIndex(const KisNodeModel *, const QModelIndex &)
{
    return dummyFromIndexValues.isEmpty() ? nullptr : dummyFromIndexValues.takeFirst();
}

KisNodeDummy *KisNodeModel::StructureAccess::parentDummy(KisNodeDummy *)
{
    return parentDummyValues.isEmpty() ? nullptr : parentDummyValues.takeFirst();
}

QModelIndex KisNodeModel::StructureAccess::indexFromDummy(const KisNodeModel *, KisNodeDummy *dummy)
{
    indexFromDummyRequests.append(dummy);
    return indexFromDummyValues.isEmpty() ? QModelIndex() : indexFromDummyValues.takeFirst();
}

int KisNodeModel::StructureAccess::rowCount(const KisNodeModel *, const QModelIndex &)
{
    return rowCountValue;
}

int KisNodeModel::StructureAccess::dummyColumns(const KisNodeModel *)
{
    return dummyColumnCount;
}

bool KisNodeModel::StructureAccess::hasMatchingModel(const KisNodeModel *model, const QModelIndex &index)
{
    return index.model() == model;
}

KisNodeModel::KisNodeModel(QObject *parent, int)
    : QAbstractItemModel(parent)
    , m_d(nullptr)
{
}

KisNodeModel::~KisNodeModel() = default;

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

class TestableKisNodeModel : public KisNodeModel
{
public:
    using KisNodeModel::KisNodeModel;

    QModelIndex makeIndex(int row, int column, void *pointer)
    {
        return createIndex(row, column, pointer);
    }
};

class KisNodeModelStructureContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void missingFacadeSuppressesTree();
    void cloneColumnsStayOnPrimaryColumn();
    void indexMapsRowsAndCloneColumns();
    void parentMapsDummyParent();
    void siblingPreservesItemsAndMapsOtherRows();
};

void KisNodeModelStructureContractTest::init()
{
    dummiesFacadeAvailable = false;
    dummyColumnCount = 0;
    rowCountValue = 0;
    dummyFromRowValues.clear();
    dummyFromIndexValues.clear();
    parentDummyValues.clear();
    indexFromDummyValues.clear();
    dummyFromRowRequests.clear();
    indexFromDummyRequests.clear();
}

void KisNodeModelStructureContractTest::missingFacadeSuppressesTree()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex child = model.makeIndex(0, 0, token<void>(1));

    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.index(0, 0).isValid());
    QVERIFY(!model.parent(child).isValid());
    QVERIFY(dummyFromRowRequests.isEmpty());
    QVERIFY(indexFromDummyRequests.isEmpty());
}

void KisNodeModelStructureContractTest::cloneColumnsStayOnPrimaryColumn()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    dummyColumnCount = 2;
    rowCountValue = 4;
    const QModelIndex cloneColumnParent = model.makeIndex(0, 1, token<void>(1));

    QCOMPARE(model.columnCount(), 3);
    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.columnCount(cloneColumnParent), 0);
    QCOMPARE(model.rowCount(cloneColumnParent), 0);
}

void KisNodeModelStructureContractTest::indexMapsRowsAndCloneColumns()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    dummyColumnCount = 2;
    rowCountValue = 4;
    auto *dummy = token<KisNodeDummy>(1);
    auto *pointer = token<void>(2);
    dummyFromRowValues = {dummy};
    indexFromDummyValues = {model.makeIndex(2, 0, pointer)};

    const QModelIndex result = model.index(2, 1);

    QCOMPARE(result.row(), 2);
    QCOMPARE(result.column(), 1);
    QCOMPARE(result.internalPointer(), pointer);
    QCOMPARE(dummyFromRowRequests, QList<int>({2}));
    QVERIFY(!model.index(4, 0).isValid());
}

void KisNodeModelStructureContractTest::parentMapsDummyParent()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    auto *childDummy = token<KisNodeDummy>(1);
    auto *parentDummy = token<KisNodeDummy>(2);
    const QModelIndex child = model.makeIndex(3, 0, childDummy);
    const QModelIndex expectedParent = model.makeIndex(1, 0, parentDummy);
    dummyFromIndexValues = {childDummy};
    parentDummyValues = {parentDummy};
    indexFromDummyValues = {expectedParent};

    QCOMPARE(model.parent(child), expectedParent);
    QCOMPARE(indexFromDummyRequests, QList<KisNodeDummy *>({parentDummy}));
}

void KisNodeModelStructureContractTest::siblingPreservesItemsAndMapsOtherRows()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    dummyColumnCount = 1;
    rowCountValue = 5;
    auto *childDummy = token<KisNodeDummy>(1);
    auto *parentDummy = token<KisNodeDummy>(2);
    auto *targetDummy = token<KisNodeDummy>(3);
    auto *childPointer = token<void>(4);
    auto *targetPointer = token<void>(5);
    const QModelIndex child = model.makeIndex(2, 0, childPointer);
    const QModelIndex parent = model.makeIndex(0, 0, parentDummy);
    const QModelIndex target = model.makeIndex(3, 0, targetPointer);

    QCOMPARE(model.sibling(2, 0, child), child);
    const QModelIndex cloneSibling = model.sibling(2, 1, child);
    QCOMPARE(cloneSibling.row(), 2);
    QCOMPARE(cloneSibling.column(), 1);
    QCOMPARE(cloneSibling.internalPointer(), childPointer);

    dummyFromIndexValues = {childDummy};
    parentDummyValues = {parentDummy};
    dummyFromRowValues = {targetDummy};
    indexFromDummyValues = {parent, target};

    QCOMPARE(model.sibling(3, 0, child), target);
    QCOMPARE(dummyFromRowRequests, QList<int>({3}));
}

QTEST_MAIN(KisNodeModelStructureContractTest)

#include "KisNodeModelStructureContractTest.moc"
