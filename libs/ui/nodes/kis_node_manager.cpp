/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"
#include "kis_filter_mask.h"

#include <QStandardPaths>
#include <QMessageBox>
#include <KisSignalMapper.h>
#include <QApplication>
#include <kactioncollection.h>

#include <QKeySequence>

#include <kis_icon.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <KoShape.h>
#include <KoShapeLayer.h>
#include <KisImportExportManager.h>
#include <KoFileDialog.h>
#include <KoToolManager.h>
#include <KoProperties.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>

#include <kis_types.h>
#include <kis_node.h>
#include <kis_selection.h>
#include <kis_selection_mask.h>
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <kis_mask.h>
#include <kis_image.h>
#include <kis_painter.h>
#include <kis_paint_layer.h>
#include <KisMimeDatabase.h>
#include <KisReferenceImagesLayer.h>

#include "application/ui/orchestration/KisPart.h"
#include "canvas/kis_canvas2.h"
#include "kis_shape_controller.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "application/ui/workspace/KisViewManager.h"
#include "document/KisDocument.h"
#include "nodes/kis_mask_manager.h"
#include "nodes/kis_layer_manager.h"
#include "selection/kis_selection_manager.h"
#include <commands/kis_node_commands_adapter.h>
#include "application/ui/orchestration/kis_action.h"
#include "application/ui/orchestration/kis_action_manager.h"
#include "kis_sequential_iterator.h"
#include "kis_transaction.h"
#include "nodes/kis_node_selection_adapter.h"
#include "nodes/kis_node_insertion_adapter.h"
#include "commands/kis_node_operation_batch.h"
#include "canvas/KisNodeDisplayModeAdapter.h"
#include "kis_clipboard.h"
#include "kis_node_dummies_graph.h"
#include "kis_mimedata.h"
#include "kis_layer_utils.h"
#include "krita_utils.h"
#include "kis_shape_layer.h"
#include "processing/kis_mirror_processing_visitor.h"
#include "application/ui/workspace/KisView.h"

#include <kis_signals_blocker.h>
#include <libs/image/kis_layer_properties_icons.h>
#include <libs/image/commands/kis_node_property_list_command.h>
#include <KisSynchronizedConnection.h>

struct KisNodeManager::Private {
    Private(KisNodeManager *_q, KisViewManager *v)
        : q(_q)
        , view(v)
        , imageView(0)
        , layerManager(v)
        , maskManager(v)
        , commandsAdapter(KisImageWSP(), v)
        , nodeSelectionAdapter(new KisNodeSelectionAdapter(q))
        , nodeInsertionAdapter(new KisNodeInsertionAdapter(q))
        , nodeDisplayModeAdapter(new KisNodeDisplayModeAdapter())
        , lastRequestedIsolatedModeStatus(false)
    {
    }

    KisNodeManager * q {nullptr};
    KisViewManager * view {nullptr};
    QPointer<KisView>imageView;
    KisLayerManager layerManager;
    KisMaskManager maskManager;
    KisNodeCommandsAdapter commandsAdapter;
    QScopedPointer<KisNodeSelectionAdapter> nodeSelectionAdapter;
    QScopedPointer<KisNodeInsertionAdapter> nodeInsertionAdapter;
    QScopedPointer<KisNodeDisplayModeAdapter> nodeDisplayModeAdapter;

    KisAction *pinToTimeline {nullptr};

    KisNodeList selectedNodes;
    QPointer<KisNodeOperationBatch> nodeOperationBatch;

    KisNodeWSP previouslyActiveNode;

    bool activateNodeImpl(KisNodeSP node);

    KisSignalMapper nodeCreationSignalMapper;
    KisSignalMapper nodeConversionSignalMapper;

    bool lastRequestedIsolatedModeStatus {false};
    KisSynchronizedConnection<KisNodeSP, KisNodeList> activateNodeConnection;

    void saveDeviceAsImage(KisPaintDeviceSP device,
                           const QString &defaultName,
                           const QRect &bounds,
                           qreal xRes,
                           qreal yRes,
                           quint8 opacity);

    void mergeTransparencyMaskAsAlpha(bool writeToLayers);
    KisNodeOperationBatch* lazyGetNodeOperationBatch(const KUndo2MagicString &actionName);
};

bool KisNodeManager::Private::activateNodeImpl(KisNodeSP node)
{
    Q_ASSERT(view);
    Q_ASSERT(view->canvasBase());
    Q_ASSERT(view->canvasBase()->globalShapeManager());
    Q_ASSERT(imageView);
    if (node && node == q->activeNode()) {
        return false;
    }

    // Set the selection on the shape manager to the active layer
    // and set call KoSelection::setActiveLayer( KoShapeLayer* layer )
    // with the parent of the active layer.
    KoSelection *selection = view->canvasBase()->globalShapeManager()->selection();
    Q_ASSERT(selection);
    selection->deselectAll();

    // Disable all enter-group modes that were active
    view->canvasBase()->setCurrentShapeManagerOwnerShape(nullptr);

    if (!node) {
        selection->setActiveLayer(0);
        imageView->setCurrentNode(0);
        maskManager.activateMask(0);
        layerManager.activateLayer(0);
        previouslyActiveNode = q->activeNode();
    } else {
        previouslyActiveNode = q->activeNode();

        KoShape * shape = view->document()->shapeForNode(node);

        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(shape, false);

        selection->select(shape);
        KoShapeLayer * shapeLayer = dynamic_cast<KoShapeLayer*>(shape);

        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(shapeLayer, false);

        //         shapeLayer->setGeometryProtected(node->userLocked());
        //         shapeLayer->setVisible(node->visible());
        selection->setActiveLayer(shapeLayer);

        imageView->setCurrentNode(node);
        if (KisLayerSP layer = qobject_cast<KisLayer*>(node.data())) {
            maskManager.activateMask(0);
            layerManager.activateLayer(layer);
        } else if (KisMaskSP mask = dynamic_cast<KisMask*>(node.data())) {
            maskManager.activateMask(mask);
            // XXX_NODE: for now, masks cannot be nested.
            layerManager.activateLayer(static_cast<KisLayer*>(node->parent().data()));
        }
    }
    return true;
}

//=====================================================================================

KisNodeManager::KisNodeManager(KisViewManager *view)
    : m_d(new Private(this, view))
{
    m_d->activateNodeConnection.connectOutputSlot(this, &KisNodeManager::slotImageRequestNodeReselection);
}

KisNodeManager::~KisNodeManager()
{
    delete m_d;
}

void KisNodeManager::setView(QPointer<KisView>imageView)
{
    m_d->maskManager.setView(imageView);
    m_d->layerManager.setView(imageView);

    if (m_d->imageView) {
        KisShapeController *shapeController = dynamic_cast<KisShapeController*>(m_d->imageView->document()->shapeController());
        Q_ASSERT(shapeController);
        shapeController->disconnect(SIGNAL(sigActivateNode(KisNodeSP)), this);
        m_d->imageView->image()->disconnect(this);
        m_d->imageView->image()->disconnect(&m_d->activateNodeConnection);
    }

    m_d->imageView = imageView;
    m_d->commandsAdapter.setImage(imageView ? imageView->image() : KisImageWSP());

    if (m_d->imageView) {
        KisShapeController *shapeController = dynamic_cast<KisShapeController*>(m_d->imageView->document()->shapeController());
        Q_ASSERT(shapeController);
        connect(shapeController, SIGNAL(sigActivateNode(KisNodeSP)), SLOT(slotNonUiActivatedNode(KisNodeSP)));

        if (!m_d->imageView->currentNode()) {
            /**
             * The view has not been initialized yet, so we should try to initialize it with
             * the node saved in KisDummiesFacadeBase (or just wait for a signal from it)
             */
            if (shapeController->lastActivatedNode() && !m_d->imageView->currentNode()) {
                slotNonUiActivatedNode(shapeController->lastActivatedNode());
            } else {
                // if last activated node is null, most probably, it means that the shape controller
                // is going to Q_EMIT the activation signal very soon
            }
        } else {
            /**
             * If the view is initialized, we should check if the layer still belongs
             * to the actual image, since it could have been removed. And since the
             * forwarding happens via KisNodeManager, the could have missed this
             * update.
             */
            if (!m_d->imageView->currentNode()->graphListener()) {
                slotNonUiActivatedNode(m_d->imageView->image()->root()->lastChild());
            }
        }

        m_d->activateNodeConnection.connectInputSignal(m_d->imageView->image(), &KisImage::sigRequestNodeReselection);
        m_d->imageView->resourceProvider()->slotNodeActivated(m_d->imageView->currentNode());
        connect(m_d->imageView->image(), SIGNAL(sigIsolatedModeChanged()), this, SLOT(handleExternalIsolationChange()));
    }

}

#define NEW_LAYER_ACTION(id, layerType)                                 \
{                                                                   \
    action = actionManager->createAction(id);                       \
    m_d->nodeCreationSignalMapper.setMapping(action, layerType);    \
    connect(action, SIGNAL(triggered()),                            \
    &m_d->nodeCreationSignalMapper, SLOT(map()));           \
    }

#define CONVERT_NODE_ACTION_2(id, layerType, exclude)                   \
{                                                                   \
    action = actionManager->createAction(id);                       \
    action->setExcludedNodeTypes(QStringList(exclude));             \
    actionManager->addAction(id, action);                           \
    m_d->nodeConversionSignalMapper.setMapping(action, layerType);  \
    connect(action, SIGNAL(triggered()),                            \
    &m_d->nodeConversionSignalMapper, SLOT(map()));         \
    }

#define CONVERT_NODE_ACTION(id, layerType)              \
    CONVERT_NODE_ACTION_2(id, layerType, layerType)

void KisNodeManager::setup(KisKActionCollection * actionCollection, KisActionManager* actionManager)
{
    m_d->layerManager.setup(actionManager);
    m_d->maskManager.setup(actionCollection, actionManager);

    KisAction * action = 0;

    action = actionManager->createAction("mirrorNodeX");
    connect(action, SIGNAL(triggered()), this, SLOT(mirrorNodeX()));

    action  = actionManager->createAction("mirrorNodeY");
    connect(action, SIGNAL(triggered()), this, SLOT(mirrorNodeY()));

    action = actionManager->createAction("mirrorAllNodesX");
    connect(action, SIGNAL(triggered()), this, SLOT(mirrorAllNodesX()));

    action  = actionManager->createAction("mirrorAllNodesY");
    connect(action, SIGNAL(triggered()), this, SLOT(mirrorAllNodesY()));

    action = actionManager->createAction("activateNextLayer");
    connect(action, SIGNAL(triggered()), this, SLOT(activateNextNode()));

    action = actionManager->createAction("activateNextSiblingLayer");
    connect(action, SIGNAL(triggered()), this, SLOT(activateNextSiblingNode()));

    action = actionManager->createAction("activatePreviousLayer");
    connect(action, SIGNAL(triggered()), this, SLOT(activatePreviousNode()));

    action = actionManager->createAction("activatePreviousSiblingLayer");
    connect(action, SIGNAL(triggered()), this, SLOT(activatePreviousSiblingNode()));

    action = actionManager->createAction("switchToPreviouslyActiveNode");
    connect(action, SIGNAL(triggered()), this, SLOT(switchToPreviouslyActiveNode()));

    action  = actionManager->createAction("save_node_as_image");
    connect(action, SIGNAL(triggered()), this, SLOT(saveNodeAsImage()));

    action  = actionManager->createAction("save_vector_node_to_svg");
    connect(action, SIGNAL(triggered()), this, SLOT(saveVectorLayerAsImage()));
    action->setActivationFlags(KisAction::ACTIVE_SHAPE_LAYER);

    action = actionManager->createAction("duplicatelayer");
    connect(action, SIGNAL(triggered()), this, SLOT(duplicateActiveNode()));

    action = actionManager->createAction("copy_layer_clipboard");
    connect(action, SIGNAL(triggered()), this, SLOT(copyLayersToClipboard()));

    action = actionManager->createAction("cut_layer_clipboard");
    connect(action, SIGNAL(triggered()), this, SLOT(cutLayersToClipboard()));

    action = actionManager->createAction("paste_layer_from_clipboard");
    connect(action, SIGNAL(triggered()), this, SLOT(pasteLayersFromClipboard()));

    action = actionManager->createAction("create_quick_group");
    connect(action, SIGNAL(triggered()), this, SLOT(createQuickGroup()));

    action = actionManager->createAction("create_quick_clipping_group");
    connect(action, SIGNAL(triggered()), this, SLOT(createQuickClippingGroup()));

    action = actionManager->createAction("quick_ungroup");
    connect(action, SIGNAL(triggered()), this, SLOT(quickUngroup()));

    action = actionManager->createAction("select_all_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectAllNodes()));

    action = actionManager->createAction("select_visible_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectVisibleNodes()));

    action = actionManager->createAction("select_locked_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectLockedNodes()));

    action = actionManager->createAction("select_invisible_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectInvisibleNodes()));

    action = actionManager->createAction("select_unlocked_layers");
    connect(action, SIGNAL(triggered()), this, SLOT(selectUnlockedNodes()));

    action = actionManager->createAction("new_from_visible");
    connect(action, SIGNAL(triggered()), this, SLOT(createFromVisible()));
    
    action = actionManager->createAction("create_reference_image_from_active_layer");
    connect(action, SIGNAL(triggered()), this, SLOT(createReferenceImageFromLayer()));
    
    action = actionManager->createAction("create_reference_image_from_visible_canvas");
    connect(action, SIGNAL(triggered()), this, SLOT(createReferenceImageFromVisible()));

    action = actionManager->createAction("pin_to_timeline");
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(slotPinToTimeline(bool)));
    m_d->pinToTimeline = action;

    NEW_LAYER_ACTION("add_new_paint_layer", "KisPaintLayer");

    NEW_LAYER_ACTION("add_new_group_layer", "KisGroupLayer");

    NEW_LAYER_ACTION("add_new_clone_layer", "KisCloneLayer");

    NEW_LAYER_ACTION("add_new_shape_layer", "KisShapeLayer");

    NEW_LAYER_ACTION("add_new_adjustment_layer", "KisAdjustmentLayer");

    NEW_LAYER_ACTION("add_new_fill_layer", "KisGeneratorLayer");

    NEW_LAYER_ACTION("add_new_file_layer", "KisFileLayer");

    NEW_LAYER_ACTION("add_new_transparency_mask", "KisTransparencyMask");

    NEW_LAYER_ACTION("add_new_filter_mask", "KisFilterMask");

    // NOTE: FastColorOverlayFilterMask is just an identifier, not an actual class name
    NEW_LAYER_ACTION("add_new_fast_color_overlay_mask", "FastColorOverlayFilterMask");

    NEW_LAYER_ACTION("add_new_colorize_mask", "KisColorizeMask");

    NEW_LAYER_ACTION("add_new_transform_mask", "KisTransformMask");

    NEW_LAYER_ACTION("add_new_selection_mask", "KisSelectionMask");

#ifdef Q_OS_IOS
    // Pencil actions can arrive through a synthesized mouse release. Defer
    // node and UI mutation until Qt has finished that tablet/mouse delivery;
    // otherwise QGestureManager may retain a context destroyed by the action.
    connect(&m_d->nodeCreationSignalMapper, SIGNAL(mapped(QString)),
            this, SLOT(createNode(QString)), Qt::QueuedConnection);
#else
    connect(&m_d->nodeCreationSignalMapper, SIGNAL(mapped(QString)),
            this, SLOT(createNode(QString)));
#endif

    CONVERT_NODE_ACTION("convert_to_paint_layer", "KisPaintLayer");

    CONVERT_NODE_ACTION_2("convert_to_selection_mask", "KisSelectionMask", QStringList() << "KisSelectionMask" << "KisColorizeMask");

    CONVERT_NODE_ACTION_2("convert_to_filter_mask", "KisFilterMask", QStringList() << "KisFilterMask" << "KisColorizeMask");

    CONVERT_NODE_ACTION_2("convert_to_transparency_mask", "KisTransparencyMask", QStringList() << "KisTransparencyMask" << "KisColorizeMask");

    CONVERT_NODE_ACTION_2("convert_to_file_layer", "KisFileLayer", QStringList() << "KisFileLayer" << "KisCloneLayer");

    connect(&m_d->nodeConversionSignalMapper, SIGNAL(mapped(QString)),
            this, SLOT(convertNode(QString)));

    // Isolation Modes...
    // Post Qt5.14 this can be replaced with QActionGroup + ExclusionPolicy::ExclusiveOptional.
    action = actionManager->createAction("isolate_active_layer");
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setIsolateActiveLayerMode(bool)));
    action = actionManager->createAction("isolate_active_group");
    connect(action, SIGNAL(triggered(bool)), this, SLOT(setIsolateActiveGroupMode(bool)));
    connect(this, SIGNAL(sigNodeActivated(KisNodeSP)), SLOT(changeIsolationRoot(KisNodeSP)));

    action = actionManager->createAction("toggle_layer_visibility");
    connect(action, SIGNAL(triggered()), this, SLOT(toggleVisibility()));

    action = actionManager->createAction("toggle_layer_lock");
    connect(action, SIGNAL(triggered()), this, SLOT(toggleLock()));

    action = actionManager->createAction("toggle_layer_inherit_alpha");
    connect(action, SIGNAL(triggered()), this, SLOT(toggleInheritAlpha()));

    action = actionManager->createAction("toggle_layer_alpha_lock");
    connect(action, SIGNAL(triggered()), this, SLOT(toggleAlphaLock()));

    action  = actionManager->createAction("split_alpha_into_mask");
    connect(action, SIGNAL(triggered()), this, SLOT(slotSplitAlphaIntoMask()));

    action  = actionManager->createAction("split_alpha_write");
    connect(action, SIGNAL(triggered()), this, SLOT(slotSplitAlphaWrite()));

    // HINT: we can save even when the nodes are not editable
    action  = actionManager->createAction("split_alpha_save_merged");
    connect(action, SIGNAL(triggered()), this, SLOT(slotSplitAlphaSaveMerged()));
}

void KisNodeManager::updateGUI()
{
    // enable/disable all relevant actions
    m_d->layerManager.updateGUI();
    m_d->maskManager.updateGUI();
}

KisNodeSP KisNodeManager::ActiveAccess::activeNode(KisNodeManager *manager)
{
    if (manager->m_d->imageView) {
        return manager->m_d->imageView->currentNode();
    }
    return 0;
}

KisLayerSP KisNodeManager::ActiveAccess::activeLayer(KisNodeManager *manager)
{
    return manager->m_d->layerManager.activeLayer();
}

bool KisNodeManager::ActiveAccess::hasActiveMask(KisNodeManager *manager)
{
    return manager->m_d->maskManager.activeMask();
}

KisPaintDeviceSP KisNodeManager::ActiveAccess::activeMaskDevice(KisNodeManager *manager)
{
    return manager->m_d->maskManager.activeDevice();
}

KisPaintDeviceSP KisNodeManager::ActiveAccess::activeLayerDevice(KisNodeManager *manager)
{
    return manager->m_d->layerManager.activeDevice();
}

bool KisNodeManager::ActiveAccess::hasActiveMaskDevice(KisNodeManager *manager)
{
    return manager->m_d->maskManager.activeDevice();
}

const KoColorSpace *KisNodeManager::ActiveAccess::activeMaskColorSpace(KisNodeManager *manager)
{
    return manager->m_d->maskManager.activeDevice()->colorSpace();
}

bool KisNodeManager::ActiveAccess::hasActiveLayer(KisNodeManager *manager)
{
    return manager->m_d->layerManager.activeLayer();
}

bool KisNodeManager::ActiveAccess::activeLayerHasParent(KisNodeManager *manager)
{
    return manager->m_d->layerManager.activeLayer()->parentLayer();
}

const KoColorSpace *KisNodeManager::ActiveAccess::activeLayerParentColorSpace(KisNodeManager *manager)
{
    return manager->m_d->layerManager.activeLayer()->parentLayer()->colorSpace();
}

const KoColorSpace *KisNodeManager::ActiveAccess::imageColorSpace(KisNodeManager *manager)
{
    return manager->m_d->view->image()->colorSpace();
}

bool KisNodeManager::ModificationAccess::isEditable(KisNodeSP node)
{
    return node->isEditable(false);
}

QString KisNodeManager::ModificationAccess::name(KisNodeSP node)
{
    return node->name();
}

KisNodeSP KisNodeManager::ModificationAccess::parentNode(KisNodeSP node)
{
    return node->parent();
}

void KisNodeManager::ModificationAccess::showWarning(KisNodeManager *manager, const QString &message)
{
    manager->m_d->view->showFloatingMessage(message, QIcon());
}

KisNodeList KisNodeManager::TreeOperationAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

KisNodeSP KisNodeManager::TreeOperationAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

void KisNodeManager::TreeOperationAccess::moveNodeAt(KisNodeManager *manager,
                                                     KisNodeSP node,
                                                     KisNodeSP parent,
                                                     int index)
{
    manager->m_d->commandsAdapter.moveNode(node, parent, index);
}

void KisNodeManager::TreeOperationAccess::moveNodes(KisNodeManager *manager,
                                                    const KisNodeList &nodes,
                                                    KisNodeSP parent,
                                                    KisNodeSP aboveThis,
                                                    KisNodeSP activeNode)
{
    manager->m_d->lazyGetNodeOperationBatch(kundo2_i18n("Move Nodes"))->moveNode(nodes, parent, aboveThis, activeNode);
}

void KisNodeManager::TreeOperationAccess::copyNodes(KisNodeManager *manager,
                                                    const KisNodeList &nodes,
                                                    KisNodeSP parent,
                                                    KisNodeSP aboveThis,
                                                    KisNodeSP activeNode)
{
    manager->m_d->lazyGetNodeOperationBatch(kundo2_i18n("Copy Nodes"))->copyNode(nodes, parent, aboveThis, activeNode);
}

void KisNodeManager::TreeOperationAccess::addNodes(KisNodeManager *manager,
                                                   const KisNodeList &nodes,
                                                   KisNodeSP parent,
                                                   KisNodeSP aboveThis,
                                                   KisNodeSP activeNode)
{
    manager->m_d->lazyGetNodeOperationBatch(kundo2_i18n("Add Nodes"))->addNode(nodes, parent, aboveThis, activeNode);
}

void KisNodeManager::TreeOperationAccess::addNodeUndoable(KisNodeManager *manager,
                                                          KisNodeSP node,
                                                          KisNodeSP parent,
                                                          KisNodeSP aboveThis)
{
    manager->m_d->commandsAdapter.addNode(node, parent, aboveThis);
}

void KisNodeManager::TreeOperationAccess::duplicateNodes(KisNodeManager *manager,
                                                         const KisNodeList &nodes,
                                                         KisNodeSP activeNode)
{
    const KUndo2MagicString actionName = kundo2_i18n("Duplicate Nodes");
    KisNodeOperationBatch *batch = manager->m_d->lazyGetNodeOperationBatch(actionName);
    batch->duplicateNode(nodes, activeNode);
}

bool KisNodeManager::IsolationAccess::imageAvailable(KisNodeManager *manager)
{
    return bool(manager->m_d->view->image());
}

bool KisNodeManager::IsolationAccess::isIsolatingLayer(KisNodeManager *manager)
{
    return manager->m_d->view->image()->isIsolatingLayer();
}

bool KisNodeManager::IsolationAccess::isIsolatingGroup(KisNodeManager *manager)
{
    return manager->m_d->view->image()->isIsolatingGroup();
}

KisNodeSP KisNodeManager::IsolationAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

bool KisNodeManager::IsolationAccess::startIsolatedMode(KisNodeManager *manager,
                                                        KisNodeSP isolationRoot,
                                                        bool isolateActiveLayer,
                                                        bool isolateActiveGroup)
{
    return manager->m_d->view->image()->startIsolatedMode(isolationRoot, isolateActiveLayer, isolateActiveGroup);
}

void KisNodeManager::IsolationAccess::stopIsolatedMode(KisNodeManager *manager)
{
    manager->m_d->view->image()->stopIsolatedMode();
}

bool KisNodeManager::IsolationAccess::isActiveWindow(KisNodeManager *manager)
{
    return manager->m_d->view->mainWindowAsQWidget()->isActiveWindow();
}

void KisNodeManager::IsolationAccess::toggleLayerAction(KisNodeManager *manager)
{
    manager->m_d->view->actionManager()->actionByName("isolate_active_layer")->toggle();
}

void KisNodeManager::IsolationAccess::setLayerActionChecked(KisNodeManager *manager, bool checked)
{
    manager->m_d->view->actionManager()->actionByName("isolate_active_layer")->setChecked(checked);
}

void KisNodeManager::IsolationAccess::setGroupActionChecked(KisNodeManager *manager, bool checked)
{
    manager->m_d->view->actionManager()->actionByName("isolate_active_group")->setChecked(checked);
}

KisNodeSP  KisNodeManager::createNode(const QString & nodeType, bool quiet, KisPaintDeviceSP copyFrom)
{
    if (!m_d->view->blockUntilOperationsFinished(m_d->view->image())) {
        return 0;
    }

    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) {
        activeNode = m_d->view->image()->root();
    }

    KIS_ASSERT_RECOVER_RETURN_VALUE(activeNode, 0);

    /// the check for editability happens inside the functions
    /// themselves, because layers can be created anyway (in a
    /// different position), but masks cannot.

    // XXX: make factories for this kind of stuff,
    //      with a registry

    if (nodeType == "KisPaintLayer") {
        return m_d->layerManager.addPaintLayer(activeNode);
    } else if (nodeType == "KisGroupLayer") {
        return m_d->layerManager.addGroupLayer(activeNode);
    } else if (nodeType == "KisAdjustmentLayer") {
        return m_d->layerManager.addAdjustmentLayer(activeNode);
    } else if (nodeType == "KisGeneratorLayer") {
        return m_d->layerManager.addGeneratorLayer(activeNode);
    } else if (nodeType == "KisShapeLayer") {
        return m_d->layerManager.addShapeLayer(activeNode);
    } else if (nodeType == "KisCloneLayer") {
        KisNodeList nodes = selectedNodes();
        if (nodes.isEmpty()) {
            nodes.append(activeNode);
        }
        return m_d->layerManager.addCloneLayer(nodes);
    } else if (nodeType == "KisTransparencyMask") {
        return m_d->maskManager.createTransparencyMask(activeNode, copyFrom, false);
    } else if (nodeType == "KisFilterMask") {
        return m_d->maskManager.createFilterMask(activeNode, copyFrom, quiet, false);
    } else if (nodeType == "FastColorOverlayFilterMask") {
        return m_d->maskManager.createFastColorOverlayMask(activeNode);
    } else if (nodeType == "KisColorizeMask") {
        return m_d->maskManager.createColorizeMask(activeNode);
    } else if (nodeType == "KisTransformMask") {
        return m_d->maskManager.createTransformMask(activeNode);
    } else if (nodeType == "KisSelectionMask") {
        return m_d->maskManager.createSelectionMask(activeNode, copyFrom, false);
    } else if (nodeType == "KisFileLayer") {
        return m_d->layerManager.addFileLayer(activeNode);
    }
    return 0;
}

KisImage *KisNodeManager::LayerCreationAccess::image(KisNodeManager *manager)
{
    return manager->m_d->view->image().data();
}

KisNode *KisNodeManager::LayerCreationAccess::rootLastChild(KisImage *image)
{
    return image->root()->lastChild().data();
}

void KisNodeManager::LayerCreationAccess::createFromVisible(KisImage *image, KisNode *putAfter)
{
    KisLayerUtils::newLayerFromVisible(KisImageSP(image), KisNodeSP(putAfter));
}

KisLayerSP KisNodeManager::LayerCreationAccess::createPaintLayer(KisNodeManager *manager, const QString &nodeType)
{
    KisNodeSP node = manager->createNode(nodeType);
    return dynamic_cast<KisLayer *>(node.data());
}

void KisNodeManager::convertNode(const QString &nodeType)
{
    if (!m_d->view->blockUntilOperationsFinished(m_d->view->image())) {
        return;
    }

    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) return;

    if (!canModifyLayer(activeNode)) return;

    if (nodeType == "KisPaintLayer") {
        m_d->layerManager.convertNodeToPaintLayer(activeNode);
    } else if (nodeType == "KisSelectionMask" ||
               nodeType == "KisFilterMask" ||
               nodeType == "KisTransparencyMask") {

        KisPaintDeviceSP copyFrom = activeNode->paintDevice() ?
                    activeNode->paintDevice() : activeNode->projection();

        m_d->commandsAdapter.beginMacro(kundo2_i18n("Convert to a Selection Mask"));

        bool result = false;

        if (nodeType == "KisSelectionMask") {
            result = !m_d->maskManager.createSelectionMask(activeNode, copyFrom, true).isNull();
        } else if (nodeType == "KisFilterMask") {
            result = !m_d->maskManager.createFilterMask(activeNode, copyFrom, false, true).isNull();
        } else if (nodeType == "KisTransparencyMask") {
            result = !m_d->maskManager.createTransparencyMask(activeNode, copyFrom, true).isNull();
        }

        m_d->commandsAdapter.endMacro();

        if (!result) {
            m_d->view->blockUntilOperationsFinishedForced(m_d->imageView->image());
            m_d->commandsAdapter.undoLastCommand();
        }
    } else if (nodeType == "KisFileLayer") {
        m_d->layerManager.convertLayerToFileLayer(activeNode);
    } else {
        warnKrita << "Unsupported node conversion type:" << nodeType;
    }
}

KisPaintDevice *KisNodeManager::ReferenceImageAccess::activeLayerProjection(KisNodeManager *manager)
{
    return manager->m_d->view->activeLayer()->projection().data();
}

KisPaintDevice *KisNodeManager::ReferenceImageAccess::visibleProjection(KisNodeManager *manager)
{
    return manager->m_d->view->canvasBase()->currentImage()->projection().data();
}

QImage KisNodeManager::ReferenceImageAccess::convertToImage(KisPaintDevice *device)
{
    return device->convertToQImage(0,
                                   KoColorConversionTransformation::internalRenderingIntent(),
                                   KoColorConversionTransformation::internalConversionFlags());
}

KisReferenceImage *KisNodeManager::ReferenceImageAccess::createReferenceImage(KisNodeManager *manager,
                                                                              const QImage &image)
{
    return KisReferenceImage::fromQImage(*manager->m_d->view->canvasBase()->coordinatesConverter(), image);
}

void KisNodeManager::ReferenceImageAccess::deleteReferenceImage(KisReferenceImage *reference)
{
    delete reference;
}

int KisNodeManager::ReferenceImageAccess::referenceImageCount(KisNodeManager *manager)
{
    KisReferenceImagesLayerSP layer = manager->m_d->view->document()->referenceImagesLayer();
    return layer ? layer->shapes().size() : -1;
}

void KisNodeManager::ReferenceImageAccess::setZIndex(KisReferenceImage *reference, int index)
{
    reference->setZIndex(index);
}

void KisNodeManager::ReferenceImageAccess::addReferenceImage(KisNodeManager *manager, KisReferenceImage *reference)
{
    KisViewManager *view = manager->m_d->view;
    view->canvasBase()->addCommand(KisReferenceImagesLayer::addReferenceImages(view->document(), {reference}));
}

void KisNodeManager::ReferenceImageAccess::switchTool(const QString &toolId)
{
    KoToolManager::instance()->switchToolRequested(toolId);
}

bool KisNodeManager::ReferenceImageAccess::hasCanvasWidget(KisNodeManager *manager)
{
    return manager->m_d->view->canvasBase()->canvasWidget();
}

void KisNodeManager::ReferenceImageAccess::showFloatingMessage(KisNodeManager *manager,
                                                               const QString &message,
                                                               int timeout,
                                                               bool highPriority,
                                                               bool singleLine)
{
    const KisFloatingMessage::Priority priority = highPriority ? KisFloatingMessage::High : KisFloatingMessage::Medium;
    const int alignment = singleLine ? Qt::TextSingleLine : Qt::AlignCenter | Qt::TextWordWrap;
    manager->m_d->view->showFloatingMessage(message, QIcon(), timeout, priority, alignment);
}

void KisNodeManager::slotSomethingActivatedNodeImpl(KisNodeSP node)
{
    KisDummiesFacadeBase *dummiesFacade = dynamic_cast<KisDummiesFacadeBase*>(m_d->imageView->document()->shapeController());
    KIS_SAFE_ASSERT_RECOVER_RETURN(dummiesFacade);

    const bool nodeVisible = !isNodeHidden(node, !m_d->nodeDisplayModeAdapter->showGlobalSelectionMask());
    if (!nodeVisible) {
        return;
    }

    KIS_ASSERT_RECOVER_RETURN(node != activeNode());
    if (m_d->activateNodeImpl(node)) {
        if (node) {
            /**
             * Notify the dummies facade about the lastly
             * activated node. This information may be used
             * when a new view is created for the image.
             */
            dummiesFacade->setLastActivatedNode(node);
        }
        Q_EMIT sigUiNeedChangeActiveNode(node);
        Q_EMIT sigNodeActivated(node);
        nodesUpdated();
        if (node) {
            bool toggled =  m_d->view->actionCollection()->action("view_show_canvas_only")->isChecked();
            if (toggled) {
                m_d->view->showFloatingMessage( node->name(), QIcon(), 1600, KisFloatingMessage::Medium, Qt::TextSingleLine);
            }
        }
    }
}

void KisNodeManager::slotNonUiActivatedNode(KisNodeSP node)
{
    // the node must still be in the graph, some asynchronous
    // signals may easily break this requirement
    if (node && !node->graphListener()) {
        node = 0;
    }

    if (node == activeNode()) return;

    slotSomethingActivatedNodeImpl(node);
}

void KisNodeManager::slotUiActivatedNode(KisNodeSP node)
{
    // the node must still be in the graph, some asynchronous
    // signals may easily break this requirement
    if (node && !node->graphListener()) {
        node = 0;
    }

    if (node) {
        QStringList vectorTools = QStringList()
                << "InteractionTool"
                << "KarbonGradientTool"
                << "KarbonCalligraphyTool"
                << "PathTool";

        QStringList pixelTools = QStringList()
                << "KritaShape/KisToolBrush"
                << "KritaShape/KisToolDyna"
                << "KritaShape/KisToolMultiBrush"
                << "KritaFill/KisToolFill"
                << "KritaFill/KisToolGradient";

        KisSelectionMask *selectionMask = dynamic_cast<KisSelectionMask*>(node.data());
        const bool nodeHasVectorAbilities = node->inherits("KisShapeLayer") ||
                (selectionMask && selectionMask->selection()->hasShapeSelection());

        if (nodeHasVectorAbilities) {
            if (pixelTools.contains(KoToolManager::instance()->activeToolId())) {
                KoToolManager::instance()->switchToolRequested("InteractionTool");
            }
        }
        else {
            if (vectorTools.contains(KoToolManager::instance()->activeToolId())) {
                KoToolManager::instance()->switchToolRequested("KritaShape/KisToolBrush");
            }
        }
    }

    if (node == activeNode()) return;

    slotSomethingActivatedNodeImpl(node);
}

QString KisNodeManager::NodeChangeAccess::name(KisNodeSP node)
{
    return node->name();
}

qint32 KisNodeManager::NodeChangeAccess::opacity(KisNodeSP node)
{
    return node->opacity();
}

const KoCompositeOp *KisNodeManager::NodeChangeAccess::compositeOp(KisNodeSP node)
{
    return node->compositeOp();
}

void KisNodeManager::NodeChangeAccess::setName(KisNodeManager *manager, KisNodeSP node, const QString &name)
{
    manager->m_d->commandsAdapter.setNodeName(node, name);
}

void KisNodeManager::NodeChangeAccess::setOpacity(KisNodeManager *manager, KisNodeSP node, qint32 opacity)
{
    manager->m_d->commandsAdapter.setOpacity(node, opacity);
}

void KisNodeManager::NodeChangeAccess::setCompositeOp(KisNodeManager *manager,
                                                      KisNodeSP node,
                                                      const KoCompositeOp *compositeOp)
{
    manager->m_d->commandsAdapter.setCompositeOp(node, compositeOp);
}

void KisNodeManager::SelectionStateAccess::setSelectedNodes(KisNodeManager *manager, const KisNodeList &nodes)
{
    manager->m_d->selectedNodes = nodes;
}

KisNodeList KisNodeManager::AccessorAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->m_d->selectedNodes;
}

KisNodeSelectionAdapter *KisNodeManager::AccessorAccess::nodeSelectionAdapter(const KisNodeManager *manager)
{
    return manager->m_d->nodeSelectionAdapter.data();
}

KisNodeInsertionAdapter *KisNodeManager::AccessorAccess::nodeInsertionAdapter(const KisNodeManager *manager)
{
    return manager->m_d->nodeInsertionAdapter.data();
}

KisNodeDisplayModeAdapter *KisNodeManager::AccessorAccess::nodeDisplayModeAdapter(const KisNodeManager *manager)
{
    return manager->m_d->nodeDisplayModeAdapter.data();
}

bool KisNodeManager::PropertyAccess::isPaintLayer(KisNodeSP node)
{
    return dynamic_cast<KisPaintLayer *>(node.data());
}

bool KisNodeManager::PropertyAccess::containsOnionSkin(const KisBaseNode::PropertyList &properties)
{
    const auto onionSkinOn = KisLayerPropertiesIcons::getProperty(KisLayerPropertiesIcons::onionSkins, true);
    return properties.contains(onionSkinOn);
}

bool KisNodeManager::PropertyAccess::hasOpaqueBackground(KisNodeSP node)
{
    const KisPaintLayer *paintLayer = dynamic_cast<KisPaintLayer *>(node.data());
    Q_ASSERT(paintLayer);
    const KisPaintDeviceSP &paintDevice = paintLayer->paintDevice();
    return paintDevice && paintDevice->defaultPixel().opacityU8() == 255;
}

void KisNodeManager::PropertyAccess::showOnionSkinTransparencyWarning(const KisNodeManager *manager)
{
    manager->m_d->view->showFloatingMessage(i18n("Onion skins require a layer with transparent background."), QIcon());
}

void KisNodeManager::PropertyAccess::applyProperties(KisNodeSP node,
                                                     KisImageSP image,
                                                     KisBaseNode::PropertyList properties)
{
    KisNodePropertyListCommand::setNodePropertiesAutoUndo(node, image, properties);
}

bool KisNodeManager::PropertyDialogAccess::isLayer(KisNodeSP node)
{
    return node->inherits("KisLayer");
}

bool KisNodeManager::PropertyDialogAccess::isMask(KisNodeSP node)
{
    return node->inherits("KisMask");
}

void KisNodeManager::PropertyDialogAccess::showLayerProperties(KisNodeManager *manager)
{
    manager->m_d->layerManager.layerProperties();
}

void KisNodeManager::PropertyDialogAccess::showMaskProperties(KisNodeManager *manager)
{
    manager->m_d->maskManager.maskProperties();
}

KisNodeSP KisNodeManager::PropertyDialogAccess::currentNode(KisNodeManager *manager)
{
    return manager->m_d->imageView->currentNode();
}

void KisNodeManager::PropertyDialogAccess::setCurrentNode(KisNodeManager *manager, KisNodeSP node)
{
    manager->m_d->imageView->setCurrentNode(node);
}

void KisNodeManager::PropertyDialogAccess::changeCloneSource(KisNodeManager *manager)
{
    manager->m_d->layerManager.changeCloneSource();
}

KisNodeSP KisNodeManager::PropertyDialogAccess::colorOverlayMask(KisNodeSP node)
{
    const KisLayerSP layer = qobject_cast<KisLayer *>(node.data());
    if (!layer) {
        return KisNodeSP();
    }
    return layer->colorOverlayMask();
}

KisNodeSP KisNodeManager::NodeUpdateAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

void KisNodeManager::NodeUpdateAccess::updateLayers(KisNodeManager *manager)
{
    manager->m_d->layerManager.layersUpdated();
}

void KisNodeManager::NodeUpdateAccess::updateMasks(KisNodeManager *manager)
{
    manager->m_d->maskManager.masksUpdated();
}

void KisNodeManager::NodeUpdateAccess::updateView(KisNodeManager *manager)
{
    manager->m_d->view->updateGUI();
}

void KisNodeManager::NodeUpdateAccess::notifySelectionChanged(KisNodeManager *manager)
{
    manager->m_d->view->selectionManager()->selectionChanged();
}

bool KisNodeManager::NodeUpdateAccess::isPinnedToTimeline(KisNodeSP node)
{
    return node->isPinnedToTimeline();
}

void KisNodeManager::NodeUpdateAccess::setTimelinePinned(KisNodeManager *manager, bool value)
{
    KisSignalsBlocker blocker(manager->m_d->pinToTimeline);
    manager->m_d->pinToTimeline->setChecked(value);
}

KisNodeList KisNodeManager::NodeUpdateAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->m_d->selectedNodes;
}

void KisNodeManager::NodeUpdateAccess::setNodePinnedToTimeline(KisNodeSP node, bool value)
{
    node->setPinnedToTimeline(value);
}

void KisNodeManager::NavigationAccess::activateNextNode(KisNodeManager *manager, bool siblingsOnly)
{
    manager->activateNextNode(siblingsOnly);
}

void KisNodeManager::NavigationAccess::activatePreviousNode(KisNodeManager *manager, bool siblingsOnly)
{
    manager->activatePreviousNode(siblingsOnly);
}

KisNodeSP KisNodeManager::NavigationAccess::previouslyActiveNode(KisNodeManager *manager)
{
    return manager->m_d->previouslyActiveNode;
}

bool KisNodeManager::NavigationAccess::hasParent(KisNodeSP node)
{
    return bool(node->parent());
}

void KisNodeManager::NavigationAccess::activateNode(KisNodeManager *manager, KisNodeSP node)
{
    manager->slotNonUiActivatedNode(node);
}

KisNodeSP KisNodeManager::NavigationAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

KisNodeSP KisNodeManager::NavigationAccess::nextSibling(KisNodeSP node)
{
    return node->nextSibling();
}

KisNodeSP KisNodeManager::NavigationAccess::previousSibling(KisNodeSP node)
{
    return node->prevSibling();
}

bool KisNodeManager::NavigationAccess::hasChildren(KisNodeSP node)
{
    return node->childCount() > 0;
}

KisNodeSP KisNodeManager::NavigationAccess::firstChild(KisNodeSP node)
{
    return node->firstChild();
}

KisNodeSP KisNodeManager::NavigationAccess::lastChild(KisNodeSP node)
{
    return node->lastChild();
}

KisNodeSP KisNodeManager::NavigationAccess::parentNode(KisNodeSP node)
{
    return node->parent();
}

bool KisNodeManager::NavigationAccess::isHidden(KisNodeManager *manager, KisNodeSP node)
{
    return KisNodeManager::isNodeHidden(node, manager->m_d->nodeDisplayModeAdapter->showGlobalSelectionMask());
}

KisNodeList KisNodeManager::OrderingAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->m_d->selectedNodes;
}

bool KisNodeManager::OrderingAccess::canMoveLayers(KisNodeManager *manager, const KisNodeList &nodes)
{
    return manager->canMoveLayers(nodes);
}

KisNodeSP KisNodeManager::OrderingAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

void KisNodeManager::OrderingAccess::raiseNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode)
{
    const KUndo2MagicString actionName = kundo2_i18n("Raise Nodes");
    KisNodeOperationBatch *batch = manager->m_d->lazyGetNodeOperationBatch(actionName);
    batch->raiseNode(nodes, activeNode);
}

void KisNodeManager::OrderingAccess::lowerNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode)
{
    const KUndo2MagicString actionName = kundo2_i18n("Lower Nodes");
    KisNodeOperationBatch *batch = manager->m_d->lazyGetNodeOperationBatch(actionName);
    batch->lowerNode(nodes, activeNode);
}

KisNodeList KisNodeManager::RemovalAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

KisNodeSP KisNodeManager::RemovalAccess::parentNode(KisNodeSP node)
{
    return node->parent();
}

bool KisNodeManager::RemovalAccess::canModifyLayers(KisNodeManager *manager, const KisNodeList &nodes)
{
    return manager->canModifyLayers(nodes);
}

KisNodeSP KisNodeManager::RemovalAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

void KisNodeManager::RemovalAccess::removeNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode)
{
    const KUndo2MagicString actionName = kundo2_i18n("Remove Nodes");
    KisNodeOperationBatch *batch = manager->m_d->lazyGetNodeOperationBatch(actionName);
    batch->removeNode(nodes, activeNode);
}

KisNodeOperationBatch* KisNodeManager::Private::lazyGetNodeOperationBatch(const KUndo2MagicString &actionName)
{
    KisImageWSP image = view->image();

    if (!nodeOperationBatch ||
            (nodeOperationBatch &&
             (nodeOperationBatch->isEnded() ||
              !nodeOperationBatch->canMergeAction(actionName)))) {

        nodeOperationBatch = new KisNodeOperationBatch(actionName, image, 750);
        nodeOperationBatch->setAutoDelete(true);
    }

    return nodeOperationBatch;
}

KisNodeList KisNodeManager::MirrorAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

bool KisNodeManager::MirrorAccess::isMask(KisNodeSP node)
{
    return node->inherits("KisMask");
}

KisSelectionSP KisNodeManager::MirrorAccess::selection(KisNodeManager *manager)
{
    return manager->m_d->view->selection();
}

KisNodeSP KisNodeManager::MirrorAccess::rootNode(KisNodeManager *manager)
{
    return manager->m_d->view->image()->root();
}

bool KisNodeManager::MirrorAccess::canModifyLayer(KisNodeManager *manager, KisNodeSP node)
{
    return manager->canModifyLayer(node);
}

void KisNodeManager::MirrorAccess::applyToNodes(KisNodeManager *manager,
                                                const KisNodeList &nodes,
                                                Qt::Orientation orientation,
                                                KisSelectionSP selection,
                                                const KUndo2MagicString &actionName)
{
    KisMirrorProcessingVisitor::applyToNodes(manager->m_d->view->image(), nodes, orientation, selection, actionName);
}

void KisNodeManager::MirrorAccess::nodesUpdated(KisNodeManager *manager)
{
    manager->nodesUpdated();
}

void KisNodeManager::Private::saveDeviceAsImage(KisPaintDeviceSP device,
                                                const QString &defaultName,
                                                const QRect &bounds,
                                                qreal xRes,
                                                qreal yRes,
                                                quint8 opacity)
{
    KoFileDialog dialog(view->mainWindowAsQWidget(), KoFileDialog::SaveFile, "savenodeasimage");
    dialog.setCaption(i18n("Export \"%1\"", defaultName));
    dialog.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    dialog.setMimeTypeFilters(KisImportExportManager::supportedMimeTypes(KisImportExportManager::Export));
    QString filename = dialog.filename();

    if (filename.isEmpty()) return;

    QString mimefilter = KisMimeDatabase::mimeTypeForFile(filename, false);

    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());

    KisImageSP dst = new KisImage(doc->createUndoStore(),
                                  bounds.width(),
                                  bounds.height(),
                                  device->compositionSourceColorSpace(),
                                  defaultName);
    dst->setResolution(xRes, yRes);
    doc->setCurrentImage(dst);
    KisPaintLayer* paintLayer = new KisPaintLayer(dst, "paint device", opacity);
    paintLayer->paintDevice()->makeCloneFrom(device, bounds);
    dst->addNode(paintLayer, dst->rootLayer(), KisLayerSP(0));

    dst->initialRefreshGraph();

    if (!doc->exportDocumentSync(filename, mimefilter.toLatin1())) {
        QMessageBox::warning(qApp->activeWindow(),
                             i18nc("@title:window", "LibrePaint"),
                             i18n("Could not save the layer. %1", doc->errorMessage().toUtf8().data()),
                             QMessageBox::Ok);

    }
}

void KisNodeManager::saveNodeAsImage()
{
    KisNodeSP node = activeNode();

    if (!node) {
        warnKrita << "BUG: Save Node As Image was called without any node selected";
        return;
    }

    KisPaintDeviceSP saveDevice = node->projection();

    if (!saveDevice) {
        m_d->view->showFloatingMessage(i18nc("warning message when trying to export a transform mask", "Layer has no pixel data"), QIcon());
        return;
    }

    KisImageSP image = m_d->view->image();
    QRect saveRect = image->bounds() | node->exactBounds();

    m_d->saveDeviceAsImage(saveDevice,
                           node->name(),
                           saveRect,
                           image->xRes(), image->yRes(),
                           node->opacity());
}

#include "SvgWriter.h"

void KisNodeManager::saveVectorLayerAsImage()
{
    KisShapeLayerSP shapeLayer = qobject_cast<KisShapeLayer*>(activeNode().data());
    if (!shapeLayer) {
        return;
    }

    KoFileDialog dialog(m_d->view->mainWindowAsQWidget(), KoFileDialog::SaveFile, "savenodeasimage");
    dialog.setCaption(i18nc("@title:window", "Export to SVG"));
    dialog.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    dialog.setMimeTypeFilters(QStringList() << "image/svg+xml", "image/svg+xml");
    QString filename = dialog.filename();

    if (filename.isEmpty()) return;

    QUrl url = QUrl::fromLocalFile(filename);

    if (url.isEmpty()) return;

    const QSizeF sizeInPx = m_d->view->image()->bounds().size();
    const QSizeF sizeInPt(sizeInPx.width() / m_d->view->image()->xRes(),
                          sizeInPx.height() / m_d->view->image()->yRes());

    QList<KoShape*> shapes = shapeLayer->shapes();
    std::sort(shapes.begin(), shapes.end(), KoShape::compareShapeZIndex);

    SvgWriter writer(shapes);
    if (!writer.save(filename, sizeInPt, true)) {
        QMessageBox::warning(qApp->activeWindow(), i18nc("@title:window", "LibrePaint"), i18n("Could not save to svg: %1", filename));
    }
}

KisNodeSP KisNodeManager::SplitAlphaAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

bool KisNodeManager::SplitAlphaAccess::canModifyLayer(KisNodeManager *manager, KisNodeSP node)
{
    return manager->canModifyLayer(node);
}

bool KisNodeManager::SplitAlphaAccess::hasEditablePaintDevice(KisNodeSP node)
{
    return node->hasEditablePaintDevice();
}

QString KisNodeManager::SplitAlphaAccess::createMaskName(KisNodeManager *manager,
                                                         KisNodeSP node,
                                                         const QString &maskType,
                                                         const QString &defaultName)
{
    return manager->m_d->maskManager.createMaskNameCommon(node, maskType, defaultName);
}

void KisNodeManager::SplitAlphaAccess::splitAlphaToMask(KisNodeSP node, const QString &maskName)
{
    KisLayerUtils::splitAlphaToMask(node->image(), node, maskName);
}

void KisNodeManager::SplitAlphaAccess::mergeTransparencyMaskAsAlpha(KisNodeManager *manager, bool writeToLayers)
{
    manager->m_d->mergeTransparencyMaskAsAlpha(writeToLayers);
}

void KisNodeManager::Private::mergeTransparencyMaskAsAlpha(bool writeToLayers)
{
    KisNodeSP node = q->activeNode();
    KisNodeSP parentNode = node->parent();

    // guaranteed by KisActionManager
    KIS_ASSERT_RECOVER_RETURN(node->inherits("KisTransparencyMask"));

    if (writeToLayers && (!parentNode->hasEditablePaintDevice() || !node->isEditable(false))) {
        QMessageBox::information(view->mainWindowAsQWidget(),
                                 i18nc("@title:window", "Layer %1 is not editable", parentNode->name()),
                                 i18n("Cannot write alpha channel of "
                                      "the parent layer \"%1\".\n"
                                      "The operation will be cancelled.", parentNode->name()));
        return;
    }

    KisPaintDeviceSP dstDevice;
    if (writeToLayers) {
        KIS_ASSERT_RECOVER_RETURN(parentNode->paintDevice());
        dstDevice = parentNode->paintDevice();
    } else {
        KisPaintDeviceSP copyDevice = parentNode->paintDevice();
        if (!copyDevice) {
            copyDevice = parentNode->original();
        }
        dstDevice = new KisPaintDevice(*copyDevice);
    }

    const KoColorSpace *dstCS = dstDevice->colorSpace();

    KisPaintDeviceSP selectionDevice = node->paintDevice();
    KIS_ASSERT_RECOVER_RETURN(selectionDevice->colorSpace()->pixelSize() == 1);

    const QRect processRect =
            selectionDevice->exactBounds() |
            dstDevice->exactBounds() |
            selectionDevice->defaultBounds()->bounds();

    QScopedPointer<KisTransaction> transaction;

    if (writeToLayers) {
        commandsAdapter.beginMacro(kundo2_i18n("Write Alpha into a Layer"));
        transaction.reset(new KisTransaction(kundo2_noi18n("__write_alpha_channel__"), dstDevice));
    }

    KisSequentialIterator srcIt(selectionDevice, processRect);
    KisSequentialIterator dstIt(dstDevice, processRect);

    while (srcIt.nextPixel() && dstIt.nextPixel()) {
        quint8 *alpha8Ptr = srcIt.rawData();
        quint8 *dstPtr = dstIt.rawData();

        dstCS->setOpacity(dstPtr, *alpha8Ptr, 1);
    }

    if (writeToLayers) {
        commandsAdapter.addExtraCommand(transaction->endAndTake());
        commandsAdapter.removeNode(node);
        commandsAdapter.endMacro();
    } else {
        KisImageWSP image = view->image();
        QRect saveRect = image->bounds();

        saveDeviceAsImage(dstDevice, parentNode->name(),
                          saveRect,
                          image->xRes(), image->yRes(),
                          OPACITY_OPAQUE_U8);
    }
}
KisNodeList KisNodeManager::ToggleAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

KisNodeSP KisNodeManager::ToggleAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

bool KisNodeManager::ToggleAccess::supportsProperty(KisNodeSP node, ToggleProperty property)
{
    switch (property) {
    case ToggleProperty::Locked:
    case ToggleProperty::Visible:
        return true;
    case ToggleProperty::AlphaLocked:
        return qobject_cast<KisPaintLayer *>(node.data());
    case ToggleProperty::InheritAlpha:
        return qobject_cast<KisLayer *>(node.data());
    }

    Q_UNREACHABLE_RETURN(false);
}

bool KisNodeManager::ToggleAccess::propertyState(KisNodeSP node, ToggleProperty property)
{
    switch (property) {
    case ToggleProperty::Locked:
        return node->userLocked();
    case ToggleProperty::Visible:
        return node->visible();
    case ToggleProperty::AlphaLocked:
        return qobject_cast<KisPaintLayer *>(node.data())->alphaLocked();
    case ToggleProperty::InheritAlpha:
        return qobject_cast<KisLayer *>(node.data())->alphaChannelDisabled();
    }

    Q_UNREACHABLE_RETURN(false);
}

void KisNodeManager::ToggleAccess::setProperty(KisNodeManager *manager,
                                               KisNodeSP node,
                                               ToggleProperty property,
                                               bool value)
{
    switch (property) {
    case ToggleProperty::Locked:
        KisLayerPropertiesIcons::setNodePropertyAutoUndo(node,
                                                         KisLayerPropertiesIcons::locked,
                                                         value,
                                                         manager->m_d->view->image());
        return;
    case ToggleProperty::Visible:
        KisLayerPropertiesIcons::setNodePropertyAutoUndo(node,
                                                         KisLayerPropertiesIcons::visible,
                                                         value,
                                                         manager->m_d->view->image());
        return;
    case ToggleProperty::AlphaLocked:
        KisLayerPropertiesIcons::setNodePropertyAutoUndo(node,
                                                         KisLayerPropertiesIcons::alphaLocked,
                                                         value,
                                                         manager->m_d->view->image());
        return;
    case ToggleProperty::InheritAlpha:
        KisLayerPropertiesIcons::setNodePropertyAutoUndo(node,
                                                         KisLayerPropertiesIcons::inheritAlpha,
                                                         value,
                                                         manager->m_d->view->image());
        return;
    }

    Q_UNREACHABLE();
}

KisNodeList KisNodeManager::ClipboardAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

KisNodeSP KisNodeManager::ClipboardAccess::parentNode(KisNodeSP node)
{
    return node->parent();
}

void KisNodeManager::ClipboardAccess::setLayers(KisNodeManager *manager, const KisNodeList &nodes, bool copy)
{
    KisClipboard::instance()->setLayers(nodes, manager->m_d->view->image(), copy);
}

bool KisNodeManager::ClipboardAccess::canModifyLayers(KisNodeManager *manager, const KisNodeList &nodes)
{
    return manager->canModifyLayers(nodes);
}

void KisNodeManager::ClipboardAccess::removeNodes(KisNodeManager *manager,
                                                  const KisNodeList &nodes,
                                                  const KUndo2MagicString &actionName)
{
    KisNodeOperationBatch *batch = manager->m_d->lazyGetNodeOperationBatch(actionName);
    batch->removeNode(nodes, manager->activeNode());
}

const QMimeData *KisNodeManager::ClipboardAccess::layersMimeData()
{
    return KisClipboard::instance()->layersMimeData();
}

KisNodeSP KisNodeManager::ClipboardAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

KisNodeSP KisNodeManager::ClipboardAccess::rootNode(KisNodeManager *manager)
{
    return manager->m_d->view->image()->root();
}

void KisNodeManager::ClipboardAccess::insertMimeLayersAsLastChild(KisNodeManager *manager,
                                                                  const QMimeData *data,
                                                                  KisNodeSP targetNode,
                                                                  bool copyNode,
                                                                  bool changeOffset,
                                                                  QPointF offset,
                                                                  KisProcessingApplicator *applicator)
{
    KisShapeController *shapeController =
        dynamic_cast<KisShapeController *>(manager->m_d->imageView->document()->shapeController());
    Q_ASSERT(shapeController);

    KisDummiesFacadeBase *dummiesFacade =
        dynamic_cast<KisDummiesFacadeBase *>(manager->m_d->imageView->document()->shapeController());
    Q_ASSERT(dummiesFacade);

    KisImageSP image = manager->m_d->view->image();
    KisNodeDummy *parentDummy = dummiesFacade->dummyForNode(targetNode);
    KisNodeDummy *aboveThisDummy = parentDummy ? parentDummy->lastChild() : 0;

    KisMimeData::insertMimeLayers(data,
                                  image,
                                  shapeController,
                                  parentDummy,
                                  aboveThisDummy,
                                  copyNode,
                                  manager->nodeInsertionAdapter(),
                                  changeOffset,
                                  offset,
                                  applicator);
}

KisNodeOperationBatch *KisNodeManager::QuickGroupAccess::operationBatch(KisNodeManager *manager,
                                                                        const KUndo2MagicString &actionName)
{
    return manager->m_d->lazyGetNodeOperationBatch(actionName);
}

KisNodeSP KisNodeManager::QuickGroupAccess::activeNode(KisNodeManager *manager)
{
    return manager->activeNode();
}

bool KisNodeManager::QuickGroupAccess::canMoveLayer(KisNodeManager *manager, KisNodeSP node)
{
    return manager->canMoveLayer(node);
}

QString KisNodeManager::QuickGroupAccess::nextLayerName(KisNodeManager *manager, const QString &defaultName)
{
    return manager->m_d->view->image()->nextLayerName(defaultName);
}

KisNodeList KisNodeManager::QuickGroupAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

bool KisNodeManager::QuickGroupAccess::createGroup(KisNodeOperationBatch *batch,
                                                   const KisNodeList &nodes,
                                                   KisNodeSP activeNode,
                                                   const QString &groupName,
                                                   KisNodeSP *newGroup,
                                                   KisNodeSP *newLastChild)
{
    return batch->createGroup(nodes, activeNode, groupName, newGroup, newLastChild);
}

void KisNodeManager::QuickGroupAccess::addClippingMask(KisNodeManager *manager,
                                                       KisNodeOperationBatch *batch,
                                                       KisNodeSP parent,
                                                       KisNodeSP above,
                                                       const QString &maskName)
{
    KisImageSP image = manager->m_d->view->image();
    KisPaintLayerSP maskLayer = new KisPaintLayer(image.data(), maskName, OPACITY_OPAQUE_U8, image->colorSpace());
    maskLayer->disableAlphaChannel(true);
    batch->addNode(KisNodeList() << maskLayer, parent, above, manager->activeNode());
}

bool KisNodeManager::QuickGroupAccess::canModifyLayer(KisNodeManager *manager, KisNodeSP node)
{
    return manager->canModifyLayer(node);
}

bool KisNodeManager::QuickGroupAccess::ungroupNodes(KisNodeOperationBatch *batch,
                                                    const KisNodeList &nodes,
                                                    KisNodeSP activeNode,
                                                    KisNodeSP *incompatibleNode,
                                                    KisNodeSP *destinationParent)
{
    return batch->ungroupNodes(nodes, activeNode, incompatibleNode, destinationParent);
}

KisNodeSP KisNodeManager::QuickGroupAccess::parentNode(KisNodeSP node)
{
    return node->parent();
}

QString KisNodeManager::QuickGroupAccess::nodeName(KisNodeSP node)
{
    return node->name();
}

void KisNodeManager::QuickGroupAccess::showFloatingMessage(KisNodeManager *manager, const QString &message)
{
    manager->m_d->view->showFloatingMessage(message, QIcon());
}

KisNodeList KisNodeManager::SelectionAccess::findNodes(KisNodeManager *manager, SelectionProperty property, bool value)
{
    KoProperties properties;
    if (property == SelectionProperty::Visible) {
        properties.setProperty("visible", value);
    } else if (property == SelectionProperty::Locked) {
        properties.setProperty("locked", value);
    }

    KisImageSP image = manager->m_d->view->image();
    return KisLayerUtils::findNodesWithProps(image->root(), properties, true);
}

KisNodeList KisNodeManager::SelectionAccess::selectedNodes(KisNodeManager *manager)
{
    return manager->selectedNodes();
}

bool KisNodeManager::SelectionAccess::sameNodesUnordered(const KisNodeList &first, const KisNodeList &second)
{
    return KritaUtils::compareListsUnordered(first, second);
}

void KisNodeManager::SelectionAccess::reselectNodes(KisNodeManager *manager,
                                                    KisNodeSP activeNode,
                                                    const KisNodeList &nodes)
{
    manager->slotImageRequestNodeReselection(activeNode, nodes);
}

void KisNodeManager::slotUiActivateNode()
{
    if (!sender()->property("node").isNull()) {
        QString name = sender()->property("node").toString();
        KisNodeSP node = KisLayerUtils::findNodeByName(m_d->imageView->image()->rootLayer(),name);
        if (node) {
            slotUiActivatedNode(node);
        }
    }
}
