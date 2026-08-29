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

namespace
{

bool deferNodeCreationValue = false;
QStringList actionRoutes;
QStringList checkableActions;
QStringList shapeLayerOnlyActions;
QStringList storedPinActions;
QStringList nodeCreationMappings;
QStringList nodeConversionMappings;
QList<bool> nodeCreationConnections;
int nodeConversionConnections = 0;
int nodeActivationIsolationConnections = 0;
QStringList effects;

QString actionRoute(const char *actionId, const char *signal, const char *slot)
{
    return QStringLiteral("%1|%2|%3")
        .arg(QString::fromLatin1(actionId), QString::fromLatin1(signal), QString::fromLatin1(slot));
}

QString typeMapping(const char *actionId, const char *nodeType)
{
    return QStringLiteral("%1|%2").arg(QString::fromLatin1(actionId), QString::fromLatin1(nodeType));
}

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

void KisNodeManager::SetupAccess::setupLayerManager(KisNodeManager *, KisActionManager *)
{
    effects.append(QStringLiteral("setup-layer"));
}

void KisNodeManager::SetupAccess::setupMaskManager(KisNodeManager *, KisKActionCollection *, KisActionManager *)
{
    effects.append(QStringLiteral("setup-mask"));
}

void KisNodeManager::SetupAccess::registerAction(KisNodeManager *,
                                                 KisActionManager *,
                                                 const char *actionId,
                                                 const char *signal,
                                                 const char *slot,
                                                 bool checkable,
                                                 bool shapeLayerOnly,
                                                 bool storePinAction)
{
    const QString id = QString::fromLatin1(actionId);
    actionRoutes.append(actionRoute(actionId, signal, slot));
    if (checkable) {
        checkableActions.append(id);
    }
    if (shapeLayerOnly) {
        shapeLayerOnlyActions.append(id);
    }
    if (storePinAction) {
        storedPinActions.append(id);
    }
    effects.append(QStringLiteral("action:%1").arg(id));
}

void KisNodeManager::SetupAccess::registerNodeCreation(KisNodeManager *,
                                                       KisActionManager *,
                                                       const char *actionId,
                                                       const char *nodeType)
{
    nodeCreationMappings.append(typeMapping(actionId, nodeType));
    effects.append(QStringLiteral("creation:%1").arg(QString::fromLatin1(actionId)));
}

bool KisNodeManager::SetupAccess::deferNodeCreation()
{
    return deferNodeCreationValue;
}

void KisNodeManager::SetupAccess::connectNodeCreation(KisNodeManager *, bool deferred)
{
    nodeCreationConnections.append(deferred);
    effects.append(deferred ? QStringLiteral("connect-creation:deferred") : QStringLiteral("connect-creation:direct"));
}

void KisNodeManager::SetupAccess::registerNodeConversion(KisNodeManager *,
                                                         KisActionManager *,
                                                         const char *actionId,
                                                         const char *nodeType,
                                                         const QStringList &excludedNodeTypes)
{
    nodeConversionMappings.append(QStringLiteral("%1|%2|%3")
                                      .arg(QString::fromLatin1(actionId),
                                           QString::fromLatin1(nodeType),
                                           excludedNodeTypes.join(QLatin1Char(','))));
    effects.append(QStringLiteral("conversion:%1").arg(QString::fromLatin1(actionId)));
}

void KisNodeManager::SetupAccess::connectNodeConversion(KisNodeManager *)
{
    ++nodeConversionConnections;
    effects.append(QStringLiteral("connect-conversion"));
}

void KisNodeManager::SetupAccess::connectNodeActivationToIsolation(KisNodeManager *)
{
    ++nodeActivationIsolationConnections;
    effects.append(QStringLiteral("connect-isolation"));
}

class KisNodeManagerSetupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void ownerSetupPrecedesActionRegistration();
    void commandActionsPreserveRoutesAndFlags();
    void nodeCreationMappingsPreserveDelivery_data();
    void nodeCreationMappingsPreserveDelivery();
    void nodeConversionsPreserveExclusions();
    void mapperAndIsolationConnectionsFollowRegistration();
};

void KisNodeManagerSetupContractTest::init()
{
    deferNodeCreationValue = false;
    actionRoutes.clear();
    checkableActions.clear();
    shapeLayerOnlyActions.clear();
    storedPinActions.clear();
    nodeCreationMappings.clear();
    nodeConversionMappings.clear();
    nodeCreationConnections.clear();
    nodeConversionConnections = 0;
    nodeActivationIsolationConnections = 0;
    effects.clear();
}

void KisNodeManagerSetupContractTest::ownerSetupPrecedesActionRegistration()
{
    KisNodeManager manager(nullptr);

    manager.setup(reinterpret_cast<KisKActionCollection *>(1), reinterpret_cast<KisActionManager *>(2));

    QCOMPARE(effects.at(0), QStringLiteral("setup-layer"));
    QCOMPARE(effects.at(1), QStringLiteral("setup-mask"));
    QCOMPARE(effects.at(2), QStringLiteral("action:mirrorNodeX"));
}

void KisNodeManagerSetupContractTest::commandActionsPreserveRoutesAndFlags()
{
    KisNodeManager manager(nullptr);

    manager.setup(reinterpret_cast<KisKActionCollection *>(1), reinterpret_cast<KisActionManager *>(2));

    const QStringList expectedRoutes{
        actionRoute("mirrorNodeX", "triggered()", "mirrorNodeX()"),
        actionRoute("mirrorNodeY", "triggered()", "mirrorNodeY()"),
        actionRoute("mirrorAllNodesX", "triggered()", "mirrorAllNodesX()"),
        actionRoute("mirrorAllNodesY", "triggered()", "mirrorAllNodesY()"),
        actionRoute("activateNextLayer", "triggered()", "activateNextNode()"),
        actionRoute("activateNextSiblingLayer", "triggered()", "activateNextSiblingNode()"),
        actionRoute("activatePreviousLayer", "triggered()", "activatePreviousNode()"),
        actionRoute("activatePreviousSiblingLayer", "triggered()", "activatePreviousSiblingNode()"),
        actionRoute("switchToPreviouslyActiveNode", "triggered()", "switchToPreviouslyActiveNode()"),
        actionRoute("save_node_as_image", "triggered()", "saveNodeAsImage()"),
        actionRoute("save_vector_node_to_svg", "triggered()", "saveVectorLayerAsImage()"),
        actionRoute("duplicatelayer", "triggered()", "duplicateActiveNode()"),
        actionRoute("copy_layer_clipboard", "triggered()", "copyLayersToClipboard()"),
        actionRoute("cut_layer_clipboard", "triggered()", "cutLayersToClipboard()"),
        actionRoute("paste_layer_from_clipboard", "triggered()", "pasteLayersFromClipboard()"),
        actionRoute("create_quick_group", "triggered()", "createQuickGroup()"),
        actionRoute("create_quick_clipping_group", "triggered()", "createQuickClippingGroup()"),
        actionRoute("quick_ungroup", "triggered()", "quickUngroup()"),
        actionRoute("select_all_layers", "triggered()", "selectAllNodes()"),
        actionRoute("select_visible_layers", "triggered()", "selectVisibleNodes()"),
        actionRoute("select_locked_layers", "triggered()", "selectLockedNodes()"),
        actionRoute("select_invisible_layers", "triggered()", "selectInvisibleNodes()"),
        actionRoute("select_unlocked_layers", "triggered()", "selectUnlockedNodes()"),
        actionRoute("new_from_visible", "triggered()", "createFromVisible()"),
        actionRoute("create_reference_image_from_active_layer", "triggered()", "createReferenceImageFromLayer()"),
        actionRoute("create_reference_image_from_visible_canvas", "triggered()", "createReferenceImageFromVisible()"),
        actionRoute("pin_to_timeline", "toggled(bool)", "slotPinToTimeline(bool)"),
        actionRoute("isolate_active_layer", "toggled(bool)", "setIsolateActiveLayerMode(bool)"),
        actionRoute("isolate_active_group", "triggered(bool)", "setIsolateActiveGroupMode(bool)"),
        actionRoute("toggle_layer_visibility", "triggered()", "toggleVisibility()"),
        actionRoute("toggle_layer_lock", "triggered()", "toggleLock()"),
        actionRoute("toggle_layer_inherit_alpha", "triggered()", "toggleInheritAlpha()"),
        actionRoute("toggle_layer_alpha_lock", "triggered()", "toggleAlphaLock()"),
        actionRoute("split_alpha_into_mask", "triggered()", "slotSplitAlphaIntoMask()"),
        actionRoute("split_alpha_write", "triggered()", "slotSplitAlphaWrite()"),
        actionRoute("split_alpha_save_merged", "triggered()", "slotSplitAlphaSaveMerged()"),
    };
    QCOMPARE(actionRoutes, expectedRoutes);
    QCOMPARE(checkableActions, QStringList({QStringLiteral("pin_to_timeline")}));
    QCOMPARE(shapeLayerOnlyActions, QStringList({QStringLiteral("save_vector_node_to_svg")}));
    QCOMPARE(storedPinActions, QStringList({QStringLiteral("pin_to_timeline")}));
}

void KisNodeManagerSetupContractTest::nodeCreationMappingsPreserveDelivery_data()
{
    QTest::addColumn<bool>("deferred");

    QTest::newRow("direct") << false;
    QTest::newRow("deferred") << true;
}

void KisNodeManagerSetupContractTest::nodeCreationMappingsPreserveDelivery()
{
    QFETCH(bool, deferred);
    deferNodeCreationValue = deferred;
    KisNodeManager manager(nullptr);

    manager.setup(reinterpret_cast<KisKActionCollection *>(1), reinterpret_cast<KisActionManager *>(2));

    const QStringList expectedMappings{
        typeMapping("add_new_paint_layer", "KisPaintLayer"),
        typeMapping("add_new_group_layer", "KisGroupLayer"),
        typeMapping("add_new_clone_layer", "KisCloneLayer"),
        typeMapping("add_new_shape_layer", "KisShapeLayer"),
        typeMapping("add_new_adjustment_layer", "KisAdjustmentLayer"),
        typeMapping("add_new_fill_layer", "KisGeneratorLayer"),
        typeMapping("add_new_file_layer", "KisFileLayer"),
        typeMapping("add_new_transparency_mask", "KisTransparencyMask"),
        typeMapping("add_new_filter_mask", "KisFilterMask"),
        typeMapping("add_new_fast_color_overlay_mask", "FastColorOverlayFilterMask"),
        typeMapping("add_new_colorize_mask", "KisColorizeMask"),
        typeMapping("add_new_transform_mask", "KisTransformMask"),
        typeMapping("add_new_selection_mask", "KisSelectionMask"),
    };
    QCOMPARE(nodeCreationMappings, expectedMappings);
    QCOMPARE(nodeCreationConnections, QList<bool>({deferred}));
}

void KisNodeManagerSetupContractTest::nodeConversionsPreserveExclusions()
{
    KisNodeManager manager(nullptr);

    manager.setup(reinterpret_cast<KisKActionCollection *>(1), reinterpret_cast<KisActionManager *>(2));

    QCOMPARE(nodeConversionMappings,
             QStringList({QStringLiteral("convert_to_paint_layer|KisPaintLayer|KisPaintLayer"),
                          QStringLiteral("convert_to_selection_mask|KisSelectionMask|KisSelectionMask,KisColorizeMask"),
                          QStringLiteral("convert_to_filter_mask|KisFilterMask|KisFilterMask,KisColorizeMask"),
                          QStringLiteral(
                              "convert_to_transparency_mask|KisTransparencyMask|KisTransparencyMask,KisColorizeMask"),
                          QStringLiteral("convert_to_file_layer|KisFileLayer|KisFileLayer,KisCloneLayer")}));
    QCOMPARE(nodeConversionConnections, 1);
}

void KisNodeManagerSetupContractTest::mapperAndIsolationConnectionsFollowRegistration()
{
    KisNodeManager manager(nullptr);

    manager.setup(reinterpret_cast<KisKActionCollection *>(1), reinterpret_cast<KisActionManager *>(2));

    const int lastCommand = effects.indexOf(QStringLiteral("action:pin_to_timeline"));
    const int firstCreation = effects.indexOf(QStringLiteral("creation:add_new_paint_layer"));
    const int creationConnection = effects.indexOf(QStringLiteral("connect-creation:direct"));
    const int firstConversion = effects.indexOf(QStringLiteral("conversion:convert_to_paint_layer"));
    const int conversionConnection = effects.indexOf(QStringLiteral("connect-conversion"));
    const int isolateLayer = effects.indexOf(QStringLiteral("action:isolate_active_layer"));
    const int isolateGroup = effects.indexOf(QStringLiteral("action:isolate_active_group"));
    const int isolationConnection = effects.indexOf(QStringLiteral("connect-isolation"));
    const int firstTrailingAction = effects.indexOf(QStringLiteral("action:toggle_layer_visibility"));

    QVERIFY(lastCommand < firstCreation);
    QVERIFY(firstCreation < creationConnection);
    QVERIFY(creationConnection < firstConversion);
    QVERIFY(firstConversion < conversionConnection);
    QVERIFY(conversionConnection < isolateLayer);
    QVERIFY(isolateLayer < isolateGroup);
    QVERIFY(isolateGroup < isolationConnection);
    QVERIFY(isolationConnection < firstTrailingAction);
    QCOMPARE(nodeActivationIsolationConnections, 1);
}

QTEST_MAIN(KisNodeManagerSetupContractTest)

#include "KisNodeManagerSetupContractTest.moc"
