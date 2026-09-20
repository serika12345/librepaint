/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QHash>
#include <QMimeData>
#include <QScopedPointer>
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

KisImage *imageToken(quintptr id)
{
    return token<KisImage>(id);
}

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

KisImage *imageValue = nullptr;
QHash<const KisNode *, bool> editableValues;
QList<quintptr> requestedNodeIds;
QList<const KisNode *> editableRequests;
QList<bool> visibilityChecks;
KisNodeList capturedNodes;
KisImage *capturedImage = nullptr;
bool capturedForceCopy = false;
int createMimeDataCalls = 0;

} // namespace

KisNodeSP KisNodeModel::MimeDataAccess::nodeFromIndex(const KisNodeModel *, const QModelIndex &index)
{
    requestedNodeIds.append(index.internalId());
    return nodeToken(index.internalId());
}

bool KisNodeModel::MimeDataAccess::isEditable(const KisNodeSP &node, bool checkVisibility)
{
    editableRequests.append(node.data());
    visibilityChecks.append(checkVisibility);
    return editableValues.value(node.data(), true);
}

KisImage *KisNodeModel::MimeDataAccess::image(const KisNodeModel *)
{
    return imageValue;
}

QMimeData *KisNodeModel::MimeDataAccess::createMimeData(const KisNodeList &nodes, KisImage *image, bool forceCopy)
{
    ++createMimeDataCalls;
    capturedNodes = nodes;
    capturedImage = image;
    capturedForceCopy = forceCopy;
    return new QMimeData;
}

KisNodeModel::KisNodeModel(QObject *parent, int)
    : QAbstractItemModel(parent)
    , m_d(nullptr)
{
}

KisNodeModel::~KisNodeModel() = default;

QModelIndex KisNodeModel::index(int, int, const QModelIndex &) const
{
    return {};
}

int KisNodeModel::rowCount(const QModelIndex &) const
{
    return 0;
}

int KisNodeModel::columnCount(const QModelIndex &) const
{
    return 1;
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

bool KisNodeModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

Qt::ItemFlags KisNodeModel::flags(const QModelIndex &) const
{
    return Qt::NoItemFlags;
}

QStringList KisNodeModel::mimeTypes() const
{
    return {};
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

    QModelIndex makeIndex(int row, int column, quintptr itemId)
    {
        return createIndex(row, column, itemId);
    }
};

class KisNodeModelMimeDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void primaryColumnsPreserveNodeOrderAndImage();
    void lockedPrimaryNodeForcesCopy();
};

void KisNodeModelMimeDataContractTest::init()
{
    imageValue = imageToken(9);
    editableValues.clear();
    requestedNodeIds.clear();
    editableRequests.clear();
    visibilityChecks.clear();
    capturedNodes.clear();
    capturedImage = nullptr;
    capturedForceCopy = false;
    createMimeDataCalls = 0;
}

void KisNodeModelMimeDataContractTest::primaryColumnsPreserveNodeOrderAndImage()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex first = model.makeIndex(0, 0, 3);
    const QModelIndex cloneColumn = model.makeIndex(0, 1, 4);
    const QModelIndex second = model.makeIndex(1, 0, 5);

    QScopedPointer<QMimeData> result(model.mimeData({first, cloneColumn, second}));

    QVERIFY(result);
    QCOMPARE(requestedNodeIds, QList<quintptr>({3, 5}));
    QCOMPARE(capturedNodes, KisNodeList({nodeToken(3), nodeToken(5)}));
    QCOMPARE(capturedImage, imageValue);
    QCOMPARE(createMimeDataCalls, 1);
    QVERIFY(!capturedForceCopy);
    QCOMPARE(visibilityChecks, QList<bool>({false, false}));
}

void KisNodeModelMimeDataContractTest::lockedPrimaryNodeForcesCopy()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex editable = model.makeIndex(0, 0, 3);
    const QModelIndex locked = model.makeIndex(1, 0, 5);
    editableValues.insert(nodeToken(5).data(), false);

    QScopedPointer<QMimeData> result(model.mimeData({editable, locked}));

    QVERIFY(result);
    QCOMPARE(editableRequests, QList<const KisNode *>({nodeToken(3).data(), nodeToken(5).data()}));
    QCOMPARE(createMimeDataCalls, 1);
    QVERIFY(capturedForceCopy);
}

QTEST_MAIN(KisNodeModelMimeDataContractTest)

#include "KisNodeModelMimeDataContractTest.moc"
