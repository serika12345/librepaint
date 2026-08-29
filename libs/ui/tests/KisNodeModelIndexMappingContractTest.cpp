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

#include "kis_shared_ptr.h"

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

KisNodeDummy *dummyFromIndexValue = nullptr;
KisNodeDummy *dummyForNodeValue = nullptr;
KisNodeSP nodeForDummyValue;
QModelIndex indexFromDummyValue;
QList<KisNodeDummy *> nodeForDummyRequests;
QList<const KisNode *> dummyForNodeRequests;
QList<KisNodeDummy *> indexFromDummyRequests;

} // namespace

KisNodeDummy *KisNodeModel::IndexMappingAccess::dummyFromIndex(const KisNodeModel *, const QModelIndex &)
{
    return dummyFromIndexValue;
}

KisNodeSP KisNodeModel::IndexMappingAccess::nodeForDummy(KisNodeDummy *dummy)
{
    nodeForDummyRequests.append(dummy);
    return nodeForDummyValue;
}

KisNodeDummy *KisNodeModel::IndexMappingAccess::dummyForNode(const KisNodeModel *, KisNodeSP node)
{
    dummyForNodeRequests.append(node.data());
    return dummyForNodeValue;
}

QModelIndex KisNodeModel::IndexMappingAccess::indexFromDummy(const KisNodeModel *, KisNodeDummy *dummy)
{
    indexFromDummyRequests.append(dummy);
    return indexFromDummyValue;
}

KisNodeModel::KisNodeModel(QObject *parent, int)
    : QAbstractItemModel(parent)
    , m_d(nullptr)
{
}

KisNodeModel::~KisNodeModel() = default;

int KisNodeModel::rowCount(const QModelIndex &) const
{
    return 0;
}

int KisNodeModel::columnCount(const QModelIndex &) const
{
    return 0;
}

QModelIndex KisNodeModel::index(int, int, const QModelIndex &) const
{
    return {};
}

QModelIndex KisNodeModel::parent(const QModelIndex &) const
{
    return {};
}

QModelIndex KisNodeModel::sibling(int, int, const QModelIndex &) const
{
    return {};
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

class TestableKisNodeModel : public KisNodeModel
{
public:
    using KisNodeModel::KisNodeModel;

    QModelIndex makeIndex(int row, int column, void *pointer)
    {
        return createIndex(row, column, pointer);
    }
};

class KisNodeModelIndexMappingContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void indexWithoutDummyReturnsNullNode();
    void indexWithDummyReturnsMappedNode();
    void nodeWithoutDummyReturnsInvalidIndex();
    void nodeWithDummyReturnsMappedIndex();
};

void KisNodeModelIndexMappingContractTest::init()
{
    dummyFromIndexValue = nullptr;
    dummyForNodeValue = nullptr;
    nodeForDummyValue.clear();
    indexFromDummyValue = {};
    nodeForDummyRequests.clear();
    dummyForNodeRequests.clear();
    indexFromDummyRequests.clear();
}

void KisNodeModelIndexMappingContractTest::indexWithoutDummyReturnsNullNode()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex index = model.makeIndex(2, 0, token<void>(1));

    QVERIFY(!model.nodeFromIndex(index));
    QVERIFY(nodeForDummyRequests.isEmpty());
}

void KisNodeModelIndexMappingContractTest::indexWithDummyReturnsMappedNode()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex index = model.makeIndex(2, 0, token<void>(1));
    dummyFromIndexValue = token<KisNodeDummy>(2);
    nodeForDummyValue = nodeToken(3);

    QCOMPARE(model.nodeFromIndex(index), nodeForDummyValue);
    QCOMPARE(nodeForDummyRequests, QList<KisNodeDummy *>({dummyFromIndexValue}));
}

void KisNodeModelIndexMappingContractTest::nodeWithoutDummyReturnsInvalidIndex()
{
    TestableKisNodeModel model(nullptr);
    const KisNodeSP node = nodeToken(4);

    QVERIFY(!model.indexFromNode(node).isValid());
    QCOMPARE(dummyForNodeRequests, QList<const KisNode *>({node.data()}));
    QVERIFY(indexFromDummyRequests.isEmpty());
}

void KisNodeModelIndexMappingContractTest::nodeWithDummyReturnsMappedIndex()
{
    TestableKisNodeModel model(nullptr);
    const KisNodeSP node = nodeToken(5);
    dummyForNodeValue = token<KisNodeDummy>(6);
    indexFromDummyValue = model.makeIndex(3, 1, dummyForNodeValue);

    QCOMPARE(model.indexFromNode(node), indexFromDummyValue);
    QCOMPARE(dummyForNodeRequests, QList<const KisNode *>({node.data()}));
    QCOMPARE(indexFromDummyRequests, QList<KisNodeDummy *>({dummyForNodeValue}));
}

QTEST_MAIN(KisNodeModelIndexMappingContractTest)

#include "KisNodeModelIndexMappingContractTest.moc"
