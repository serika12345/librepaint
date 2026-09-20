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

const QString acceptedMimeType = QStringLiteral("application/x-librepaint-node-model-contract");
Qt::DropActions acceptedDropActions = Qt::CopyAction;

} // namespace

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
    return {acceptedMimeType};
}

QMimeData *KisNodeModel::mimeData(const QModelIndexList &) const
{
    return nullptr;
}

bool KisNodeModel::dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)
{
    return false;
}

Qt::DropActions KisNodeModel::supportedDragActions() const
{
    return Qt::IgnoreAction;
}

Qt::DropActions KisNodeModel::supportedDropActions() const
{
    return acceptedDropActions;
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

class KisNodeModelDropAcceptanceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void validParentAlwaysAcceptsDrop();
    void rootUsesQtBaseAcceptance();
};

void KisNodeModelDropAcceptanceContractTest::init()
{
    acceptedDropActions = Qt::CopyAction;
}

void KisNodeModelDropAcceptanceContractTest::validParentAlwaysAcceptsDrop()
{
    TestableKisNodeModel model(nullptr);
    const QModelIndex parent = model.makeIndex(0, 0, 17);
    QMimeData unsupportedData;
    unsupportedData.setData(QStringLiteral("application/x-unsupported"), QByteArrayLiteral("payload"));

    QVERIFY(model.canDropMimeData(nullptr, Qt::IgnoreAction, -1, -1, parent));
    QVERIFY(model.canDropMimeData(&unsupportedData, Qt::MoveAction, 19, 7, parent));
}

void KisNodeModelDropAcceptanceContractTest::rootUsesQtBaseAcceptance()
{
    TestableKisNodeModel model(nullptr);
    QMimeData acceptedData;
    acceptedData.setData(acceptedMimeType, QByteArrayLiteral("payload"));
    QMimeData unsupportedData;
    unsupportedData.setData(QStringLiteral("application/x-unsupported"), QByteArrayLiteral("payload"));

    QVERIFY(model.canDropMimeData(&acceptedData, Qt::CopyAction, -1, -1, QModelIndex()));
    QVERIFY(!model.canDropMimeData(&unsupportedData, Qt::CopyAction, -1, -1, QModelIndex()));
    QVERIFY(!model.canDropMimeData(&acceptedData, Qt::MoveAction, -1, -1, QModelIndex()));
}

QTEST_MAIN(KisNodeModelDropAcceptanceContractTest)

#include "KisNodeModelDropAcceptanceContractTest.moc"
