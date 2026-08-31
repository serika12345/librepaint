/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_model_index_converter_base.h"

#include <QAbstractListModel>
#include <QTest>

#include <memory>
#include <type_traits>

class KisNodeDummy
{
};

namespace
{

class IndexFactory final : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 64;
    }

    QVariant data(const QModelIndex &, int) const override
    {
        return {};
    }

    QModelIndex makeIndex(int row, void *token) const
    {
        return createIndex(row, 0, token);
    }
};

class ConverterProbe final : public KisModelIndexConverterBase
{
public:
    explicit ConverterProbe(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~ConverterProbe() override
    {
        ++*m_destructionCount;
    }

    KisNodeDummy *dummyFromRow(int row, QModelIndex parent) override
    {
        dummyFromRowRow = row;
        dummyFromRowParent = parent;
        return dummyFromRowResult;
    }

    KisNodeDummy *dummyFromIndex(QModelIndex index) override
    {
        dummyFromIndexIndex = index;
        return dummyFromIndexResult;
    }

    QModelIndex indexFromDummy(KisNodeDummy *dummy) override
    {
        indexFromDummyDummy = dummy;
        return indexFromDummyResult;
    }

    bool indexFromAddedDummy(KisNodeDummy *parentDummy,
                             int index,
                             const QString &newNodeMetaObjectType,
                             QModelIndex &parentIndex,
                             int &row) override
    {
        addedParentDummy = parentDummy;
        addedIndex = index;
        addedNodeMetaObjectType = newNodeMetaObjectType;
        parentIndex = addedParentIndexResult;
        row = addedRowResult;
        return addedResult;
    }

    int rowCount(QModelIndex parent) override
    {
        rowCountParent = parent;
        return rowCountResult;
    }

    KisNodeDummy *dummyFromRowResult = nullptr;
    int dummyFromRowRow = 0;
    QModelIndex dummyFromRowParent;

    KisNodeDummy *dummyFromIndexResult = nullptr;
    QModelIndex dummyFromIndexIndex;

    QModelIndex indexFromDummyResult;
    KisNodeDummy *indexFromDummyDummy = nullptr;

    bool addedResult = false;
    QModelIndex addedParentIndexResult;
    int addedRowResult = 0;
    KisNodeDummy *addedParentDummy = nullptr;
    int addedIndex = 0;
    QString addedNodeMetaObjectType;

    int rowCountResult = 0;
    QModelIndex rowCountParent;

private:
    int *m_destructionCount;
};

} // namespace

class KisModelIndexConverterBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abstractBasePreservesVirtualLifetime();
    void rowAndIndexRequestsPreserveInputsAndResults();
    void dummyAndParentRequestsPreserveInputsAndResults();
    void addedDummyRequestPreservesOrderedInputsAndOutputs();
};

void KisModelIndexConverterBaseContractTest::abstractBasePreservesVirtualLifetime()
{
    static_assert(std::is_abstract_v<KisModelIndexConverterBase>);
    static_assert(std::has_virtual_destructor_v<KisModelIndexConverterBase>);

    int destructionCount = 0;
    {
        std::unique_ptr<KisModelIndexConverterBase> converter(new ConverterProbe(&destructionCount));
        QCOMPARE(destructionCount, 0);
    }
    QCOMPARE(destructionCount, 1);
}

void KisModelIndexConverterBaseContractTest::rowAndIndexRequestsPreserveInputsAndResults()
{
    IndexFactory indexes;
    KisNodeDummy parentToken;
    KisNodeDummy rowResult;
    KisNodeDummy indexResult;
    const QModelIndex parentIndex = indexes.makeIndex(9, &parentToken);
    const QModelIndex requestedIndex = indexes.makeIndex(17, &indexResult);
    int destructionCount = 0;
    ConverterProbe probe(&destructionCount);
    KisModelIndexConverterBase &converter = probe;
    probe.dummyFromRowResult = &rowResult;
    probe.dummyFromIndexResult = &indexResult;

    QCOMPARE(converter.dummyFromRow(-7, parentIndex), &rowResult);
    QCOMPARE(probe.dummyFromRowRow, -7);
    QCOMPARE(probe.dummyFromRowParent, parentIndex);

    QCOMPARE(converter.dummyFromIndex(requestedIndex), &indexResult);
    QCOMPARE(probe.dummyFromIndexIndex, requestedIndex);
}

void KisModelIndexConverterBaseContractTest::dummyAndParentRequestsPreserveInputsAndResults()
{
    IndexFactory indexes;
    KisNodeDummy dummy;
    KisNodeDummy resultToken;
    KisNodeDummy parentToken;
    const QModelIndex resultIndex = indexes.makeIndex(23, &resultToken);
    const QModelIndex parentIndex = indexes.makeIndex(31, &parentToken);
    int destructionCount = 0;
    ConverterProbe probe(&destructionCount);
    KisModelIndexConverterBase &converter = probe;
    probe.indexFromDummyResult = resultIndex;
    probe.rowCountResult = 41;

    QCOMPARE(converter.indexFromDummy(&dummy), resultIndex);
    QCOMPARE(probe.indexFromDummyDummy, &dummy);

    QCOMPARE(converter.rowCount(parentIndex), 41);
    QCOMPARE(probe.rowCountParent, parentIndex);
}

void KisModelIndexConverterBaseContractTest::addedDummyRequestPreservesOrderedInputsAndOutputs()
{
    IndexFactory indexes;
    KisNodeDummy parentDummy;
    KisNodeDummy initialParentToken;
    KisNodeDummy resultParentToken;
    QModelIndex parentIndex = indexes.makeIndex(3, &initialParentToken);
    const QModelIndex expectedParentIndex = indexes.makeIndex(47, &resultParentToken);
    int row = -11;
    int destructionCount = 0;
    ConverterProbe probe(&destructionCount);
    KisModelIndexConverterBase &converter = probe;
    probe.addedResult = true;
    probe.addedParentIndexResult = expectedParentIndex;
    probe.addedRowResult = 53;
    const QString nodeType = QStringLiteral("UnicodeNode_葉_🌿");

    QVERIFY(converter.indexFromAddedDummy(&parentDummy, -29, nodeType, parentIndex, row));
    QCOMPARE(probe.addedParentDummy, &parentDummy);
    QCOMPARE(probe.addedIndex, -29);
    QCOMPARE(probe.addedNodeMetaObjectType, nodeType);
    QCOMPARE(parentIndex, expectedParentIndex);
    QCOMPARE(row, 53);
}

QTEST_GUILESS_MAIN(KisModelIndexConverterBaseContractTest)

#include "KisModelIndexConverterBaseContractTest.moc"
