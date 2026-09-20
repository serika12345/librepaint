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

void kisSharedPtrAddReference(KisImage *)
{
}

bool kisSharedPtrRelease(KisImage *)
{
    return true;
}

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

KisImageSP imageToken(quintptr id)
{
    return KisImageSP(token<KisImage>(id));
}

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

KisNodeSP isolationRootValue;
QList<KisNodeDummy *> dummyForNodeValues;
QList<KisNodeDummy *> parentDummyValues;
QList<const KisNode *> dummyForNodeRequests;
QList<KisNodeDummy *> parentDummyRequests;

} // namespace

KisNodeSP KisNodeModel::IsolationMembershipAccess::isolationRoot(const KisImageSP &)
{
    return isolationRootValue;
}

KisNodeDummy *KisNodeModel::IsolationMembershipAccess::dummyForNode(KisDummiesFacadeBase *, const KisNodeSP &node)
{
    dummyForNodeRequests.append(node.data());
    return dummyForNodeValues.isEmpty() ? nullptr : dummyForNodeValues.takeFirst();
}

KisNodeDummy *KisNodeModel::IsolationMembershipAccess::parentDummy(KisNodeDummy *dummy)
{
    parentDummyRequests.append(dummy);
    return parentDummyValues.isEmpty() ? nullptr : parentDummyValues.takeFirst();
}

class KisNodeModelIsolationMembershipContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void noIsolationRootIncludesEveryNode();
    void isolationRootIncludesItself();
    void isolationRootIncludesDescendants();
    void isolationRootExcludesOtherBranches();
};

void KisNodeModelIsolationMembershipContractTest::init()
{
    isolationRootValue.clear();
    dummyForNodeValues.clear();
    parentDummyValues.clear();
    dummyForNodeRequests.clear();
    parentDummyRequests.clear();
}

void KisNodeModelIsolationMembershipContractTest::noIsolationRootIncludesEveryNode()
{
    const KisImageSP image = imageToken(1);
    const KisNodeSP node = nodeToken(2);
    auto *facade = token<KisDummiesFacadeBase>(3);

    QVERIFY(KisNodeModel::belongsToIsolatedGroup(image, node, facade));
    QVERIFY(dummyForNodeRequests.isEmpty());
    QVERIFY(parentDummyRequests.isEmpty());
}

void KisNodeModelIsolationMembershipContractTest::isolationRootIncludesItself()
{
    const KisImageSP image = imageToken(1);
    const KisNodeSP root = nodeToken(2);
    auto *facade = token<KisDummiesFacadeBase>(3);
    auto *rootDummy = token<KisNodeDummy>(4);
    isolationRootValue = root;
    dummyForNodeValues = {rootDummy, rootDummy};

    QVERIFY(KisNodeModel::belongsToIsolatedGroup(image, root, facade));
    QCOMPARE(dummyForNodeRequests, QList<const KisNode *>({root.data(), root.data()}));
    QVERIFY(parentDummyRequests.isEmpty());
}

void KisNodeModelIsolationMembershipContractTest::isolationRootIncludesDescendants()
{
    const KisImageSP image = imageToken(1);
    const KisNodeSP root = nodeToken(2);
    const KisNodeSP child = nodeToken(3);
    auto *facade = token<KisDummiesFacadeBase>(4);
    auto *rootDummy = token<KisNodeDummy>(5);
    auto *childDummy = token<KisNodeDummy>(6);
    isolationRootValue = root;
    dummyForNodeValues = {rootDummy, childDummy};
    parentDummyValues = {rootDummy};

    QVERIFY(KisNodeModel::belongsToIsolatedGroup(image, child, facade));
    QCOMPARE(parentDummyRequests, QList<KisNodeDummy *>({childDummy}));
}

void KisNodeModelIsolationMembershipContractTest::isolationRootExcludesOtherBranches()
{
    const KisImageSP image = imageToken(1);
    const KisNodeSP root = nodeToken(2);
    const KisNodeSP other = nodeToken(3);
    auto *facade = token<KisDummiesFacadeBase>(4);
    auto *rootDummy = token<KisNodeDummy>(5);
    auto *otherDummy = token<KisNodeDummy>(6);
    auto *otherParentDummy = token<KisNodeDummy>(7);
    isolationRootValue = root;
    dummyForNodeValues = {rootDummy, otherDummy};
    parentDummyValues = {otherParentDummy, nullptr};

    QVERIFY(!KisNodeModel::belongsToIsolatedGroup(image, other, facade));
    QCOMPARE(parentDummyRequests, QList<KisNodeDummy *>({otherDummy, otherParentDummy}));
}

QTEST_MAIN(KisNodeModelIsolationMembershipContractTest)

#include "KisNodeModelIsolationMembershipContractTest.moc"
