/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#include <klocalizedstring.h>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
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

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

KisNodeSP activeNodeValue;
bool canModifyResult = false;
bool hasEditablePaintDeviceResult = false;
QString createdMaskNameValue;
KisNodeList modificationRequests;
KisNodeList editablePaintDeviceRequests;
KisNodeList maskNameNodes;
QStringList maskTypes;
QStringList defaultMaskNames;
KisNodeList splitNodes;
QStringList splitMaskNames;
QList<bool> mergeRequests;
int activeNodeRequests = 0;
int recoverableAssertionCount = 0;

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
    ++recoverableAssertionCount;
}

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeSP KisNodeManager::SplitAlphaAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

bool KisNodeManager::SplitAlphaAccess::canModifyLayer(KisNodeManager *, KisNodeSP node)
{
    modificationRequests.append(node);
    return canModifyResult;
}

bool KisNodeManager::SplitAlphaAccess::hasEditablePaintDevice(KisNodeSP node)
{
    editablePaintDeviceRequests.append(node);
    return hasEditablePaintDeviceResult;
}

QString KisNodeManager::SplitAlphaAccess::createMaskName(KisNodeManager *,
                                                         KisNodeSP node,
                                                         const QString &maskType,
                                                         const QString &defaultName)
{
    maskNameNodes.append(node);
    maskTypes.append(maskType);
    defaultMaskNames.append(defaultName);
    return createdMaskNameValue;
}

void KisNodeManager::SplitAlphaAccess::splitAlphaToMask(KisNodeSP node, const QString &maskName)
{
    splitNodes.append(node);
    splitMaskNames.append(maskName);
}

void KisNodeManager::SplitAlphaAccess::mergeTransparencyMaskAsAlpha(KisNodeManager *, bool writeToLayers)
{
    mergeRequests.append(writeToLayers);
}

class KisNodeManagerSplitAlphaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void splitRequiresModifiableNodeWithEditablePaintDevice();
    void splitUsesTransparencyMaskName();
    void mergeEntryPointsChooseWriteOrSave();
};

void KisNodeManagerSplitAlphaContractTest::init()
{
    activeNodeValue.clear();
    canModifyResult = false;
    hasEditablePaintDeviceResult = false;
    createdMaskNameValue.clear();
    modificationRequests.clear();
    editablePaintDeviceRequests.clear();
    maskNameNodes.clear();
    maskTypes.clear();
    defaultMaskNames.clear();
    splitNodes.clear();
    splitMaskNames.clear();
    mergeRequests.clear();
    activeNodeRequests = 0;
    recoverableAssertionCount = 0;
}

void KisNodeManagerSplitAlphaContractTest::splitRequiresModifiableNodeWithEditablePaintDevice()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);

    manager.slotSplitAlphaIntoMask();

    QCOMPARE(activeNodeRequests, 1);
    QCOMPARE(modificationRequests, (KisNodeList{activeNodeValue}));
    QVERIFY(editablePaintDeviceRequests.isEmpty());
    QVERIFY(maskNameNodes.isEmpty());
    QVERIFY(splitNodes.isEmpty());

    canModifyResult = true;
    manager.slotSplitAlphaIntoMask();

    QCOMPARE(editablePaintDeviceRequests, (KisNodeList{activeNodeValue}));
    QCOMPARE(recoverableAssertionCount, 1);
    QVERIFY(maskNameNodes.isEmpty());
    QVERIFY(splitNodes.isEmpty());
}

void KisNodeManagerSplitAlphaContractTest::splitUsesTransparencyMaskName()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    canModifyResult = true;
    hasEditablePaintDeviceResult = true;
    createdMaskNameValue = QStringLiteral("Transparency Mask 2");

    manager.slotSplitAlphaIntoMask();

    QCOMPARE(editablePaintDeviceRequests, (KisNodeList{activeNodeValue}));
    QCOMPARE(maskNameNodes, (KisNodeList{activeNodeValue}));
    QCOMPARE(maskTypes, (QStringList{QStringLiteral("KisTransparencyMask")}));
    QCOMPARE(defaultMaskNames, (QStringList{i18n("Transparency Mask")}));
    QCOMPARE(splitNodes, (KisNodeList{activeNodeValue}));
    QCOMPARE(splitMaskNames, (QStringList{createdMaskNameValue}));
}

void KisNodeManagerSplitAlphaContractTest::mergeEntryPointsChooseWriteOrSave()
{
    KisNodeManager manager(nullptr);

    manager.slotSplitAlphaWrite();
    manager.slotSplitAlphaSaveMerged();

    QCOMPARE(mergeRequests, (QList<bool>{true, false}));
}

QTEST_GUILESS_MAIN(KisNodeManagerSplitAlphaContractTest)

#include "KisNodeManagerSplitAlphaContractTest.moc"
