/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisUpdateSelectionJobSelectionAccess_p.h"
#include "kis_update_selection_job.h"

#include <QTest>
#include <QVector>

namespace
{

struct NodeToken {
    int references = 0;
    QVector<QRect> extents;
    int extentReads = 0;
    bool overlay = false;
};

struct SelectionToken {
    int references = 0;
    NodeToken *parent = nullptr;
};

enum class OperationType {
    ResolveParent,
    ReadExtent,
    UpdateFullProjection,
    UpdateProjectionRect,
    NotifyChanged,
    CheckOverlay,
    SetDirty,
};

struct Operation {
    OperationType type;
    KisSelection *selection;
    KisNode *node;
    QRect rect;
};

QVector<Operation> operations;

KisSelection *selectionPointer(SelectionToken *token)
{
    return reinterpret_cast<KisSelection *>(token);
}

SelectionToken *selectionToken(KisSelection *selection)
{
    return reinterpret_cast<SelectionToken *>(selection);
}

KisNode *nodePointer(NodeToken *token)
{
    return reinterpret_cast<KisNode *>(token);
}

NodeToken *nodeToken(KisNode *node)
{
    return reinterpret_cast<NodeToken *>(node);
}

void compareOperation(int index,
                      OperationType type,
                      KisSelection *selection = nullptr,
                      KisNode *node = nullptr,
                      const QRect &rect = QRect())
{
    const Operation &operation = operations.at(index);
    QVERIFY(operation.type == type);
    QCOMPARE(operation.selection, selection);
    QCOMPARE(operation.node, node);
    QCOMPARE(operation.rect, rect);
}

class OtherSpontaneousJob : public KisSpontaneousJob
{
public:
    bool overrides(const KisSpontaneousJob *) override
    {
        return false;
    }

    void run() override
    {
    }

    int levelOfDetail() const override
    {
        return 0;
    }

    QString debugName() const override
    {
        return QStringLiteral("OtherSpontaneousJob");
    }
};

} // namespace

void kisSharedPtrAddReference(KisSelection *selection)
{
    ++selectionToken(selection)->references;
}

bool kisSharedPtrRelease(KisSelection *selection)
{
    --selectionToken(selection)->references;
    return true;
}

void kisSharedPtrAddReference(KisNode *node)
{
    ++nodeToken(node)->references;
}

bool kisSharedPtrRelease(KisNode *node)
{
    --nodeToken(node)->references;
    return true;
}

KisNodeSP KisUpdateSelectionJobSelectionAccess::parentNode(KisSelection *selection)
{
    NodeToken *const parent = selectionToken(selection)->parent;
    KisNode *const node = parent ? nodePointer(parent) : nullptr;
    operations.append({OperationType::ResolveParent, selection, node, QRect()});
    return node ? KisNodeSP(node) : KisNodeSP();
}

QRect KisUpdateSelectionJobSelectionAccess::extent(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    const QRect rect = token->extents.at(token->extentReads++);
    operations.append({OperationType::ReadExtent, nullptr, node, rect});
    return rect;
}

void KisUpdateSelectionJobSelectionAccess::updateProjection(KisSelection *selection)
{
    operations.append({OperationType::UpdateFullProjection, selection, nullptr, QRect()});
}

void KisUpdateSelectionJobSelectionAccess::updateProjection(KisSelection *selection, const QRect &rect)
{
    operations.append({OperationType::UpdateProjectionRect, selection, nullptr, rect});
}

void KisUpdateSelectionJobSelectionAccess::notifySelectionChanged(KisSelection *selection)
{
    operations.append({OperationType::NotifyChanged, selection, nullptr, QRect()});
}

bool KisUpdateSelectionJobSelectionAccess::isOverlayProjectionLeaf(KisNode *node)
{
    operations.append({OperationType::CheckOverlay, nullptr, node, QRect()});
    return nodeToken(node)->overlay;
}

void KisUpdateSelectionJobSelectionAccess::setDirty(KisNode *node, const QRect &rect)
{
    operations.append({OperationType::SetDirty, nullptr, node, rect});
}

class KisUpdateSelectionJobContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionRetainsSelectionAndReportsExclusiveMetadata();
    void fullUpdateWithoutParentUpdatesProjectionThenNotifies();
    void rangedUpdateOnNonOverlayParentDoesNotDirtyParent();
    void overlayParentReceivesUnionOfExtentsAfterRangedUpdate();
    void sameSelectionOverridesMergeRangesAndPreserveFullReceiver();
    void overridesRejectDifferentSelectionAndOtherTypes();
};

void KisUpdateSelectionJobContractTest::constructionRetainsSelectionAndReportsExclusiveMetadata()
{
    SelectionToken token;
    KisSelectionSP selection(selectionPointer(&token));

    {
        KisUpdateSelectionJob job(selection, QRect(3, 5, 7, 11));
        selection.clear();

        QCOMPARE(token.references, 1);
        QVERIFY(job.isExclusive());
        QCOMPARE(job.levelOfDetail(), 0);
        QCOMPARE(job.debugName(), QStringLiteral("KisUpdateSelectionJob"));
    }

    QCOMPARE(token.references, 0);
}

void KisUpdateSelectionJobContractTest::fullUpdateWithoutParentUpdatesProjectionThenNotifies()
{
    operations.clear();
    SelectionToken token;
    KisSelection *const selection = selectionPointer(&token);
    KisUpdateSelectionJob job{KisSelectionSP(selection)};

    job.run();

    QCOMPARE(operations.size(), 3);
    compareOperation(0, OperationType::ResolveParent, selection);
    compareOperation(1, OperationType::UpdateFullProjection, selection);
    compareOperation(2, OperationType::NotifyChanged, selection);
}

void KisUpdateSelectionJobContractTest::rangedUpdateOnNonOverlayParentDoesNotDirtyParent()
{
    operations.clear();
    NodeToken parent{0, {QRect(1, 2, 30, 40)}, 0, false};
    SelectionToken token{0, &parent};
    KisSelection *const selection = selectionPointer(&token);
    KisNode *const parentNode = nodePointer(&parent);
    const QRect updateRect(5, 7, 11, 13);
    KisUpdateSelectionJob job(KisSelectionSP(selection), updateRect);

    job.run();

    QCOMPARE(operations.size(), 5);
    compareOperation(0, OperationType::ResolveParent, selection, parentNode);
    compareOperation(1, OperationType::ReadExtent, nullptr, parentNode, parent.extents.at(0));
    compareOperation(2, OperationType::UpdateProjectionRect, selection, nullptr, updateRect);
    compareOperation(3, OperationType::NotifyChanged, selection);
    compareOperation(4, OperationType::CheckOverlay, nullptr, parentNode);
    QCOMPARE(parent.extentReads, 1);
    QCOMPARE(parent.references, 0);
}

void KisUpdateSelectionJobContractTest::overlayParentReceivesUnionOfExtentsAfterRangedUpdate()
{
    operations.clear();
    const QRect oldExtent(1, 2, 10, 12);
    const QRect newExtent(8, 9, 14, 16);
    NodeToken parent{0, {oldExtent, newExtent}, 0, true};
    SelectionToken token{0, &parent};
    KisSelection *const selection = selectionPointer(&token);
    KisNode *const parentNode = nodePointer(&parent);
    const QRect updateRect(21, 23, 25, 27);
    KisUpdateSelectionJob job(KisSelectionSP(selection), updateRect);

    job.run();

    QCOMPARE(operations.size(), 7);
    compareOperation(0, OperationType::ResolveParent, selection, parentNode);
    compareOperation(1, OperationType::ReadExtent, nullptr, parentNode, oldExtent);
    compareOperation(2, OperationType::UpdateProjectionRect, selection, nullptr, updateRect);
    compareOperation(3, OperationType::NotifyChanged, selection);
    compareOperation(4, OperationType::CheckOverlay, nullptr, parentNode);
    compareOperation(5, OperationType::ReadExtent, nullptr, parentNode, newExtent);
    compareOperation(6, OperationType::SetDirty, nullptr, parentNode, oldExtent | newExtent);
    QCOMPARE(parent.extentReads, 2);
    QCOMPARE(parent.references, 0);
}

void KisUpdateSelectionJobContractTest::sameSelectionOverridesMergeRangesAndPreserveFullReceiver()
{
    operations.clear();
    SelectionToken token;
    KisSelectionSP selection(selectionPointer(&token));
    KisSelection *const selectionAddress = selection.data();
    KisUpdateSelectionJob first(selection, QRect(1, 3, 5, 7));
    KisUpdateSelectionJob second(selection, QRect(4, 6, 8, 10));

    QVERIFY(first.overrides(&second));
    first.run();
    QCOMPARE(operations.size(), 3);
    compareOperation(1, OperationType::UpdateProjectionRect, selectionAddress, nullptr, QRect(1, 3, 11, 13));

    operations.clear();
    KisUpdateSelectionJob full(selection);
    QVERIFY(full.overrides(&second));
    full.run();
    QCOMPARE(operations.size(), 3);
    compareOperation(1, OperationType::UpdateFullProjection, selectionAddress);
}

void KisUpdateSelectionJobContractTest::overridesRejectDifferentSelectionAndOtherTypes()
{
    SelectionToken firstToken;
    SelectionToken secondToken;
    KisUpdateSelectionJob first(KisSelectionSP(selectionPointer(&firstToken)), QRect(1, 2, 3, 4));
    KisUpdateSelectionJob otherSelection(KisSelectionSP(selectionPointer(&secondToken)), QRect(2, 3, 4, 5));
    OtherSpontaneousJob otherType;

    QVERIFY(!first.overrides(&otherSelection));
    QVERIFY(!first.overrides(&otherType));
    QVERIFY(!first.overrides(nullptr));
}

QTEST_GUILESS_MAIN(KisUpdateSelectionJobContractTest)

#include "KisUpdateSelectionJobContractTest.moc"
