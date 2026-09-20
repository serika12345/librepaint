/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSet>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_model.h"
#pragma pop_macro("Q_OBJECT")

namespace
{

bool dummiesFacadeAvailable = false;
QSet<quintptr> dropEnabledItems;

} // namespace

bool KisNodeModel::ItemFlagsAccess::hasDummiesFacade(const KisNodeModel *)
{
    return dummiesFacadeAvailable;
}

bool KisNodeModel::ItemFlagsAccess::isDropEnabled(const KisNodeModel *, quintptr itemId)
{
    return dropEnabledItems.contains(itemId);
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

    QModelIndex makeIndex(int row, int column, quintptr itemId)
    {
        return createIndex(row, column, itemId);
    }
};

class KisNodeModelItemFlagsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void missingTreeOrIndexExposesDropTarget();
    void primaryColumnSupportsEditingSelectionAndDrag();
    void dropEnabledPrimaryItemAcceptsDrop();
    void cloneColumnOnlySupportsEditing();
};

void KisNodeModelItemFlagsContractTest::init()
{
    dummiesFacadeAvailable = false;
    dropEnabledItems.clear();
}

void KisNodeModelItemFlagsContractTest::missingTreeOrIndexExposesDropTarget()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex item = model.makeIndex(0, 0, 17);

    QCOMPARE(model.flags(item), Qt::ItemFlags(Qt::ItemIsDropEnabled));
    dummiesFacadeAvailable = true;
    QCOMPARE(model.flags(QModelIndex()), Qt::ItemFlags(Qt::ItemIsDropEnabled));
}

void KisNodeModelItemFlagsContractTest::primaryColumnSupportsEditingSelectionAndDrag()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    const QModelIndex item = model.makeIndex(0, 0, 17);

    QCOMPARE(model.flags(item),
             Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable));
}

void KisNodeModelItemFlagsContractTest::dropEnabledPrimaryItemAcceptsDrop()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    dropEnabledItems.insert(17);
    const QModelIndex item = model.makeIndex(0, 0, 17);

    QVERIFY(model.flags(item).testFlag(Qt::ItemIsDropEnabled));
}

void KisNodeModelItemFlagsContractTest::cloneColumnOnlySupportsEditing()
{
    TestableKisNodeModel model(nullptr);
    dummiesFacadeAvailable = true;
    dropEnabledItems.insert(17);
    const QModelIndex cloneItem = model.makeIndex(0, 1, 17);

    QCOMPARE(model.flags(cloneItem), Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable));
}

QTEST_MAIN(KisNodeModelItemFlagsContractTest)

#include "KisNodeModelItemFlagsContractTest.moc"
