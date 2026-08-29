/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QPointer>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_model.h"
#pragma pop_macro("Q_OBJECT")

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

int clonedColumnCount = -1;
int updateConnectionCount = 0;
int thumbnailConnectionCount = 0;
int destructionCount = 0;
void *createdPrivateState = nullptr;
void *destroyedPrivateState = nullptr;

} // namespace

void *KisNodeModel::LifecycleAccess::createPrivateState(int clonedColumns)
{
    clonedColumnCount = clonedColumns;
    createdPrivateState = token<void>(1);
    return createdPrivateState;
}

void KisNodeModel::LifecycleAccess::connectUpdateCompressor(KisNodeModel *, void *privateState)
{
    QCOMPARE(privateState, createdPrivateState);
    ++updateConnectionCount;
}

void KisNodeModel::LifecycleAccess::connectThumbnailCache(KisNodeModel *, void *privateState)
{
    QCOMPARE(privateState, createdPrivateState);
    ++thumbnailConnectionCount;
}

void KisNodeModel::LifecycleAccess::destroyPrivateState(void *privateState)
{
    destroyedPrivateState = privateState;
    ++destructionCount;
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
    return clonedColumnCount;
}

bool KisNodeModel::StructureAccess::hasMatchingModel(const KisNodeModel *model, const QModelIndex &index)
{
    return index.model() == model;
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

class KisNodeModelLifecycleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void negativeCloneColumnsNormalizeToPrimaryColumn();
    void positiveCloneColumnsExpandTreeShape();
    void destructionReleasesPrivateStateOnce();
};

void KisNodeModelLifecycleContractTest::init()
{
    clonedColumnCount = -1;
    updateConnectionCount = 0;
    thumbnailConnectionCount = 0;
    destructionCount = 0;
    createdPrivateState = nullptr;
    destroyedPrivateState = nullptr;
}

void KisNodeModelLifecycleContractTest::negativeCloneColumnsNormalizeToPrimaryColumn()
{
    QObject owner;

    {
        KisNodeModel model(&owner, -3);

        QCOMPARE(model.QObject::parent(), &owner);
        QCOMPARE(model.columnCount(), 1);
        QCOMPARE(updateConnectionCount, 1);
        QCOMPARE(thumbnailConnectionCount, 1);
    }

    QCOMPARE(destructionCount, 1);
}

void KisNodeModelLifecycleContractTest::positiveCloneColumnsExpandTreeShape()
{
    KisNodeModel model(nullptr, 3);

    QCOMPARE(model.columnCount(), 4);
}

void KisNodeModelLifecycleContractTest::destructionReleasesPrivateStateOnce()
{
    QObject owner;
    auto *model = new KisNodeModel(&owner);
    QPointer<KisNodeModel> guardedModel(model);

    QCOMPARE(owner.children(), QObjectList({model}));
    delete model;

    QVERIFY(guardedModel.isNull());
    QVERIFY(owner.children().isEmpty());
    QCOMPARE(destructionCount, 1);
    QCOMPARE(destroyedPrivateState, createdPrivateState);
}

QTEST_MAIN(KisNodeModelLifecycleContractTest)

#include "KisNodeModelLifecycleContractTest.moc"
