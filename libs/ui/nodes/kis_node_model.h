/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_NODE_MODEL
#define KIS_NODE_MODEL

#include "kritaui_export.h"
#include <kis_types.h>
#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QVariant>

#include <KisSelectionTags.h>

class KisDummiesFacadeBase;
class KisNodeDummy;
class KisShapeController;
class KisModelIndexConverterBase;
class KisNodeSelectionAdapter;
class KisNodeInsertionAdapter;
class KisSelectionActionsAdapter;
class KisNodeDisplayModeAdapter;
class KisNodeManager;
class KisIdleTasksManager;

/**
 * KisNodeModel offers a Qt model-view compatible view of the node
 * hierarchy. The KisNodeView displays a thumbnail and a row of
 * icon properties for every document section.
 *
 * Note that there's a discrepancy between the krita node tree model
 * and the model Qt wants to see: we hide the root node from Qt.
 *
 * The node model also shows an inverse view of the layer tree: we want
 * the first layer to show up at the bottom.
 * 
 * See also the Qt documentation for QAbstractItemModel. 
 * This class extends that interface to provide a name and set of toggle
 * properties (like visible, locked, selected.)
 * 
 */
class KRITAUI_EXPORT KisNodeModel : public QAbstractItemModel
{

    Q_OBJECT

public:
    /// Extensions to Qt::ItemDataRole.
    enum ItemDataRole
    {
        /// Whether the section is the active one
        ActiveRole = Qt::UserRole + 1,

        /// A list of properties the part has.
        PropertiesRole,

        /// The aspect ratio of the section as a floating point value: width divided by height.
        AspectRatioRole,

        /// Use to communicate a progress report to the section delegate on an action (a value of -1 or a QVariant() disable the progress bar
        ProgressRole,

        /// Special activation role which is emitted when the user Alt-clicks on a section
        /// The item is first activated with ActiveRole, then a separate AlternateActiveRole comes
        AlternateActiveRole,

        // When a layer is not (recursively) visible, then it should be grayed out
        ShouldGrayOutRole,

        // An index of a color label associated with the node
        ColorLabelIndexRole,

        // Instruct this model to update all its items' Qt::ItemIsDropEnabled flags in order to
        // reflect if the item allows an "onto" drop of the given QMimeData*.
        DropEnabled,

        // Instructs the model to activate "select opaque" action,
        // the selection action (of type SelectionAction) value
        // is passed via QVariant as integer
        SelectOpaqueRole,

        // Returns a text explaining why the node has been excluded from
        // projection rendering. If the node is not excluded, then empty
        // string is returned
        DropReasonRole,

        // Let's the model quickly determine whether a layer has any
        // animated content attached to it.
        IsAnimatedRole,

        // Returns a string with layer opacity and blending mode information;
        // content depends on style setting.
        InfoTextRole,

        // If the item is a filter mask that has a "color" property,
        // returns that mask's color. Otherwise, returns a null variant.
        FilterMaskColorRole,

        // If the item is a filter mask, shows that mask's properties dialog.
        FilterMaskPropertiesRole,

        // If the item is a layer that has a color overlay filter mask,
        // returns that mask's color. Otherwise, returns a null variant.
        LayerColorOverlayColorRole,

        // If the item is a layer that has a color overlay filter mask,
        // shows that mask's properties dialog.
        LayerColorOverlayPropertiesRole,

        /// This is to ensure that we can extend the data role in the future, since it's not possible to add a role after BeginThumbnailRole (due to "Hack")
        ReservedRole = Qt::UserRole + 99,

        /**
         * For values of BeginThumbnailRole or higher, a thumbnail of the layer of which neither dimension
         * is larger than (int) value - (int) BeginThumbnailRole.
         * This is a hack to work around the fact that Interview doesn't have a nice way to
         * request thumbnails of arbitrary size.
         */
        BeginThumbnailRole
    };

public: // from QAbstractItemModel

    explicit KisNodeModel(QObject * parent, int clonedColumns = 0);
    ~KisNodeModel() override;

    void setDummiesFacade(KisDummiesFacadeBase *dummiesFacade,
                          KisImageWSP image,
                          KisShapeController *shapeController,
                          KisSelectionActionsAdapter *selectionActionsAdapter,
                          KisNodeManager *nodeManager);
    void setIdleTaskManager(KisIdleTasksManager *idleTasksManager);
    KisNodeSP nodeFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromNode(KisNodeSP node) const;

    bool showGlobalSelection() const;
    void setPreferredThumnalSize(int preferredSize) const;

public Q_SLOTS:
    void setShowGlobalSelection(bool value);

public:

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList & indexes) const override;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;
    bool hasDummiesFacade();

    static bool belongsToIsolatedGroup(KisImageSP image, KisNodeSP node, KisDummiesFacadeBase *dummiesFacade);

Q_SIGNALS:

    void toggleIsolateActiveNode();
    void sigBeforeBeginRemoveRows(const QModelIndex &parent, int start, int end);

protected Q_SLOTS:
    void slotBeginInsertDummy(KisNodeDummy *parent, int index, const QString &metaObjectType);
    void slotEndInsertDummy(KisNodeDummy *dummy);
    void slotBeginRemoveDummy(KisNodeDummy *dummy);
    void slotEndRemoveDummy();
    void slotDummyChanged(KisNodeDummy *dummy);

    void slotIsolatedModeChanged();

    void slotNodeDisplayModeChanged(bool showRootNode, bool showGlobalSelectionMask);

    void processUpdateQueue();
    void progressPercentageChanged(int, const KisNodeSP);

    void slotLayerThumbnailUpdated(KisNodeSP node);

protected:
    struct KRITAUI_EXPORT DropMimeDataAccess {
        struct Context {
            KisImage *image;
            KisShapeController *shapeController;
            KisNodeInsertionAdapter *nodeInsertionAdapter;
        };

        static KisNodeDummy *parentDummy(const KisNodeModel *model, const QModelIndex &parent);
        static KisNodeDummy *lastChild(KisNodeDummy *dummy);
        static int rowCount(const KisNodeModel *model, const QModelIndex &parent);
        static KisNodeDummy *dummyFromRow(const KisNodeModel *model, int row, const QModelIndex &parent);
        static Context context(const KisNodeModel *model);
        static bool insertMimeLayers(const QMimeData *data,
                                     const Context &context,
                                     KisNodeDummy *parentDummy,
                                     KisNodeDummy *aboveThisDummy,
                                     bool copyNode);
    };

    struct KRITAUI_EXPORT MimeDataAccess {
        static KisNodeSP nodeFromIndex(const KisNodeModel *model, const QModelIndex &index);
        static bool isEditable(const KisNodeSP &node, bool checkVisibility);
        static KisImage *image(const KisNodeModel *model);
        static QMimeData *createMimeData(const KisNodeList &nodes, KisImage *image, bool forceCopy);
    };

    struct KRITAUI_EXPORT ItemFlagsAccess {
        static bool hasDummiesFacade(const KisNodeModel *model);
        static bool isDropEnabled(const KisNodeModel *model, quintptr itemId);
    };

    struct KRITAUI_EXPORT FacadeSetupAccess {
        static KisDummiesFacadeBase *currentFacade(const KisNodeModel *model);
        static KisShapeController *currentShapeController(const KisNodeModel *model);
        static bool hasImage(const KisNodeModel *model);
        static void configureCollaborators(KisNodeModel *model,
                                           KisShapeController *shapeController,
                                           KisSelectionActionsAdapter *selectionActionsAdapter,
                                           KisNodeManager *nodeManager);
        static void configureDisplayMode(KisNodeModel *model, KisNodeManager *nodeManager);
        static void disconnectCurrentTree(KisNodeModel *model, KisDummiesFacadeBase *oldFacade);
        static void replaceTree(KisNodeModel *model,
                                KisDummiesFacadeBase *dummiesFacade,
                                const KisImageWSP &image);
        static void connectCurrentTree(KisNodeModel *model);
    };

    struct KRITAUI_EXPORT LifecycleAccess {
        static void *createPrivateState(int clonedColumns);
        static void connectUpdateCompressor(KisNodeModel *model, void *privateState);
        static void connectThumbnailCache(KisNodeModel *model, void *privateState);
        static void destroyPrivateState(void *privateState);
    };

    struct KRITAUI_EXPORT RemovalAccess {
        struct Plan {
            QModelIndex parentIndex;
            QModelIndex itemIndex;
        };

        static Plan prepare(KisNodeModel *model, KisNodeDummy *dummy);
        static void disconnectDummy(KisNodeModel *model, KisNodeDummy *dummy);
        static void beginRemoval(KisNodeModel *model, const Plan &plan);
        static void notifyNodeRemoved(KisNodeModel *model, KisNodeDummy *dummy);
    };

    struct KRITAUI_EXPORT DataAccess {
        static bool hasDummiesFacade(const KisNodeModel *model);
        static bool hasImage(const KisNodeModel *model);
        static QVariant nodeName(const KisNodeModel *model, const QModelIndex &index);
        static QVariant nodeIcon(const KisNodeModel *model, const QModelIndex &index);
        static QVariant imageSize(const KisNodeModel *model, const QModelIndex &index);
        static QVariant foreground(const KisNodeModel *model, const QModelIndex &index);
        static QVariant font(const KisNodeModel *model, const QModelIndex &index);
        static QVariant properties(const KisNodeModel *model, const QModelIndex &index);
        static QVariant aspectRatio(const KisNodeModel *model, const QModelIndex &index);
        static QVariant progress(const KisNodeModel *model, const QModelIndex &index);
        static QVariant active(const KisNodeModel *model, const QModelIndex &index);
        static QVariant shouldGrayOut(const KisNodeModel *model, const QModelIndex &index);
        static QVariant colorLabel(const KisNodeModel *model, const QModelIndex &index);
        static QVariant dropReason(const KisNodeModel *model, const QModelIndex &index);
        static QVariant isAnimated(const KisNodeModel *model, const QModelIndex &index);
        static QVariant remainingData(const KisNodeModel *model, const QModelIndex &index, int role);
    };

    struct KRITAUI_EXPORT SetDataAccess {
        static void setDropEnabled(KisNodeModel *model, const QMimeData *data);
        static QModelIndex takeParentOfRemovedNode(KisNodeModel *model);
        static KisNodeSP nodeFromIndex(const KisNodeModel *model, const QModelIndex &index);
        static QModelIndex indexFromNode(const KisNodeModel *model, const KisNodeSP &node);
        static QModelIndex activeNodeIndex(const KisNodeModel *model);
        static void setActiveNodeIndex(KisNodeModel *model, const QModelIndex &index);
        static void setSelectionAdapterActiveNode(KisNodeModel *model, const KisNodeSP &node);
        static int dummyColumns(const KisNodeModel *model);
        static bool setRemainingData(KisNodeModel *model,
                                     const QModelIndex &index,
                                     const QVariant &value,
                                     int role);
    };

    struct KRITAUI_EXPORT StructureAccess {
        static bool hasDummiesFacade(const KisNodeModel *model);
        static KisNodeDummy *dummyFromRow(const KisNodeModel *model, int row, const QModelIndex &parent);
        static KisNodeDummy *dummyFromIndex(const KisNodeModel *model, const QModelIndex &index);
        static KisNodeDummy *parentDummy(KisNodeDummy *dummy);
        static QModelIndex indexFromDummy(const KisNodeModel *model, KisNodeDummy *dummy);
        static int rowCount(const KisNodeModel *model, const QModelIndex &parent);
        static int dummyColumns(const KisNodeModel *model);
        static bool hasMatchingModel(const KisNodeModel *model, const QModelIndex &index);
    };

    struct KRITAUI_EXPORT IsolationMembershipAccess {
        static KisNodeSP isolationRoot(const KisImageSP &image);
        static KisNodeDummy *dummyForNode(KisDummiesFacadeBase *dummiesFacade, const KisNodeSP &node);
        static KisNodeDummy *parentDummy(KisNodeDummy *dummy);
    };

    struct KRITAUI_EXPORT IndexMappingAccess {
        static KisNodeDummy *dummyFromIndex(const KisNodeModel *model, const QModelIndex &index);
        static KisNodeSP nodeForDummy(KisNodeDummy *dummy);
        static KisNodeDummy *dummyForNode(const KisNodeModel *model, KisNodeSP node);
        static QModelIndex indexFromDummy(const KisNodeModel *model, KisNodeDummy *dummy);
    };

    struct KRITAUI_EXPORT DisplayStateAccess {
        static bool hasDisplayModeAdapter(const KisNodeModel *model);
        static bool showGlobalSelectionMask(const KisNodeModel *model);
        static void setShowGlobalSelectionMask(KisNodeModel *model, bool value);
        static void setPreferredThumbnailSize(const KisNodeModel *model, int preferredSize);
        static void setIdleTaskManager(KisNodeModel *model, KisIdleTasksManager *idleTasksManager);
        static bool hasDummiesFacade(const KisNodeModel *model);
    };

    virtual KisModelIndexConverterBase *createIndexConverter();
    KisModelIndexConverterBase *indexConverter() const;
    KisDummiesFacadeBase *dummiesFacade() const;

private:
    friend class KisModelIndexConverter;
    friend class KisModelIndexConverterShowAll;

    void connectDummy(KisNodeDummy *dummy, bool needConnect);
    void connectDummies(KisNodeDummy *dummy, bool needConnect);

    void resetIndexConverter();

    void regenerateItems(KisNodeDummy *dummy);
    bool belongsToIsolatedGroup(KisNodeSP node) const;

	void setDropEnabled(const QMimeData *data);
	void updateDropEnabled(const QList<KisNodeSP> &nodes, QModelIndex parent = QModelIndex());
    
private:

    struct Private;
    Private * const m_d;
};

#endif
