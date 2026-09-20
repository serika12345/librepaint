/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

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

bool paintLayerValue;
bool containsOnionSkinValue;
bool opaqueBackgroundValue;
int warningCount;
int applyCount;
KisNodeSP appliedNodeValue;
KisImageSP appliedImageValue;
KisBaseNode::PropertyList appliedPropertiesValue;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

bool KisNodeManager::PropertyAccess::isPaintLayer(KisNodeSP)
{
    return paintLayerValue;
}

bool KisNodeManager::PropertyAccess::containsOnionSkin(const KisBaseNode::PropertyList &)
{
    return containsOnionSkinValue;
}

bool KisNodeManager::PropertyAccess::hasOpaqueBackground(KisNodeSP)
{
    return opaqueBackgroundValue;
}

void KisNodeManager::PropertyAccess::showOnionSkinTransparencyWarning(const KisNodeManager *)
{
    warningCount++;
}

void KisNodeManager::PropertyAccess::applyProperties(KisNodeSP node,
                                                     KisImageSP image,
                                                     KisBaseNode::PropertyList properties)
{
    applyCount++;
    appliedNodeValue = node;
    appliedImageValue = image;
    appliedPropertiesValue = properties;
}

class KisNodeManagerPropertiesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void onionSkinRequiresTransparentBackgroundBeforeUndo();
};

void KisNodeManagerPropertiesContractTest::onionSkinRequiresTransparentBackgroundBeforeUndo()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node(token<KisNode>(1));
    const KisImageSP image;
    KisBaseNode::Property property;
    property.name = QStringLiteral("onion-skin");
    property.state = true;
    const KisBaseNode::PropertyList properties{property};

    struct Scenario {
        bool isPaintLayer;
        bool containsOnionSkin;
        bool hasOpaqueBackground;
        bool isRejected;
    };
    const Scenario scenarios[]{
        {true, true, true, true},
        {false, true, true, false},
        {true, false, true, false},
        {true, true, false, false},
    };

    for (const Scenario &scenario : scenarios) {
        paintLayerValue = scenario.isPaintLayer;
        containsOnionSkinValue = scenario.containsOnionSkin;
        opaqueBackgroundValue = scenario.hasOpaqueBackground;
        warningCount = 0;
        applyCount = 0;
        appliedNodeValue.clear();
        appliedImageValue.clear();
        appliedPropertiesValue.clear();

        QCOMPARE(manager.trySetNodeProperties(node, image, properties), !scenario.isRejected);
        QCOMPARE(warningCount, scenario.isRejected ? 1 : 0);
        QCOMPARE(applyCount, scenario.isRejected ? 0 : 1);
        if (!scenario.isRejected) {
            QCOMPARE(appliedNodeValue, node);
            QVERIFY(appliedImageValue.isNull());
            QCOMPARE(appliedPropertiesValue.size(), properties.size());
            QCOMPARE(appliedPropertiesValue.constFirst().name, properties.constFirst().name);
            QCOMPARE(appliedPropertiesValue.constFirst().state, properties.constFirst().state);
        }
    }
}

QTEST_GUILESS_MAIN(KisNodeManagerPropertiesContractTest)

#include "KisNodeManagerPropertiesContractTest.moc"
