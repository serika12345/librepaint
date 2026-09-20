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

KisModelIndexConverterBase *KisNodeModel::createIndexConverter()
{
    return nullptr;
}

class KisNodeModelTransferProtocolContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mimeTypesRemainOrdered();
    void dragAndDropAllowCopyAndMove();
};

void KisNodeModelTransferProtocolContractTest::mimeTypesRemainOrdered()
{
    KisNodeModel model(nullptr);

    const QStringList expected{
        QStringLiteral("application/x-krita-node-internal-pointer"),
        QStringLiteral("application/x-qt-image"),
        QStringLiteral("application/x-color"),
        QStringLiteral("krita/x-colorsetentry"),
    };
    QCOMPARE(model.mimeTypes(), expected);
}

void KisNodeModelTransferProtocolContractTest::dragAndDropAllowCopyAndMove()
{
    KisNodeModel model(nullptr);
    const Qt::DropActions expected = Qt::CopyAction | Qt::MoveAction;

    QCOMPARE(model.supportedDragActions(), expected);
    QCOMPARE(model.supportedDropActions(), expected);
}

QTEST_MAIN(KisNodeModelTransferProtocolContractTest)

#include "KisNodeModelTransferProtocolContractTest.moc"
