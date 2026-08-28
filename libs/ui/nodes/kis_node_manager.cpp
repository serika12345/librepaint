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

void KisNodeManager::moveNodeAt(KisNodeSP node, KisNodeSP parent, int index)
{
    m_d->commandsAdapter.moveNode(node, parent, index);
}

void KisNodeManager::moveNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    m_d->lazyGetNodeOperationBatch(kundo2_i18n("Move Nodes"))->moveNode(nodes, parent, aboveThis, activeNode());
}

void KisNodeManager::copyNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    m_d->lazyGetNodeOperationBatch(kundo2_i18n("Copy Nodes"))->copyNode(nodes, parent, aboveThis, activeNode());
}

void KisNodeManager::addNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    m_d->lazyGetNodeOperationBatch(kundo2_i18n("Add Nodes"))->addNode(nodes, parent, aboveThis, activeNode());
}

void KisNodeManager::addNodeUndoable(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis)
{
    m_d->commandsAdapter.addNode(node, parent, aboveThis);
}

void KisNodeManager::toggleIsolateActiveNode()
{
    QAction* action = m_d->view->actionManager()->actionByName("isolate_active_layer");
    action->toggle();
}

void KisNodeManager::setIsolateActiveLayerMode(bool checked)
{
    KisImageWSP image = m_d->view->image();
    KIS_ASSERT_RECOVER_RETURN(image);

    const bool groupIsolationState = image->isIsolatingGroup();
    changeIsolationMode(checked, groupIsolationState);
}

void KisNodeManager::setIsolateActiveGroupMode(bool checked)
{
    KisImageWSP image = m_d->view->image();
    KIS_ASSERT_RECOVER_RETURN(image);

    const bool layerIsolationState = image->isIsolatingLayer();
    changeIsolationMode(layerIsolationState, checked);
}

void KisNodeManager::changeIsolationMode(bool isolateActiveLayer, bool isolateActiveGroup)
{
    KisImageWSP image = m_d->view->image();
    KisNodeSP activeNode = this->activeNode();
    KIS_ASSERT_RECOVER_RETURN(image && activeNode);

    if (isolateActiveLayer || isolateActiveGroup) {
        if (image->startIsolatedMode(activeNode, isolateActiveLayer, isolateActiveGroup) == false) {
            reinitializeIsolationActionGroup();
        }
    } else {
        image->stopIsolatedMode();
    }
}

void KisNodeManager::changeIsolationRoot(KisNodeSP isolationRoot)
{
    KisImageWSP image = m_d->view->image();
    if (!image || !isolationRoot) return;

    const bool isIsolatingLayer = image->isIsolatingLayer();
    const bool isIsolatingGroup = image->isIsolatingGroup();

    // Restart isolation with a new root node and the same settings.
    if (image->startIsolatedMode(isolationRoot, isIsolatingLayer, isIsolatingGroup) == false) {
        reinitializeIsolationActionGroup();
    }
}

void KisNodeManager::handleExternalIsolationChange()
{
    // It might be that we have multiple Krita windows open. In such a case
    // only the currently active one should restart isolated mode
    if (!m_d->view->mainWindowAsQWidget()->isActiveWindow()) return;

    KisImageWSP image = m_d->view->image();
    KisNodeSP activeNode = this->activeNode();

    const bool isIsolatingLayer = image->isIsolatingLayer();
    const bool isIsolatingGroup = image->isIsolatingGroup();

    m_d->view->actionManager()->actionByName("isolate_active_layer")->setChecked(isIsolatingLayer);
    m_d->view->actionManager()->actionByName("isolate_active_group")->setChecked(isIsolatingGroup);
}

void KisNodeManager::reinitializeIsolationActionGroup()
{
    m_d->view->actionManager()->actionByName("isolate_active_layer")->setChecked(false);
    m_d->view->actionManager()->actionByName("isolate_active_group")->setChecked(false);
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

void KisNodeManager::createFromVisible()
{
    KisLayerUtils::newLayerFromVisible(m_d->view->image(), m_d->view->image()->root()->lastChild());
}

void KisNodeManager::slotPinToTimeline(bool value)
{
    Q_FOREACH (KisNodeSP node, selectedNodes()) {
        node->setPinnedToTimeline(value);
    }
}

KisLayerSP KisNodeManager::createPaintLayer()
{
    KisNodeSP node = createNode("KisPaintLayer");
    return dynamic_cast<KisLayer*>(node.data());
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

void KisNodeManager::createReferenceImage(bool fromLayer) {
    KisViewManager* m_view = m_d->view;
    KisDocument *document = m_view->document();
    KisCanvas2 *canvas = m_view->canvasBase();
    
    const KisPaintDeviceSP paintDevice = fromLayer ? m_view->activeLayer()->projection()
                                                  : canvas->currentImage()->projection();
    const QImage image = paintDevice->convertToQImage(0, KoColorConversionTransformation::internalRenderingIntent(),
        KoColorConversionTransformation::internalConversionFlags());
    std::unique_ptr<KisReferenceImage> reference(KisReferenceImage::fromQImage(*canvas->coordinatesConverter(), image));
    KIS_SAFE_ASSERT_RECOVER_RETURN(canvas);
    if (reference) {
        if (document->referenceImagesLayer()) {
            reference->setZIndex(document->referenceImagesLayer()->shapes().size());
        }
        canvas->addCommand(KisReferenceImagesLayer::addReferenceImages(document, {reference.release()}));

        KoToolManager::instance()->switchToolRequested("ToolReferenceImages");

    } else {
        if (canvas->canvasWidget()) {
            QString strMessage = fromLayer ? i18nc("error dialog from the reference tool", "Could not create a reference image from the active layer.")
                : i18nc("error dialog from the reference tool", "Could not create a reference image from the visible canvas.");

            m_d->view->showFloatingMessage(strMessage, QIcon(), 5000, KisFloatingMessage::High, Qt::TextSingleLine);
        }
    }
}

void KisNodeManager::createReferenceImageFromLayer() {
    createReferenceImage(true);
}

void KisNodeManager::createReferenceImageFromVisible() {
    createReferenceImage(false);
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

void KisNodeManager::nodesUpdated()
{
    KisNodeSP node = activeNode();
    if (!node) return;

    m_d->layerManager.layersUpdated();
    m_d->maskManager.masksUpdated();

    m_d->view->updateGUI();
    m_d->view->selectionManager()->selectionChanged();

    {
        KisSignalsBlocker b(m_d->pinToTimeline);
        m_d->pinToTimeline->setChecked(node->isPinnedToTimeline());
    }
}

void KisNodeManager::nodeProperties(KisNodeSP node)
{
    if ((selectedNodes().size() > 1 && node->inherits("KisLayer")) || node->inherits("KisLayer")) {
        m_d->layerManager.layerProperties();
    }
    else if (node->inherits("KisMask")) {
        m_d->maskManager.maskProperties();
    }
}

void KisNodeManager::nodePropertiesIgnoreSelection(KisNodeSP node)
{
    Q_ASSERT(node);

    // Change the current node temporarily
    KisNodeSP originalNode = m_d->imageView->currentNode();
    m_d->imageView->setCurrentNode(node);

    if (node->inherits("KisLayer")) {
        m_d->layerManager.layerProperties();
    }
    else if (node->inherits("KisMask")) {
        m_d->maskManager.maskProperties();
    }

    m_d->imageView->setCurrentNode(originalNode);
}

void KisNodeManager::changeCloneSource()
{
    m_d->layerManager.changeCloneSource();
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

void KisNodeManager::duplicateActiveNode()
{
    KUndo2MagicString actionName = kundo2_i18n("Duplicate Nodes");
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);
    batch->duplicateNode(selectedNodes(), activeNode());
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

void KisNodeManager::raiseNode()
{
    if (!canMoveLayers(selectedNodes())) return;

    KUndo2MagicString actionName = kundo2_i18n("Raise Nodes");
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);
    batch->raiseNode(selectedNodes(), activeNode());
}

void KisNodeManager::lowerNode()
{
    if (!canMoveLayers(selectedNodes())) return;

    KUndo2MagicString actionName = kundo2_i18n("Lower Nodes");
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);
    batch->lowerNode(selectedNodes(), activeNode());
}

void KisNodeManager::removeSingleNode(KisNodeSP node)
{
    if (!node || !node->parent()) {
        return;
    }

    KisNodeList nodes;
    nodes << node;
    removeSelectedNodes(nodes);
}

void KisNodeManager::removeSelectedNodes(KisNodeList nodes)
{
    if (!canModifyLayers(nodes)) return;

    KUndo2MagicString actionName = kundo2_i18n("Remove Nodes");
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);
    batch->removeNode(nodes, activeNode());
}

void KisNodeManager::removeNode()
{
    removeSelectedNodes(selectedNodes());
}

void KisNodeManager::mirrorNodeX()
{
    KisNodeList nodes = selectedNodes();

    KUndo2MagicString commandName;
    if (nodes.size() == 1 && nodes[0]->inherits("KisMask")) {
        commandName = kundo2_i18n("Mirror Mask Horizontally");
    }
    else {
        commandName = kundo2_i18np("Mirror Layer Horizontally", "Mirror %1 Layers Horizontally", nodes.size());
    }
    mirrorNodes(nodes, commandName, Qt::Horizontal, m_d->view->selection());
}

void KisNodeManager::mirrorNodeY()
{
    KisNodeList nodes = selectedNodes();

    KUndo2MagicString commandName;
    if (nodes.size() == 1 && nodes[0]->inherits("KisMask")) {
        commandName = kundo2_i18n("Mirror Mask Vertically");
    }
    else {
        commandName = kundo2_i18np("Mirror Layer Vertically", "Mirror %1 Layers Vertically", nodes.size());
    }
    mirrorNodes(nodes, commandName, Qt::Vertical, m_d->view->selection());
}

void KisNodeManager::mirrorAllNodesX()
{
    KisNodeSP node = m_d->view->image()->root();
    mirrorNode(node, kundo2_i18n("Mirror All Layers Horizontally"),
               Qt::Horizontal, m_d->view->selection());
}

void KisNodeManager::mirrorAllNodesY()
{
    KisNodeSP node = m_d->view->image()->root();
    mirrorNode(node, kundo2_i18n("Mirror All Layers Vertically"),
               Qt::Vertical, m_d->view->selection());
}

void KisNodeManager::activateNextNode(bool siblingsOnly)
{
    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) return;

    KisNodeSP nextNode = activeNode->nextSibling();

    if (!siblingsOnly) {
        // Recurse groups...
        while (nextNode && nextNode->childCount() > 0) {
            nextNode = nextNode->firstChild();
        }

        // Out of nodes? Back out of group...
        if (!nextNode && activeNode->parent()) {
            nextNode = activeNode->parent();
        }
    }

    // Skip nodes hidden from tree view..
    while (nextNode && isNodeHidden(nextNode, m_d->nodeDisplayModeAdapter->showGlobalSelectionMask())) {
        nextNode = nextNode->nextSibling();
    }

    // Select node, unless root..
    if (nextNode && nextNode->parent()) {
        slotNonUiActivatedNode(nextNode);
    }
}

void KisNodeManager::activateNextSiblingNode()
{
    activateNextNode(true);
}

void KisNodeManager::activatePreviousNode(bool siblingsOnly)
{
    KisNodeSP activeNode = this->activeNode();
    if (!activeNode) return;

    KisNodeSP nextNode = activeNode->prevSibling();

    if (!siblingsOnly) {
        // Enter groups..
        if (activeNode->childCount() > 0) {
            nextNode = activeNode->lastChild();
        }

        // Out of nodes? Back out of group...
        if (!nextNode && activeNode->parent()) {
            nextNode = activeNode->parent()->prevSibling();
        }
    }

    // Skip nodes hidden from tree view..
    while (nextNode && isNodeHidden(nextNode, m_d->nodeDisplayModeAdapter->showGlobalSelectionMask())) {
        nextNode = nextNode->prevSibling();
    }

    // Select node, unless root..
    if (nextNode && nextNode->parent()) {
        slotNonUiActivatedNode(nextNode);
    }
}

void KisNodeManager::activatePreviousSiblingNode()
{
    activatePreviousNode(true);
}

void KisNodeManager::switchToPreviouslyActiveNode()
{
    if (m_d->previouslyActiveNode && m_d->previouslyActiveNode->parent()) {
        slotNonUiActivatedNode(m_d->previouslyActiveNode);
    }
}

void KisNodeManager::mirrorNode(KisNodeSP node,
                                const KUndo2MagicString& actionName,
                                Qt::Orientation orientation,
                                KisSelectionSP selection)
{
    KisNodeList nodes = {node};
    mirrorNodes(nodes, actionName, orientation, selection);
}

void KisNodeManager::mirrorNodes(KisNodeList nodes,
                                const KUndo2MagicString& actionName,
                                Qt::Orientation orientation,
                                KisSelectionSP selection)
{
    Q_FOREACH(KisNodeSP node, nodes) {
        if (!canModifyLayer(node)) return;
    }

    KisMirrorProcessingVisitor::applyToNodes(
        m_d->view->image(), nodes, orientation, selection, actionName);

    nodesUpdated();
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

void KisNodeManager::slotSplitAlphaIntoMask()
{
    KisNodeSP node = activeNode();
    if (!canModifyLayer(node)) return;

    // guaranteed by KisActionManager
    KIS_ASSERT_RECOVER_RETURN(node->hasEditablePaintDevice());

    KisLayerUtils::splitAlphaToMask(node->image(), node, m_d->maskManager.createMaskNameCommon(node, "KisTransparencyMask",  i18n("Transparency Mask")));
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


void KisNodeManager::slotSplitAlphaWrite()
{
    m_d->mergeTransparencyMaskAsAlpha(true);
}

void KisNodeManager::slotSplitAlphaSaveMerged()
{
    m_d->mergeTransparencyMaskAsAlpha(false);
}

void KisNodeManager::toggleLock()
{
    KisNodeList nodes = this->selectedNodes();
    KisNodeSP active = activeNode();
    if (nodes.isEmpty() || !active) return;

    bool isLocked = active->userLocked();

    for (auto &node : nodes) {
        KisLayerPropertiesIcons::setNodePropertyAutoUndo(node, KisLayerPropertiesIcons::locked, !isLocked, m_d->view->image());
    }
}

void KisNodeManager::toggleVisibility()
{
    KisNodeList nodes = this->selectedNodes();
    KisNodeSP active = activeNode();
    if (nodes.isEmpty() || !active) return;

    bool isVisible = active->visible();

    for (auto &node : nodes) {
        KisLayerPropertiesIcons::setNodePropertyAutoUndo(node, KisLayerPropertiesIcons::visible, !isVisible, m_d->view->image());
    }
}

void KisNodeManager::toggleAlphaLock()
{
    KisNodeList nodes = this->selectedNodes();
    KisNodeSP active = activeNode();
    if (nodes.isEmpty() || !active) return;

    auto layer = qobject_cast<KisPaintLayer*>(active.data());
    if (!layer) {
        return;
    }

    bool isAlphaLocked = layer->alphaLocked();
    for (auto &node : nodes) {
        auto layer = qobject_cast<KisPaintLayer*>(node.data());
        if (layer) {
            KisLayerPropertiesIcons::setNodePropertyAutoUndo(node, KisLayerPropertiesIcons::alphaLocked, !isAlphaLocked, m_d->view->image());
        }
    }
}

void KisNodeManager::toggleInheritAlpha()
{
    KisNodeList nodes = this->selectedNodes();
    KisNodeSP active = activeNode();
    if (nodes.isEmpty() || !active) return;

    auto layer = qobject_cast<KisLayer*>(active.data());
    if (!layer) {
        return;
    }

    bool isAlphaDisabled = layer->alphaChannelDisabled();
    for (auto &node : nodes) {
        auto layer = qobject_cast<KisLayer*>(node.data());
        if (layer) {
            KisLayerPropertiesIcons::setNodePropertyAutoUndo(node, KisLayerPropertiesIcons::inheritAlpha, !isAlphaDisabled, m_d->view->image());
        }
    }
}

void KisNodeManager::colorOverlayMaskProperties(KisNodeSP node)
{
    Q_ASSERT(node);
    KisLayerSP layer = qobject_cast<KisLayer*>(node.data());
    if (!layer) {
        return;
    }

    KisFilterMaskSP mask = layer->colorOverlayMask();
    if (!mask) {
        // This layer does not use fast color overlay mask.
        return;
    }

    nodePropertiesIgnoreSelection(mask);
}

void KisNodeManager::cutLayersToClipboard()
{
    KisNodeList nodes = this->selectedNodes();
    if (nodes.isEmpty()) return;

    KisNodeList::Iterator it = nodes.begin();
    while (it != nodes.end()) {
        // make sure the deleted nodes aren't referenced here again
        if (!it->data()->parent()) {
            nodes.erase(it);
        }
        it++;
    }

    KisClipboard::instance()->setLayers(nodes, m_d->view->image(), false);

    if (canModifyLayers(nodes)) {
        KUndo2MagicString actionName = kundo2_i18n("Cut Nodes");
        KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);
        batch->removeNode(nodes, activeNode());
    }
}

void KisNodeManager::copyLayersToClipboard()
{
    KisNodeList nodes = this->selectedNodes();
    KisClipboard::instance()->setLayers(nodes, m_d->view->image(), true);
}

void KisNodeManager::pasteLayersFromClipboard(bool changeOffset, QPointF offset, KisProcessingApplicator *applicator)
{
    const QMimeData *data = KisClipboard::instance()->layersMimeData();
    if (!data) return;

    KisNodeSP activeNode = this->activeNode();

    KisShapeController *shapeController = dynamic_cast<KisShapeController*>(m_d->imageView->document()->shapeController());
    Q_ASSERT(shapeController);

    KisDummiesFacadeBase *dummiesFacade = dynamic_cast<KisDummiesFacadeBase*>(m_d->imageView->document()->shapeController());
    Q_ASSERT(dummiesFacade);

    const bool copyNode = false;
    KisImageSP image = m_d->view->image();
    KisNodeDummy *parentDummy = dummiesFacade->dummyForNode(activeNode ? activeNode : image->root());
    KisNodeDummy *aboveThisDummy = parentDummy ? parentDummy->lastChild() : 0;

    KisMimeData::insertMimeLayers(data,
                                  image,
                                  shapeController,
                                  parentDummy,
                                  aboveThisDummy,
                                  copyNode,
                                  nodeInsertionAdapter(),
                                  changeOffset,
                                  offset,
                                  applicator);
}

bool KisNodeManager::createQuickGroupImpl(KisNodeOperationBatch *batch,
                                          const QString &overrideGroupName,
                                          KisNodeSP *newGroup,
                                          KisNodeSP *newLastChild)
{
    KisNodeSP active = activeNode();
    if (!active) return false;

    if (!canMoveLayer(active)) return false;

    KisImageSP image = m_d->view->image();
    const QString groupName = !overrideGroupName.isEmpty()
        ? overrideGroupName
        : image->nextLayerName(i18nc("A group of layers", "Group"));

    return batch->createGroup(selectedNodes(), active, groupName, newGroup, newLastChild);
}

void KisNodeManager::createQuickGroup()
{
    KUndo2MagicString actionName = kundo2_i18n("Quick Group");
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);

    KisNodeSP parent;
    KisNodeSP above;

    createQuickGroupImpl(batch, "", &parent, &above);
}

void KisNodeManager::createQuickClippingGroup()
{
    KUndo2MagicString actionName = kundo2_i18n("Quick Clipping Group");
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);

    KisNodeSP parent;
    KisNodeSP above;

    KisImageSP image = m_d->view->image();
    if (createQuickGroupImpl(batch, image->nextLayerName(i18nc("default name for a clipping group layer", "Clipping Group")), &parent, &above)) {
        KisPaintLayerSP maskLayer = new KisPaintLayer(image.data(), i18nc("default name for quick clip group mask layer", "Mask Layer"), OPACITY_OPAQUE_U8, image->colorSpace());
        maskLayer->disableAlphaChannel(true);

        batch->addNode(KisNodeList() << maskLayer, parent, above, activeNode());
    }
}

void KisNodeManager::quickUngroup()
{
    KisNodeSP active = activeNode();
    if (!active) return;

    if (!canModifyLayer(active)) return;

    KUndo2MagicString actionName = kundo2_i18n("Quick Ungroup");

    KisNodeSP incompatibleNode;
    KisNodeSP destinationParent;
    KisNodeOperationBatch *batch = m_d->lazyGetNodeOperationBatch(actionName);
    if (!batch->ungroupNodes(selectedNodes(), active, &incompatibleNode, &destinationParent) &&
        incompatibleNode && destinationParent) {
        const QString message = destinationParent->parent()
            ? i18n("Cannot move layer \"%1\" into new parent \"%2\"",
                   incompatibleNode->name(), destinationParent->name())
            : i18n("Cannot move layer \"%1\" into the root layer",
                   incompatibleNode->name());
        m_d->view->showFloatingMessage(message, QIcon());
    }
}

void KisNodeManager::selectLayersImpl(const KoProperties &props, const KoProperties &invertedProps)
{
    KisImageSP image = m_d->view->image();
    KisNodeList nodes = KisLayerUtils::findNodesWithProps(image->root(), props, true);

    KisNodeList selectedNodes = this->selectedNodes();

    if (KritaUtils::compareListsUnordered(nodes, selectedNodes)) {
        nodes = KisLayerUtils::findNodesWithProps(image->root(), invertedProps, true);
    }

    if (!nodes.isEmpty()) {
        slotImageRequestNodeReselection(nodes.last(), nodes);
    }
}

void KisNodeManager::selectAllNodes()
{
    KoProperties props;
    selectLayersImpl(props, props);
}

void KisNodeManager::selectVisibleNodes()
{
    KoProperties props;
    props.setProperty("visible", true);

    KoProperties invertedProps;
    invertedProps.setProperty("visible", false);

    selectLayersImpl(props, invertedProps);
}

void KisNodeManager::selectLockedNodes()
{
    KoProperties props;
    props.setProperty("locked", true);

    KoProperties invertedProps;
    invertedProps.setProperty("locked", false);

    selectLayersImpl(props, invertedProps);
}

void KisNodeManager::selectInvisibleNodes()
{
    KoProperties props;
    props.setProperty("visible", false);

    KoProperties invertedProps;
    invertedProps.setProperty("visible", true);

    selectLayersImpl(props, invertedProps);
}

void KisNodeManager::selectUnlockedNodes()
{
    KoProperties props;
    props.setProperty("locked", false);

    KoProperties invertedProps;
    invertedProps.setProperty("locked", true);

    selectLayersImpl(props, invertedProps);
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
