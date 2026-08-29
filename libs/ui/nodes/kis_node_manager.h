/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_NODE_MANAGER
#define KIS_NODE_MANAGER

#include <QObject>
#include <QList>
#include <QAction>

#include "kis_types.h"
#include "kis_base_node.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_mask.h"
#include "kis_selection.h"
#include <kritaui_export.h>

class KisKActionCollection;

class KoCompositeOp;
class KoColorSpace;
class KoShape;
class KUndo2MagicString;
class QMimeData;
class QImage;
class QSizeF;

class KisFilterStrategy;
class KisViewManager;
class KisActionManager;
class KisView;
class KisNodeSelectionAdapter;
class KisNodeInsertionAdapter;
class KisNodeDisplayModeAdapter;
class KisNodeOperationBatch;
class KoProperties;
class KisProcessingApplicator;
class KisReferenceImage;
class KisShapeLayer;
class KisDummiesFacadeBase;

/**
 * The node manager passes requests for new layers or masks on to the mask and layer
 * managers.
 */
class KRITAUI_EXPORT KisNodeManager : public QObject
{

    Q_OBJECT

public:

    KisNodeManager(KisViewManager * view);
    ~KisNodeManager() override;

    void setView(QPointer<KisView>imageView);

Q_SIGNALS:

    /// emitted whenever a node is selected.
    void sigNodeActivated(KisNodeSP node);

    /// for the layer box: this sets the current node in the layerbox
    /// without telling the node manager that the node is activated,
    /// preventing loops (I think...)
    void sigUiNeedChangeActiveNode(KisNodeSP node);

    void sigUiNeedChangeSelectedNodes(const QList<KisNodeSP> &nodes);

public:

    void setup(KisKActionCollection * collection, KisActionManager* actionManager);
    void updateGUI();
    void updateImageNodeSettings(KisImageWSP image);
    void createNodeActivationActions(KisImageWSP image, KisKActionCollection *collection);

    /// Convenience function to get the active layer or mask
    KisNodeSP activeNode();
    bool activeNodeIsAnimated();
    bool activeSelectionIsEditable();

    KisNodeSP nearestNodeAfterRemoval(KisNodeSP node) const;
    KisLayerSP layerForNode(KisNodeSP node) const;
    KisMaskSP maskForNode(KisNodeSP node) const;
    KisSelectionSP selectionForNode(KisNodeSP node, KisImageWSP image) const;

    bool activeNodeIsLayer();
    bool activeNodeInherits(const QString &type);
    bool activeNodeIsEditable();
    bool activeNodeHasEditablePaintDevice();

    /// convenience function to get the active layer. If a mask is
    /// active, it's parent layer is the active layer.
    KisLayerSP activeLayer();

    /// Get the paint device the user wants to paint on now
    KisPaintDeviceSP activePaintDevice();

    /**
     * @return the active color space used for composition, meaning the color space
     * of the active mask, or the color space of the parent of the active layer
     */
    const KoColorSpace* activeColorSpace();

    /**
     * Sets the name for the node in a universal way (masks/layers)
     */
    void setNodeName(KisNodeSP node, const QString &name);

    /**
     * Sets opacity for the node in a universal way (masks/layers)
     */
    void setNodeOpacity(KisNodeSP node, qint32 opacity);

    /**
     * Sets compositeOp for the node in a universal way (masks/layers)
     */
    void setNodeCompositeOp(KisNodeSP node, const KoCompositeOp* compositeOp);

    KisNodeList selectedNodes();

    KisNodeSelectionAdapter* nodeSelectionAdapter() const;
    KisNodeInsertionAdapter* nodeInsertionAdapter() const;
    KisNodeDisplayModeAdapter* nodeDisplayModeAdapter() const;

    static bool isNodeHidden(KisNodeSP node, bool isGlobalSelectionHidden);

    bool trySetNodeProperties(KisNodeSP node, KisImageSP image, KisBaseNode::PropertyList properties) const;


    bool canModifyLayers(KisNodeList nodes, bool showWarning = true);
    bool canModifyLayer(KisNodeSP node, bool showWarning = true);

    bool canMoveLayers(KisNodeList nodes, bool showWarning = true);
    bool canMoveLayer(KisNodeSP node, bool showWarning = true);

public Q_SLOTS:

    /**
     * Explicitly activates \p node
     * The UI will be noticed that active node has been changed.
     * Both sigNodeActivated and sigUiNeedChangeActiveNode are emitted.
     *
     * WARNING: normally you needn't call this method manually. It is
     * automatically called when a node is added to the graph. If you
     * have some special cases when you need to activate a node, consider
     * adding them to KisDummiesFacadeBase instead. Calling this method
     * directly  should be the last resort.
     *
     * \see slotUiActivatedNode for comparison
     */
    void slotNonUiActivatedNode(KisNodeSP node);

    /**
     * Activates \p node in response to a UI selection and selects a compatible
     * vector or pixel tool when necessary. After the active node changes, both
     * sigNodeActivated and sigUiNeedChangeActiveNode are emitted.
     *
     * \see activateNode
     */
    void slotUiActivatedNode(KisNodeSP node);

    /**
     * Adds a list of nodes without searching appropriate position for
     * it.  You *must* ensure that the nodes are allowed to be added
     * to the parent, otherwise you'll get an assert.
     */
    void addNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis);

    /**
     * Adds one node immediately through the image undo stack.
     */
    void addNodeUndoable(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis);

    /**
     * Moves a list of nodes without searching appropriate position
     * for it.  You *must* ensure that the nodes are allowed to be
     * added to the parent, otherwise you'll get an assert.
     */
    void moveNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis);

    /**
     * Copies a list of nodes without searching appropriate position
     * for it.  You *must* ensure that the nodes are allowed to be
     * added to the parent, otherwise you'll get an assert.
     */
    void copyNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis);

    /**
     * Create new layer from actually visible
     */
    void createFromVisible();

    void slotPinToTimeline(bool value);

    // Isolation Mode..

    void toggleIsolateActiveNode();
    void setIsolateActiveLayerMode(bool checked);
    void setIsolateActiveGroupMode(bool checked);

    void changeIsolationMode(bool isolateActiveLayer, bool isolateActiveGroup);
    void changeIsolationRoot(KisNodeSP isolationRoot);

    /**
     * Responds to external changes in isolation mode (i.e. from KisImage).
     */
    void handleExternalIsolationChange();
    void reinitializeIsolationActionGroup();

    // General Node Management..

    void moveNodeAt(KisNodeSP node, KisNodeSP parent, int index);
    KisNodeSP createNode(const QString& nodeType, bool quiet = false, KisPaintDeviceSP copyFrom = 0);
    void convertNode(const QString &nodeType);
    void createReferenceImage(bool fromLayer);
    void createReferenceImageFromLayer();
    void createReferenceImageFromVisible();
    void nodesUpdated();
    void nodeProperties(KisNodeSP node);
    void nodePropertiesIgnoreSelection(KisNodeSP node);
    /// pop up a window for changing the source of the selected Clone Layers
    void changeCloneSource();
    void nodeOpacityChanged(qreal opacity);
    void nodeCompositeOpChanged(const KoCompositeOp* op);
    void duplicateActiveNode();
    void removeNode();
    void mirrorNodeX();
    void mirrorNodeY();
    void mirrorAllNodesX();
    void mirrorAllNodesY();

    void mirrorNode(KisNodeSP node, const KUndo2MagicString& commandName, Qt::Orientation orientation, KisSelectionSP selection);
    void mirrorNodes(KisNodeList nodes, const KUndo2MagicString& commandName, Qt::Orientation orientation, KisSelectionSP selection);

    void activateNextNode(bool siblingsOnly = false);
    void activateNextSiblingNode();
    void activatePreviousNode(bool siblingsOnly = false);
    void activatePreviousSiblingNode();
    void switchToPreviouslyActiveNode();

    /**
     * move the active node up the nodestack.
     */
    void raiseNode();

    /**
     * move the active node down the nodestack
     */
    void lowerNode();

    void saveNodeAsImage();
    void saveVectorLayerAsImage();

    void slotSplitAlphaIntoMask();
    void slotSplitAlphaWrite();
    void slotSplitAlphaSaveMerged();

    void toggleLock();
    void toggleVisibility();
    void toggleAlphaLock();
    void toggleInheritAlpha();

    /** Open a dialog to change layer's fast color overlay mask properties. */
    void colorOverlayMaskProperties(KisNodeSP node);

    /**
     * @brief slotSetSelectedNodes set the list of nodes selected in the layerbox. Selected nodes are not necessarily active nodes.
     * @param nodes the selected nodes
     */
    void slotSetSelectedNodes(const KisNodeList &nodes);

    void slotImageRequestNodeReselection(KisNodeSP activeNode, const KisNodeList &selectedNodes);

    void cutLayersToClipboard();
    void copyLayersToClipboard();
    void pasteLayersFromClipboard(bool changeOffset = false, QPointF offset = QPointF(), KisProcessingApplicator *applicator = nullptr);

    void createQuickGroup();
    void createQuickClippingGroup();
    void quickUngroup();

    void selectAllNodes();
    void selectVisibleNodes();
    void selectLockedNodes();
    void selectInvisibleNodes();
    void selectUnlockedNodes();

private Q_SLOTS:

    friend class KisNodeActivationActionCreatorVisitor;
    /**
     * @brief slotUiActivateNode inspects the sender to see which node needs to be activated.
     */
    void slotUiActivateNode();


public:
    void removeSingleNode(KisNodeSP node);
    KisLayerSP createPaintLayer();

protected:
    struct KRITAUI_EXPORT ImageStateAccess {
        static KisNodeSP nearestNodeAfterRemoval(KisNodeSP node);
        static bool isAnimated(KisNodeSP node);
        static KisNodeSP activeLayerNode(KisNodeManager *manager);
        static KisNodeSP selectionMaskNode(KisNodeSP layer);
        static bool isEditable(KisNodeSP mask);
        static void updateImageNodeSettings(KisImageWSP image);
        static void
        createNodeActivationActions(KisImageWSP image, KisKActionCollection *collection, KisNodeManager *manager);
        static bool isLayer(KisNodeSP node);
        static bool isMask(KisNodeSP node);
        static KisNodeSP parentNode(KisNodeSP mask);
        static KisLayerSP toLayer(KisNodeSP node);
        static KisMaskSP toMask(KisNodeSP node);
        static KisSelectionSP selection(KisNodeSP layer);
        static KisSelectionSP globalSelection(KisImageWSP image);
    };

    struct KRITAUI_EXPORT NodeStateAccess {
        static bool isLayer(KisNodeSP node);
        static bool inheritsType(KisNodeSP node, const QString &type);
        static bool isEditable(KisNodeSP node);
        static bool hasEditablePaintDevice(KisNodeSP node);
        static bool isFakeNode(KisNodeSP node);
        static bool isSelectionMask(KisNodeSP node);
        static KisNodeSP parentNode(KisNodeSP node);
    };

    struct KRITAUI_EXPORT NodeChangeAccess {
        static QString name(KisNodeSP node);
        static qint32 opacity(KisNodeSP node);
        static const KoCompositeOp *compositeOp(KisNodeSP node);
        static void setName(KisNodeManager *manager, KisNodeSP node, const QString &name);
        static void setOpacity(KisNodeManager *manager, KisNodeSP node, qint32 opacity);
        static void setCompositeOp(KisNodeManager *manager, KisNodeSP node, const KoCompositeOp *compositeOp);
    };

    struct KRITAUI_EXPORT ModificationAccess {
        static bool isEditable(KisNodeSP node);
        static QString name(KisNodeSP node);
        static KisNodeSP parentNode(KisNodeSP node);
        static void showWarning(KisNodeManager *manager, const QString &message);
    };

    struct KRITAUI_EXPORT ActiveAccess {
        static KisNodeSP activeNode(KisNodeManager *manager);
        static KisLayerSP activeLayer(KisNodeManager *manager);
        static bool hasActiveMask(KisNodeManager *manager);
        static KisPaintDeviceSP activeMaskDevice(KisNodeManager *manager);
        static KisPaintDeviceSP activeLayerDevice(KisNodeManager *manager);
        static bool hasActiveMaskDevice(KisNodeManager *manager);
        static const KoColorSpace *activeMaskColorSpace(KisNodeManager *manager);
        static bool hasActiveLayer(KisNodeManager *manager);
        static bool activeLayerHasParent(KisNodeManager *manager);
        static const KoColorSpace *activeLayerParentColorSpace(KisNodeManager *manager);
        static const KoColorSpace *imageColorSpace(KisNodeManager *manager);
    };

    struct KRITAUI_EXPORT AccessorAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static KisNodeSelectionAdapter *nodeSelectionAdapter(const KisNodeManager *manager);
        static KisNodeInsertionAdapter *nodeInsertionAdapter(const KisNodeManager *manager);
        static KisNodeDisplayModeAdapter *nodeDisplayModeAdapter(const KisNodeManager *manager);
    };

    struct KRITAUI_EXPORT SelectionStateAccess {
        static void setSelectedNodes(KisNodeManager *manager, const KisNodeList &nodes);
    };

    struct KRITAUI_EXPORT PropertyAccess {
        static bool isPaintLayer(KisNodeSP node);
        static bool containsOnionSkin(const KisBaseNode::PropertyList &properties);
        static bool hasOpaqueBackground(KisNodeSP node);
        static void showOnionSkinTransparencyWarning(const KisNodeManager *manager);
        static void applyProperties(KisNodeSP node, KisImageSP image, KisBaseNode::PropertyList properties);
    };

    struct KRITAUI_EXPORT PropertyDialogAccess {
        static bool isLayer(KisNodeSP node);
        static bool isMask(KisNodeSP node);
        static void showLayerProperties(KisNodeManager *manager);
        static void showMaskProperties(KisNodeManager *manager);
        static KisNodeSP currentNode(KisNodeManager *manager);
        static void setCurrentNode(KisNodeManager *manager, KisNodeSP node);
        static void changeCloneSource(KisNodeManager *manager);
        static KisNodeSP colorOverlayMask(KisNodeSP node);
    };

    struct KRITAUI_EXPORT NodeUpdateAccess {
        static KisNodeSP activeNode(KisNodeManager *manager);
        static void updateLayers(KisNodeManager *manager);
        static void updateMasks(KisNodeManager *manager);
        static void updateView(KisNodeManager *manager);
        static void notifySelectionChanged(KisNodeManager *manager);
        static bool isPinnedToTimeline(KisNodeSP node);
        static void setTimelinePinned(KisNodeManager *manager, bool value);
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static void setNodePinnedToTimeline(KisNodeSP node, bool value);
    };

    struct KRITAUI_EXPORT NavigationAccess {
        static void activateNextNode(KisNodeManager *manager, bool siblingsOnly);
        static void activatePreviousNode(KisNodeManager *manager, bool siblingsOnly);
        static KisNodeSP previouslyActiveNode(KisNodeManager *manager);
        static bool hasParent(KisNodeSP node);
        static void activateNode(KisNodeManager *manager, KisNodeSP node);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static KisNodeSP nextSibling(KisNodeSP node);
        static KisNodeSP previousSibling(KisNodeSP node);
        static bool hasChildren(KisNodeSP node);
        static KisNodeSP firstChild(KisNodeSP node);
        static KisNodeSP lastChild(KisNodeSP node);
        static KisNodeSP parentNode(KisNodeSP node);
        static bool isHidden(KisNodeManager *manager, KisNodeSP node);
    };

    struct KRITAUI_EXPORT OrderingAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static bool canMoveLayers(KisNodeManager *manager, const KisNodeList &nodes);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static void raiseNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode);
        static void lowerNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode);
    };

    struct KRITAUI_EXPORT RemovalAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static KisNodeSP parentNode(KisNodeSP node);
        static bool canModifyLayers(KisNodeManager *manager, const KisNodeList &nodes);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static void removeNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode);
    };

    struct KRITAUI_EXPORT TreeOperationAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static void moveNodeAt(KisNodeManager *manager, KisNodeSP node, KisNodeSP parent, int index);
        static void moveNodes(KisNodeManager *manager,
                              const KisNodeList &nodes,
                              KisNodeSP parent,
                              KisNodeSP aboveThis,
                              KisNodeSP activeNode);
        static void copyNodes(KisNodeManager *manager,
                              const KisNodeList &nodes,
                              KisNodeSP parent,
                              KisNodeSP aboveThis,
                              KisNodeSP activeNode);
        static void addNodes(KisNodeManager *manager,
                             const KisNodeList &nodes,
                             KisNodeSP parent,
                             KisNodeSP aboveThis,
                             KisNodeSP activeNode);
        static void addNodeUndoable(KisNodeManager *manager, KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis);
        static void duplicateNodes(KisNodeManager *manager, const KisNodeList &nodes, KisNodeSP activeNode);
    };

    struct KRITAUI_EXPORT IsolationAccess {
        static bool imageAvailable(KisNodeManager *manager);
        static bool isIsolatingLayer(KisNodeManager *manager);
        static bool isIsolatingGroup(KisNodeManager *manager);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static bool startIsolatedMode(KisNodeManager *manager,
                                      KisNodeSP isolationRoot,
                                      bool isolateActiveLayer,
                                      bool isolateActiveGroup);
        static void stopIsolatedMode(KisNodeManager *manager);
        static bool isActiveWindow(KisNodeManager *manager);
        static void toggleLayerAction(KisNodeManager *manager);
        static void setLayerActionChecked(KisNodeManager *manager, bool checked);
        static void setGroupActionChecked(KisNodeManager *manager, bool checked);
    };

    enum class SelectionProperty {
        Any,
        Visible,
        Locked,
    };

    struct KRITAUI_EXPORT SelectionAccess {
        static KisNodeList findNodes(KisNodeManager *manager, SelectionProperty property, bool value);
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static bool sameNodesUnordered(const KisNodeList &first, const KisNodeList &second);
        static void reselectNodes(KisNodeManager *manager, KisNodeSP activeNode, const KisNodeList &nodes);
    };

    enum class ToggleProperty {
        Locked,
        Visible,
        AlphaLocked,
        InheritAlpha,
    };

    struct KRITAUI_EXPORT ToggleAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static bool supportsProperty(KisNodeSP node, ToggleProperty property);
        static bool propertyState(KisNodeSP node, ToggleProperty property);
        static void setProperty(KisNodeManager *manager, KisNodeSP node, ToggleProperty property, bool value);
    };

    struct KRITAUI_EXPORT MirrorAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static bool isMask(KisNodeSP node);
        static KisSelectionSP selection(KisNodeManager *manager);
        static KisNodeSP rootNode(KisNodeManager *manager);
        static bool canModifyLayer(KisNodeManager *manager, KisNodeSP node);
        static void applyToNodes(KisNodeManager *manager,
                                 const KisNodeList &nodes,
                                 Qt::Orientation orientation,
                                 KisSelectionSP selection,
                                 const KUndo2MagicString &actionName);
        static void nodesUpdated(KisNodeManager *manager);
    };

    struct KRITAUI_EXPORT ClipboardAccess {
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static KisNodeSP parentNode(KisNodeSP node);
        static void setLayers(KisNodeManager *manager, const KisNodeList &nodes, bool copy);
        static bool canModifyLayers(KisNodeManager *manager, const KisNodeList &nodes);
        static void removeNodes(KisNodeManager *manager, const KisNodeList &nodes, const KUndo2MagicString &actionName);
        static const QMimeData *layersMimeData();
        static KisNodeSP activeNode(KisNodeManager *manager);
        static KisNodeSP rootNode(KisNodeManager *manager);
        static void insertMimeLayersAsLastChild(KisNodeManager *manager,
                                                const QMimeData *data,
                                                KisNodeSP targetNode,
                                                bool copyNode,
                                                bool changeOffset,
                                                QPointF offset,
                                                KisProcessingApplicator *applicator);
    };

    struct KRITAUI_EXPORT SplitAlphaAccess {
        static KisNodeSP activeNode(KisNodeManager *manager);
        static bool canModifyLayer(KisNodeManager *manager, KisNodeSP node);
        static bool hasEditablePaintDevice(KisNodeSP node);
        static QString
        createMaskName(KisNodeManager *manager, KisNodeSP node, const QString &maskType, const QString &defaultName);
        static void splitAlphaToMask(KisNodeSP node, const QString &maskName);
        static void mergeTransparencyMaskAsAlpha(KisNodeManager *manager, bool writeToLayers);
    };

    struct KRITAUI_EXPORT QuickGroupAccess {
        static KisNodeOperationBatch *operationBatch(KisNodeManager *manager, const KUndo2MagicString &actionName);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static bool canMoveLayer(KisNodeManager *manager, KisNodeSP node);
        static QString nextLayerName(KisNodeManager *manager, const QString &defaultName);
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static bool createGroup(KisNodeOperationBatch *batch,
                                const KisNodeList &nodes,
                                KisNodeSP activeNode,
                                const QString &groupName,
                                KisNodeSP *newGroup,
                                KisNodeSP *newLastChild);
        static void addClippingMask(KisNodeManager *manager,
                                    KisNodeOperationBatch *batch,
                                    KisNodeSP parent,
                                    KisNodeSP above,
                                    const QString &maskName);
        static bool canModifyLayer(KisNodeManager *manager, KisNodeSP node);
        static bool ungroupNodes(KisNodeOperationBatch *batch,
                                 const KisNodeList &nodes,
                                 KisNodeSP activeNode,
                                 KisNodeSP *incompatibleNode,
                                 KisNodeSP *destinationParent);
        static KisNodeSP parentNode(KisNodeSP node);
        static QString nodeName(KisNodeSP node);
        static void showFloatingMessage(KisNodeManager *manager, const QString &message);
    };

    struct KRITAUI_EXPORT ReferenceImageAccess {
        static KisPaintDevice *activeLayerProjection(KisNodeManager *manager);
        static KisPaintDevice *visibleProjection(KisNodeManager *manager);
        static QImage convertToImage(KisPaintDevice *device);
        static KisReferenceImage *createReferenceImage(KisNodeManager *manager, const QImage &image);
        static void deleteReferenceImage(KisReferenceImage *reference);
        static int referenceImageCount(KisNodeManager *manager);
        static void setZIndex(KisReferenceImage *reference, int index);
        static void addReferenceImage(KisNodeManager *manager, KisReferenceImage *reference);
        static void switchTool(const QString &toolId);
        static bool hasCanvasWidget(KisNodeManager *manager);
        static void showFloatingMessage(KisNodeManager *manager,
                                        const QString &message,
                                        int timeout,
                                        bool highPriority,
                                        bool singleLine);
    };

    struct KRITAUI_EXPORT LayerCreationAccess {
        static KisImage *image(KisNodeManager *manager);
        static KisNode *rootLastChild(KisImage *image);
        static void createFromVisible(KisImage *image, KisNode *putAfter);
        static KisLayerSP createPaintLayer(KisNodeManager *manager, const QString &nodeType);
    };

    enum class NodeCreationKind {
        PaintLayer,
        GroupLayer,
        AdjustmentLayer,
        GeneratorLayer,
        ShapeLayer,
        CloneLayer,
        TransparencyMask,
        FilterMask,
        FastColorOverlayMask,
        ColorizeMask,
        TransformMask,
        SelectionMask,
        FileLayer,
    };

    enum class NodeConversionKind {
        PaintLayer,
        SelectionMask,
        FilterMask,
        TransparencyMask,
        FileLayer,
    };

    struct KRITAUI_EXPORT NodeTypeAccess {
        static bool finishPendingOperations(KisNodeManager *manager);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static KisNodeSP rootNode(KisNodeManager *manager);
        static KisNodeList selectedNodes(KisNodeManager *manager);
        static KisNodeSP createNode(KisNodeManager *manager,
                                    NodeCreationKind kind,
                                    KisNodeSP activeNode,
                                    const KisNodeList &selectedNodes,
                                    KisPaintDevice *copyFrom,
                                    bool quiet);
        static bool canModifyLayer(KisNodeManager *manager, KisNodeSP node);
        static KisPaintDevice *paintDevice(KisNodeSP node);
        static KisPaintDevice *projection(KisNodeSP node);
        static void beginConversion(KisNodeManager *manager, const KUndo2MagicString &actionName);
        static bool
        convertToMask(KisNodeManager *manager, NodeConversionKind kind, KisNodeSP node, KisPaintDevice *copyFrom);
        static void endConversion(KisNodeManager *manager);
        static void convertNode(KisNodeManager *manager, NodeConversionKind kind, KisNodeSP node);
        static void finishPendingOperationsForced(KisNodeManager *manager);
        static void undoLastConversion(KisNodeManager *manager);
        static void reportUnsupportedNodeType(const QString &nodeType);
    };

    struct KRITAUI_EXPORT NodeExportAccess {
        static KisNodeSP activeNode(KisNodeManager *manager);
        static KisPaintDevice *projection(KisNodeSP node);
        static void reportNoActiveNode();
        static void showFloatingMessage(KisNodeManager *manager, const QString &message);
        static QRect imageBounds(KisNodeManager *manager);
        static QRect nodeBounds(KisNodeSP node);
        static QString nodeName(KisNodeSP node);
        static qreal imageXResolution(KisNodeManager *manager);
        static qreal imageYResolution(KisNodeManager *manager);
        static quint8 nodeOpacity(KisNodeSP node);
        static void saveDevice(KisNodeManager *manager,
                               KisPaintDevice *device,
                               const QString &defaultName,
                               const QRect &bounds,
                               qreal xResolution,
                               qreal yResolution,
                               quint8 opacity);
        static KisShapeLayer *shapeLayer(KisNodeSP node);
        static QString chooseSvgFilename(KisNodeManager *manager);
        static QSizeF imagePixelSize(KisNodeManager *manager);
        static QList<KoShape *> shapes(KisShapeLayer *layer);
        static void sortShapes(QList<KoShape *> *shapes);
        static bool saveSvg(const QString &filename, const QSizeF &sizeInPoints, const QList<KoShape *> &shapes);
        static void showSvgFailure(const QString &filename);
    };

    struct KRITAUI_EXPORT ActivationAccess {
        static bool hasGraphListener(KisNodeSP node);
        static KisNodeSP activeNode(KisNodeManager *manager);
        static bool nodeHasVectorAbilities(KisNodeSP node);
        static QString activeToolId();
        static void switchTool(const QString &toolId);
        static KisDummiesFacadeBase *dummiesFacade(KisNodeManager *manager);
        static bool isNodeVisible(KisNodeManager *manager, KisNodeSP node);
        static bool activateNode(KisNodeManager *manager, KisNodeSP node);
        static void setLastActivatedNode(KisDummiesFacadeBase *facade, KisNodeSP node);
        static void notifyUiNodeChange(KisNodeManager *manager, KisNodeSP node);
        static void notifyNodeActivated(KisNodeManager *manager, KisNodeSP node);
        static void nodesUpdated(KisNodeManager *manager);
        static bool canvasOnly(KisNodeManager *manager);
        static QString nodeName(KisNodeSP node);
        static void showNodeName(KisNodeManager *manager, const QString &name);
    };

    struct KRITAUI_EXPORT LifecycleAccess {
        static void *createPrivateState(KisNodeManager *manager, KisViewManager *view);
        static void connectReselectionOutput(KisNodeManager *manager);
        static void destroyPrivateState(void *state);
        static void setMaskView(KisNodeManager *manager, QPointer<KisView> imageView);
        static void setLayerView(KisNodeManager *manager, QPointer<KisView> imageView);
        static bool hasImageView(KisNodeManager *manager);
        static void disconnectNodeActivation(KisNodeManager *manager);
        static void disconnectImageSignals(KisNodeManager *manager);
        static void disconnectReselectionInput(KisNodeManager *manager);
        static void assignImageView(KisNodeManager *manager, QPointer<KisView> imageView);
        static void assignCommandImage(KisNodeManager *manager);
        static void connectNodeActivation(KisNodeManager *manager);
        static KisNodeSP currentNode(KisNodeManager *manager);
        static KisNodeSP lastActivatedNode(KisNodeManager *manager);
        static bool hasGraphListener(KisNodeSP node);
        static KisNodeSP lastRootChild(KisNodeManager *manager);
        static void activateNode(KisNodeManager *manager, KisNodeSP node);
        static void connectReselectionInput(KisNodeManager *manager);
        static void notifyResourceProvider(KisNodeManager *manager, KisNodeSP node);
        static void connectIsolation(KisNodeManager *manager);
        static void updateLayerGui(KisNodeManager *manager);
        static void updateMaskGui(KisNodeManager *manager);
    };

    struct KRITAUI_EXPORT SetupAccess {
        static void setupLayerManager(KisNodeManager *manager, KisActionManager *actionManager);
        static void setupMaskManager(KisNodeManager *manager,
                                     KisKActionCollection *actionCollection,
                                     KisActionManager *actionManager);
        static void registerAction(KisNodeManager *manager,
                                   KisActionManager *actionManager,
                                   const char *actionId,
                                   const char *signal,
                                   const char *slot,
                                   bool checkable,
                                   bool shapeLayerOnly,
                                   bool storePinAction);
        static void registerNodeCreation(KisNodeManager *manager,
                                         KisActionManager *actionManager,
                                         const char *actionId,
                                         const char *nodeType);
        static bool deferNodeCreation();
        static void connectNodeCreation(KisNodeManager *manager, bool deferred);
        static void registerNodeConversion(KisNodeManager *manager,
                                           KisActionManager *actionManager,
                                           const char *actionId,
                                           const char *nodeType,
                                           const QStringList &excludedNodeTypes);
        static void connectNodeConversion(KisNodeManager *manager);
        static void connectNodeActivationToIsolation(KisNodeManager *manager);
    };

private:
    /**
     * Scales opacity from the range 0...1
     * to the integer range 0...255
     */
    qint32 convertOpacityToInt(qreal opacity);
    KisNodeSP owningLayerNode(KisNodeSP node) const;
    void removeSelectedNodes(KisNodeList selectedNodes);
    void slotSomethingActivatedNodeImpl(KisNodeSP node);
    bool createQuickGroupImpl(KisNodeOperationBatch *batch,
                              const QString &overrideGroupName,
                              KisNodeSP *newGroup,
                              KisNodeSP *newLastChild);
    void selectLayersImpl(SelectionProperty property, bool value, bool invertedValue);
    void toggleNodeProperty(ToggleProperty property);

    struct Private;
    Private * const m_d;
};

#endif
