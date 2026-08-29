/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMimeData>
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

KisNodeDummy *validParentDummy = nullptr;
KisNodeDummy *rootDummy = nullptr;
KisNodeDummy *lastChildValue = nullptr;
KisNodeDummy *rowDummyValue = nullptr;
int rowCountValue = 0;
KisImage *contextImageValue = nullptr;
KisShapeController *contextShapeControllerValue = nullptr;
KisNodeInsertionAdapter *contextNodeInsertionAdapterValue = nullptr;
bool insertionResult = true;

QList<bool> parentValidityRequests;
QList<quintptr> parentIdRequests;
QList<KisNodeDummy *> lastChildRequests;
QList<int> rowCountRequests;
QList<int> dummyFromRowRequests;
int contextRequests = 0;
int insertionRequests = 0;
const QMimeData *capturedData = nullptr;
KisImage *capturedImage = nullptr;
KisShapeController *capturedShapeController = nullptr;
KisNodeInsertionAdapter *capturedNodeInsertionAdapter = nullptr;
KisNodeDummy *capturedParentDummy = nullptr;
KisNodeDummy *capturedAboveThisDummy = nullptr;
bool capturedCopyNode = false;

} // namespace

KisNodeDummy *KisNodeModel::DropMimeDataAccess::parentDummy(const KisNodeModel *, const QModelIndex &parent)
{
    parentValidityRequests.append(parent.isValid());
    parentIdRequests.append(parent.internalId());
    return parent.isValid() ? validParentDummy : rootDummy;
}

KisNodeDummy *KisNodeModel::DropMimeDataAccess::lastChild(KisNodeDummy *dummy)
{
    lastChildRequests.append(dummy);
    return lastChildValue;
}

int KisNodeModel::DropMimeDataAccess::rowCount(const KisNodeModel *, const QModelIndex &parent)
{
    rowCountRequests.append(parent.row());
    return rowCountValue;
}

KisNodeDummy *KisNodeModel::DropMimeDataAccess::dummyFromRow(const KisNodeModel *, int row, const QModelIndex &)
{
    dummyFromRowRequests.append(row);
    return rowDummyValue;
}

KisNodeModel::DropMimeDataAccess::Context KisNodeModel::DropMimeDataAccess::context(const KisNodeModel *)
{
    ++contextRequests;
    return {contextImageValue, contextShapeControllerValue, contextNodeInsertionAdapterValue};
}

bool KisNodeModel::DropMimeDataAccess::insertMimeLayers(const QMimeData *data,
                                                        const Context &context,
                                                        KisNodeDummy *parentDummy,
                                                        KisNodeDummy *aboveThisDummy,
                                                        bool copyNode)
{
    ++insertionRequests;
    capturedData = data;
    capturedImage = context.image;
    capturedShapeController = context.shapeController;
    capturedNodeInsertionAdapter = context.nodeInsertionAdapter;
    capturedParentDummy = parentDummy;
    capturedAboveThisDummy = aboveThisDummy;
    capturedCopyNode = copyNode;
    return insertionResult;
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

QMimeData *KisNodeModel::mimeData(const QModelIndexList &) const
{
    return nullptr;
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

class KisNodeModelDropInsertionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void validParentAppendUsesLastChildAndCopy();
    void rootAppendUsesRootWithoutLastChild();
    void boundedRowSelectsRowDummy();
    void rowAtEndUsesNullPredecessor();
};

void KisNodeModelDropInsertionContractTest::init()
{
    validParentDummy = token<KisNodeDummy>(1);
    rootDummy = token<KisNodeDummy>(2);
    lastChildValue = token<KisNodeDummy>(3);
    rowDummyValue = token<KisNodeDummy>(4);
    rowCountValue = 3;
    contextImageValue = token<KisImage>(5);
    contextShapeControllerValue = token<KisShapeController>(6);
    contextNodeInsertionAdapterValue = token<KisNodeInsertionAdapter>(7);
    insertionResult = true;
    parentValidityRequests.clear();
    parentIdRequests.clear();
    lastChildRequests.clear();
    rowCountRequests.clear();
    dummyFromRowRequests.clear();
    contextRequests = 0;
    insertionRequests = 0;
    capturedData = nullptr;
    capturedImage = nullptr;
    capturedShapeController = nullptr;
    capturedNodeInsertionAdapter = nullptr;
    capturedParentDummy = nullptr;
    capturedAboveThisDummy = nullptr;
    capturedCopyNode = false;
}

void KisNodeModelDropInsertionContractTest::validParentAppendUsesLastChildAndCopy()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex parent = model.makeIndex(4, 0, 11);
    QMimeData data;
    insertionResult = false;

    const bool result = model.dropMimeData(&data, Qt::CopyAction, -1, 19, parent);

    QVERIFY(!result);
    QCOMPARE(parentValidityRequests, QList<bool>({true}));
    QCOMPARE(parentIdRequests, QList<quintptr>({11}));
    QCOMPARE(lastChildRequests, QList<KisNodeDummy *>({validParentDummy}));
    QVERIFY(rowCountRequests.isEmpty());
    QVERIFY(dummyFromRowRequests.isEmpty());
    QCOMPARE(contextRequests, 1);
    QCOMPARE(insertionRequests, 1);
    QCOMPARE(capturedData, &data);
    QCOMPARE(capturedImage, contextImageValue);
    QCOMPARE(capturedShapeController, contextShapeControllerValue);
    QCOMPARE(capturedNodeInsertionAdapter, contextNodeInsertionAdapterValue);
    QCOMPARE(capturedParentDummy, validParentDummy);
    QCOMPARE(capturedAboveThisDummy, lastChildValue);
    QVERIFY(capturedCopyNode);
}

void KisNodeModelDropInsertionContractTest::rootAppendUsesRootWithoutLastChild()
{
    TestableKisNodeModel model(nullptr);
    QMimeData data;

    const bool result = model.dropMimeData(&data, Qt::MoveAction, -1, 0, {});

    QVERIFY(result);
    QCOMPARE(parentValidityRequests, QList<bool>({false}));
    QCOMPARE(capturedParentDummy, rootDummy);
    QCOMPARE(capturedAboveThisDummy, nullptr);
    QVERIFY(lastChildRequests.isEmpty());
    QVERIFY(!capturedCopyNode);
}

void KisNodeModelDropInsertionContractTest::boundedRowSelectsRowDummy()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex parent = model.makeIndex(4, 0, 12);
    QMimeData data;

    const bool result = model.dropMimeData(&data, Qt::LinkAction, 2, 37, parent);

    QVERIFY(result);
    QCOMPARE(rowCountRequests, QList<int>({4}));
    QCOMPARE(dummyFromRowRequests, QList<int>({2}));
    QCOMPARE(capturedParentDummy, validParentDummy);
    QCOMPARE(capturedAboveThisDummy, rowDummyValue);
    QVERIFY(lastChildRequests.isEmpty());
    QVERIFY(!capturedCopyNode);
}

void KisNodeModelDropInsertionContractTest::rowAtEndUsesNullPredecessor()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex parent = model.makeIndex(4, 0, 13);
    QMimeData data;
    rowCountValue = 2;

    const bool result = model.dropMimeData(&data, Qt::MoveAction, 2, 0, parent);

    QVERIFY(result);
    QCOMPARE(rowCountRequests, QList<int>({4}));
    QVERIFY(dummyFromRowRequests.isEmpty());
    QCOMPARE(capturedAboveThisDummy, nullptr);
}

QTEST_MAIN(KisNodeModelDropInsertionContractTest)

#include "KisNodeModelDropInsertionContractTest.moc"
