/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "workspace/KisView.h"

#include "workspace/KisView_p.h"

#include <metadata/KoDocumentInfo.h>
#include <KoToolManager.h>

#include <files/kis_document_autosave_files.h>

#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kis_debug.h>
#include <kconfiggroup.h>

#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QMimeData>
#include <QToolBar>
#include <QStatusBar>
#include <QMoveEvent>
#include <QMdiSubWindow>
#include <QFileInfo>

#include "document/KisDocument.h"
#include "workspace/KisMainWindow.h"
#include "application/KisPart.h"
#include "canvas/KisReferenceImagesDecoration.h"
#include "KisSynchronizedConnection.h"
#include "workspace/KisViewManager.h"
#include "input/ui/kis_input_manager.h"
#include "kis_canvas2.h"
#include "kis_canvas_controller.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "application/kis_config.h"
#include "document/kis_filter_manager.h"
#include "document/kis_image_manager.h"
#include "kis_mimedata.h"
#include "nodes/kis_node_manager.h"
#include "canvas/kis_painting_assistants_decoration.h"
#include "selection/kis_selection_manager.h"
#include "kis_shape_controller.h"
#include "kis_signal_compressor.h"
#include "canvas/kis_zoom_manager.h"
#include <KisScreenMigrationTracker.h>
#include "kformat.h"


//static
QString KisView::newObjectName()
{
    static int s_viewIFNumber = 0;
    QString name; name.setNum(s_viewIFNumber++); name.prepend("view_");
    return name;
}

bool KisView::s_firstView = true;

class Q_DECL_HIDDEN KisView::Private
{
public:
    Private(KisView *_q,
            KisDocument *document,
            KisViewManager *viewManager)
        : actionCollection(viewManager->actionCollection())
        , viewConverter()
        , canvasController(_q, viewManager->mainWindow(), viewManager->actionCollection())
        , canvas(&viewConverter, viewManager->canvasResourceProvider()->resourceManager(), viewManager->mainWindow(), _q, document->shapeController())
        , zoomManager(_q, &this->viewConverter, &this->canvasController)
        , viewManager(viewManager)
        , floatingMessageCompressor(100, KisSignalCompressor::POSTPONE)
        , screenMigrationTracker(_q)
    {
    }

    bool inOperation {false}; //in the middle of an operation (no screen refreshing)?

    QPointer<KisDocument> document; // our KisDocument
    QWidget *tempActiveWidget {nullptr};

    KisKActionCollection* actionCollection {nullptr};
    KisCoordinatesConverter viewConverter;
    KisCanvasController canvasController;
    KisCanvas2 canvas;
    KisZoomManager zoomManager;
    QPointer<KisViewManager> viewManager;
    KisNodeSP currentNode;
    KisPaintingAssistantsDecorationSP paintingAssistantsDecoration;
    KisReferenceImagesDecorationSP referenceImagesDecoration;
    bool isCurrent {false};
    bool showFloatingMessage {true};
    QPointer<KisFloatingMessage> savedFloatingMessage;
    KisSignalCompressor floatingMessageCompressor;
    QMdiSubWindow *subWindow {nullptr};

    bool softProofing {false};
    bool gamutCheck {false};

    KisSynchronizedConnection<KisNodeSP, KisNodeAdditionFlags> addNodeConnection;
    KisSynchronizedConnection<KisNodeSP> removeNodeConnection;

    KisScreenMigrationTracker screenMigrationTracker;

    // Hmm sorry for polluting the private class with such a big inner class.
    // At the beginning it was a little struct :)
    class StatusBarItem : public boost::equality_comparable<StatusBarItem>
    {
    public:

        StatusBarItem(QWidget * widget, int stretch, bool permanent)
            : m_widget(widget),
              m_stretch(stretch),
              m_permanent(permanent),
              m_connected(false),
              m_hidden(false) {}

        bool operator==(const StatusBarItem& rhs) {
            return m_widget == rhs.m_widget;
        }

        QWidget * widget() const {
            return m_widget;
        }

        void ensureItemShown(QStatusBar * sb) {
            Q_ASSERT(m_widget);
            if (!m_connected) {
                if (m_permanent)
                    sb->addPermanentWidget(m_widget, m_stretch);
                else
                    sb->addWidget(m_widget, m_stretch);

                if(!m_hidden)
                    m_widget->show();

                m_connected = true;
            }
        }
        void ensureItemHidden(QStatusBar * sb) {
            if (m_connected) {
                m_hidden = m_widget->isHidden();
                sb->removeWidget(m_widget);
                m_widget->hide();
                m_connected = false;
            }
        }

    private:
        QWidget * m_widget = 0;
        int m_stretch;
        bool m_permanent;
        bool m_connected = false;
        bool m_hidden = false;

    };

};

KisView::KisView(KisDocument *document, KisViewManager *viewManager, QWidget *parent)
    : QWidget(parent)
    , d(new Private(this, document, viewManager))
{
    Q_ASSERT(document);
    setObjectName(newObjectName());

    d->document = document;

    setFocusPolicy(Qt::StrongFocus);

    QStatusBar * sb = statusBar();
    if (sb) { // No statusbar in e.g. konqueror
        connect(d->document, SIGNAL(statusBarMessage(QString,int)),
                this, SLOT(slotSavingStatusMessage(QString,int)));
        connect(d->document, SIGNAL(clearStatusBarMessage()),
                this, SLOT(slotClearStatusText()));
    }

    d->canvas.setup();

    KisConfig cfg(false);

    d->canvasController.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    d->canvasController.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    d->canvasController.setCanvas(&d->canvas);

    d->zoomManager.setup(d->actionCollection);

    setAcceptDrops(true);

    connect(d->document, SIGNAL(sigLoadingFinished()), this, SLOT(slotLoadingFinished()));

    d->referenceImagesDecoration = new KisReferenceImagesDecoration(this, document, /* viewReady = */ false);
    d->canvas.addDecoration(d->referenceImagesDecoration);
    d->referenceImagesDecoration->setVisible(true);

    d->paintingAssistantsDecoration = new KisPaintingAssistantsDecoration(this);
    d->canvas.addDecoration(d->paintingAssistantsDecoration);
    d->paintingAssistantsDecoration->setVisible(true);

    d->showFloatingMessage = cfg.showCanvasMessages();
    slotScreenOrResolutionChanged();

    connect(document, SIGNAL(sigReadWriteChanged(bool)), this, SLOT(slotUpdateDocumentTitle()));
    connect(document, SIGNAL(sigRecoveredChanged(bool)), this, SLOT(slotUpdateDocumentTitle()));
    connect(document, SIGNAL(sigPathChanged(QString)), this, SLOT(slotUpdateDocumentTitle()));
    document->connectImageMemoryStatisticsUpdates(this, SLOT(slotUpdateDocumentTitle()));
    connect(document, SIGNAL(modified(bool)), this, SLOT(setWindowModified(bool)));
    slotUpdateDocumentTitle();
    setWindowModified(document->isModified());
}

KisView::~KisView()
{
    if (d->viewManager) {
        if (d->viewManager->filterManager()->isStrokeRunning()) {
            d->viewManager->filterManager()->cancelDialog();
        }

        d->viewManager->mainWindow()->notifyChildViewDestroyed(this);
    }

    d->canvas.requestStrokeCancellationForTool();

    /**
     * KisCanvas2 maintains direct connections to the image, so we should
     * disconnect it from the image before the destruction process starts
     */
    d->canvas.disconnectImage();

    KoToolManager::instance()->removeCanvasController(&d->canvasController);
    d->canvasController.setCanvas(0);

    KisPart::instance()->removeView(this);
    delete d;
}

void KisView::notifyCurrentStateChanged(bool isCurrent)
{
    d->isCurrent = isCurrent;

    if (!d->isCurrent && d->savedFloatingMessage) {
        d->savedFloatingMessage->removeMessage();
    }

    KisInputManager *inputManager = globalInputManager();
    if (d->isCurrent) {
        inputManager->attachPriorityEventFilter(&d->canvasController);
    } else {
        inputManager->detachPriorityEventFilter(&d->canvasController);
    }

    /**
     * When current view is changed, currently selected node is also changed,
     * therefore we should update selection overlay mask
     */
    viewManager()->selectionManager()->selectionChanged();
}

bool KisView::isCurrent() const
{
    return d->isCurrent;
}

void KisView::setShowFloatingMessage(bool show)
{
    d->showFloatingMessage = show;
}

void KisView::showFloatingMessage(const QString &message, const QIcon& icon, int timeout, KisFloatingMessage::Priority priority, int alignment)
{
    if (!d->viewManager) return;

    if(d->isCurrent && d->showFloatingMessage && d->viewManager->qtMainWindow()) {
        if (d->savedFloatingMessage) {
            d->savedFloatingMessage->tryOverrideMessage(message, icon, timeout, priority, alignment);
        } else {
            d->savedFloatingMessage = new KisFloatingMessage(message, this->canvasBase()->canvasWidget(), false, timeout, priority, alignment);
            d->savedFloatingMessage->setShowOverParent(true);
            d->savedFloatingMessage->setIcon(icon);

            connect(&d->floatingMessageCompressor, SIGNAL(timeout()), d->savedFloatingMessage, SLOT(showMessage()));
            d->floatingMessageCompressor.start();
        }
    }
}

bool KisView::canvasIsMirrored() const
{
    return d->canvas.xAxisMirrored() || d->canvas.yAxisMirrored();
}

void KisView::setViewManager(KisViewManager *view)
{
    d->viewManager = view;

    if (!d->viewManager) {
        return;
    }

    KoToolManager::instance()->addController(&d->canvasController);
    KisShapeController* shapeController = dynamic_cast<KisShapeController*>(d->document->shapeController());
    shapeController->setInitialShapeForCanvas(&d->canvas);

    if (d->viewManager && d->viewManager->nodeManager()) {
        d->viewManager->nodeManager()->nodesUpdated();
    }

    connect(&d->canvas,
            &KisCanvas2::sigViewImageSizeChanged,
            this,
            &KisView::slotImageSizeChanged);
    connect(&d->canvas,
            &KisCanvas2::sigViewImageResolutionChanged,
            this,
            &KisView::slotImageResolutionChanged);

    d->addNodeConnection.connectSync(&d->canvas, &KisCanvas2::sigViewNodeAddedAsync,
                                     this, &KisView::slotContinueAddNode);

    // executed in a context of an image thread
    connect(&d->canvas,
            &KisCanvas2::sigViewNodeRemovedAsync,
            this,
            &KisView::slotImageNodeRemoved,
            Qt::DirectConnection);

    d->removeNodeConnection.connectOutputSlot(this, &KisView::slotContinueRemoveNode);

    d->canvas.connectViewImageSignals();

    d->viewManager->updateGUI();

    KoToolManager::instance()->switchToolRequested("KritaShape/KisToolBrush");
}

KisViewManager* KisView::viewManager() const
{
    return d->viewManager;
}

void KisView::slotContinueAddNode(KisNodeSP newActiveNode, KisNodeAdditionFlags flags)
{
    Q_UNUSED(flags)

    /**
     * When deleting the last layer, root node got selected. We should
     * fix it when the first layer is added back.
     *
     * Here we basically reimplement what Qt's view/model do. But
     * since they are not connected, we should do it manually.
     */

    if (!d->isCurrent &&
            (!d->currentNode || !d->currentNode->parent())) {

        d->currentNode = newActiveNode;
    }
}


void KisView::slotImageNodeRemoved(KisNodeSP node)
{
    d->removeNodeConnection.start(viewManager()->nodeManager()->nearestNodeAfterRemoval(node));
}

void KisView::slotContinueRemoveNode(KisNodeSP newActiveNode)
{
    if (!d->isCurrent) {
        d->currentNode = newActiveNode;
    }
}

KisZoomManager *KisView::zoomManager() const
{
    return &d->zoomManager;
}

KisCanvasController *KisView::canvasController() const
{
    return &d->canvasController;
}

KisCanvasResourceProvider *KisView::resourceProvider() const
{
    if (d->viewManager) {
        return d->viewManager->canvasResourceProvider();
    }
    return 0;
}

KisInputManager* KisView::globalInputManager() const
{
    return d->viewManager ? d->viewManager->inputManager() : 0;
}

KisCanvas2 *KisView::canvasBase() const
{
    return &d->canvas;
}

KisImageWSP KisView::image() const
{
    if (d->document) {
        return d->document->image();
    }
    return 0;
}


KisCoordinatesConverter *KisView::viewConverter() const
{
    return &d->viewConverter;
}

void KisView::dragEnterEvent(QDragEnterEvent *event)
{
    dbgUI << Q_FUNC_INFO
          << "Formats: " << event->mimeData()->formats()
          << "Urls: " << event->mimeData()->urls()
          << "Has images: " << event->mimeData()->hasImage();

    if (shouldAcceptDrag(event)) {
        event->accept();
        setFocus(); // activate view if it should accept the drop
    } else {
        event->ignore();
    }
}

void KisView::dropEvent(QDropEvent *event)
{
    dbgUI << Q_FUNC_INFO;
    dbgUI << "\t Formats: " << event->mimeData()->formats();
    dbgUI << "\t Urls: " << event->mimeData()->urls();
    dbgUI << "\t Has images: " << event->mimeData()->hasImage();

    if (!shouldAcceptDrag(event)) {
        return;
    }

    Q_ASSERT(image());

    const QPoint imagePosition =
        canvasBase()->coordinatesConverter()->widgetToImage(event->pos()).toPoint();

    if (event->mimeData()->hasFormat("application/x-krita-node-internal-pointer")
        || event->mimeData()->hasImage()
        || event->mimeData()->hasUrls()) {
        viewManager()->imageManager()->handleImageDrop(this, event, imagePosition);
    } else if (event->mimeData()->hasColor()
               || event->mimeData()->hasFormat("krita/x-colorsetentry")) {
        QIcon messageIcon;
        const QString message =
            canvasBase()->handleColorDrop(event, viewManager(), d->currentNode, imagePosition, &messageIcon);
        if (!message.isNull() || !messageIcon.isNull()) {
            showFloatingMessage(message, messageIcon);
        }
    }
}


void KisView::dragMoveEvent(QDragMoveEvent *event)
{
    dbgUI << Q_FUNC_INFO
          << "Formats: " << event->mimeData()->formats()
          << "Urls: " << event->mimeData()->urls()
          << "Has images: " << event->mimeData()->hasImage();

    if (shouldAcceptDrag(event)) {
        event->accept();
    } else {
        event->ignore();
    }
}

KisDocument *KisView::document() const
{
    return d->document;
}

KisView *KisView::replaceBy(KisDocument *document)
{
    KisMainWindow *window = mainWindow();
    QMdiSubWindow *subWindow = d->subWindow;
    delete this;
    return window->newView(document, subWindow);
}

KisMainWindow * KisView::mainWindow() const
{
    return d->viewManager->mainWindow();
}

void KisView::setSubWindow(QMdiSubWindow *subWindow)
{
    d->subWindow = subWindow;
}

QStatusBar * KisView::statusBar() const
{
    KisMainWindow *mw = mainWindow();
    return mw ? mw->statusBar() : 0;
}

void KisView::slotSavingStatusMessage(const QString &text, int timeout, bool isAutoSaving)
{
    QStatusBar *sb = statusBar();
    if (sb) {
        sb->showMessage(text, timeout);
    }

    KisConfig cfg(true);

    if (!sb || sb->isHidden() ||
        (!isAutoSaving && cfg.forceShowSaveMessages()) ||
        (cfg.forceShowAutosaveMessages() && isAutoSaving)) {

        viewManager()->showFloatingMessage(text, QIcon());
    }
}

void KisView::slotClearStatusText()
{
    QStatusBar *sb = statusBar();
    if (sb) {
        sb->clearMessage();
    }
}

QList<QAction*> KisView::createChangeUnitActions(bool addPixelUnit)
{
    UnitActionGroup* unitActions = new UnitActionGroup(d->document, addPixelUnit, this);
    return unitActions->actions();
}

void KisView::closeEvent(QCloseEvent *event)
{
    // Check whether we're the last view
    int viewCount = KisPart::instance()->viewCount(document());
    if (viewCount > 1) {
        // there are others still, so don't bother the user
        event->accept();
        return;
    }

    if (queryClose()) {
        event->accept();
        return;
    }

    event->ignore();

}

bool KisView::queryClose()
{
    if (!document())
        return true;

    document()->waitForSavingToComplete();

    if (document()->isModified()) {
        QString name;
        name = QFileInfo(document()->path()).fileName();

        if (name.isEmpty())
            name = i18n("Untitled");

        int res = QMessageBox::warning(this,
                                       i18nc("@title:window", "LibrePaint"),
                                       i18n("<p>The document <b>'%1'</b> has been modified.</p><p>Do you want to save it?</p>", name),
                                       QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

        switch (res) {
        case QMessageBox::Yes : {
            bool isNative = (document()->mimeType() == document()->nativeFormatMimeType());
            if (!viewManager()->mainWindow()->saveDocument(document(), !isNative, false))
                return false;
            break;
        }
        case QMessageBox::No : {
            canvasBase()->requestStrokeCancellationForTool();
            viewManager()->blockUntilOperationsFinishedForced(image());
            Krita::Document::KisDocumentAutoSaveFiles::removeForDocument(
                document()->localFilePath(), document()->isRecovered(), document()->objectName(),
                KisConfig(true).readEntry<bool>("autosavefileshidden"));
            document()->setModified(false);   // Now when queryClose() is called by closeEvent it won't do anything.
            break;
        }
        default : // case QMessageBox::Cancel :
            return false;
        }
    }

    return true;
}

void KisView::slotMigratedToScreen(QScreen *screen)
{
    d->canvas.slotScreenChanged(screen);
}

void KisView::slotScreenOrResolutionChanged()
{
    /**
     * slotScreenOrResolutionChanged() is guaranteed to come after
     * slotMigratedToScreen() when a migration happens
     */
    d->canvasController.updateScreenResolution(this);

    if (d->canvas.resourceManager() && d->screenMigrationTracker.currentScreen()) {
        int penWidth = qRound(d->screenMigrationTracker.currentScreen()->devicePixelRatio());
        d->canvas.resourceManager()->setDecorationThickness(qMax(penWidth, 1));
    }
}

QScreen* KisView::currentScreen() const
{
    return d->screenMigrationTracker.currentScreen();
}

void KisView::slotThemeChanged(QPalette pal)
{
    this->setPalette(pal);
    for (int i=0; i<this->children().size();i++) {
        QWidget *w = qobject_cast<QWidget*> ( this->children().at(i));
        if (w) {
            w->setPalette(pal);
        }
    }
    if (canvasBase()) {
        canvasBase()->canvasWidget()->setPalette(pal);
    }
    if (canvasController()) {
        canvasController()->setPalette(pal);
    }
}

void KisView::slotUpdateDocumentTitle()
{
    QString title = d->document->caption();

    if (!d->document->isReadWrite()) {
        title += " " + i18n("Write Protected");
    }

    if (d->document->isRecovered()) {
        title += " " + i18n("Recovered");
    }

    // show the file size for the document
    const qint64 imageMemorySize = d->document->imageMemorySize();
    if (imageMemorySize) {
        title += QString(" (").append(KFormat().formatByteSize(qreal(imageMemorySize))).append(") ");
    }

    title += "[*]";

    this->setWindowTitle(title);
}

void KisView::syncLastActiveNodeToDocument()
{
    KisDocument *doc = document();
    if (doc) {
        doc->setPreActivatedNode(d->currentNode);
    }
}

void KisView::saveViewState(KisPropertiesConfiguration &config) const
{
    config.setProperty("file", d->document->path());
    config.setProperty("window", mainWindow()->windowStateConfig().name());

    if (d->subWindow) {
        config.setProperty("geometry", d->subWindow->saveGeometry().toBase64());
    }

    const KoZoomState zoomState = d->canvasController.zoomState();

    config.setProperty("zoomMode", zoomState.mode);
    config.setProperty("zoom", zoomState.zoom);

    d->canvasController.saveCanvasState(config);
}

void KisView::restoreViewState(const KisPropertiesConfiguration &config)
{
    if (d->subWindow) {
        QByteArray geometry = QByteArray::fromBase64(config.getString("geometry", "").toLatin1());
        d->subWindow->restoreGeometry(QByteArray::fromBase64(geometry));
    }

    qreal zoom = config.getFloat("zoom", 1.0f);
    int zoomMode = config.getInt("zoomMode", (int)KoZoomMode::ZOOM_PAGE);
    d->canvasController.setZoom((KoZoomMode::Mode)zoomMode, zoom);
    d->canvasController.restoreCanvasState(config);
}

void KisView::setCurrentNode(KisNodeSP node)
{
    d->currentNode = node;
    d->canvas.slotTrySwitchShapeManager();

    syncLastActiveNodeToDocument();
}

KisNodeSP KisView::currentNode() const
{
    return d->currentNode;
}

KisLayerSP KisView::currentLayer() const
{
    return viewManager()->nodeManager()->layerForNode(currentNode());
}

KisMaskSP KisView::currentMask() const
{
    return viewManager()->nodeManager()->maskForNode(currentNode());
}

KisSelectionSP KisView::selection()
{
    return viewManager()->nodeManager()->selectionForNode(currentNode(), image());
}

void KisView::slotSoftProofing(bool softProofing)
{
    d->softProofing = softProofing;
    QString message;
    if (canvasBase()->imageUsesFloatingPointColorDepth())
    {
        message = i18n("Soft Proofing doesn't work in floating point.");
        viewManager()->showFloatingMessage(message,QIcon());
        return;
    }
    if (softProofing){
        message = i18n("Soft Proofing turned on.");
    } else {
        message = i18n("Soft Proofing turned off.");
    }
    viewManager()->showFloatingMessage(message,QIcon());
    canvasBase()->slotSoftProofing();
}

void KisView::slotGamutCheck(bool gamutCheck)
{
    d->gamutCheck = gamutCheck;
    QString message;
    if (canvasBase()->imageUsesFloatingPointColorDepth())
    {
        message = i18n("Gamut Warnings don't work in floating point.");
        viewManager()->showFloatingMessage(message,QIcon());
        return;
    }

    if (gamutCheck){
        message = i18n("Gamut Warnings turned on.");
        if (!d->softProofing){
            message += "\n "+i18n("But Soft Proofing is still off.");
        }
    } else {
        message = i18n("Gamut Warnings turned off.");
    }
    viewManager()->showFloatingMessage(message,QIcon());
    canvasBase()->slotGamutCheck();
}

bool KisView::softProofing()
{
    return d->softProofing;
}

bool KisView::gamutCheck()
{
    return d->gamutCheck;
}

void KisView::slotLoadingFinished()
{
    if (!document()) return;

    /**
     * Cold-start of image size/resolution signals
     */
    slotImageResolutionChanged();

    canvasBase()->prepareImageForDisplay();

    connect(canvasBase(),
            &KisCanvas2::sigViewImageColorSpaceChanged,
            this,
            &KisView::sigColorSpaceChanged);
    connect(canvasBase(),
            &KisCanvas2::sigViewImageProfileChanged,
            this,
            &KisView::sigProfileChanged);
    connect(canvasBase(),
            &KisCanvas2::sigViewImageSizeChanged,
            this,
            &KisView::sigSizeChanged);

    connect(&d->screenMigrationTracker, SIGNAL(sigScreenChanged(QScreen*)), this, SLOT(slotMigratedToScreen(QScreen*)));
    connect(&d->screenMigrationTracker, SIGNAL(sigScreenOrResolutionChanged(QScreen*)), this, SLOT(slotScreenOrResolutionChanged()));
    zoomManager()->updateImageBoundsSnapping();
}

void KisView::slotImageResolutionChanged()
{
    d->canvasController.syncOnImageResolutionChange();
    d->zoomManager.syncOnImageResolutionChange();
    zoomManager()->updateImageBoundsSnapping();
}

void KisView::slotImageSizeChanged(const QPointF &oldStillPoint, const QPointF &newStillPoint)
{
    d->canvasController.syncOnImageSizeChange(oldStillPoint, newStillPoint);
    zoomManager()->updateImageBoundsSnapping();
}

void KisView::closeView()
{
    d->subWindow->close();
}

bool KisView::shouldAcceptDrag(const QDropEvent *event) const
{
    const QMimeData *data = event->mimeData();
    if (data->hasFormat(QStringLiteral("application/x-krita-node-internal-pointer"))) {
        // Don't allow dragging layers onto their own canvas, that really only
        // gets triggered accidentally if you're a bit sloppy about selecting
        // or reordering layers and then you're left confused about the layer
        // duplicates that seem to show up at random. The user can override
        // this by explicitly holding down Ctrl if necessary. We always accept
        // the enter event so that this works properly, otherwise we don't get
        // any further drag events unless Ctrl was held to begin with.
        return event->type() == QEvent::DragEnter
            || event->keyboardModifiers().testFlag(Qt::ControlModifier)
            || !KisMimeData::isNodeMimeDataFromSameImage(data, image());
    } else {
        return data->hasImage()
            || data->hasUrls()
            || data->hasFormat("krita/x-colorsetentry")
            || data->hasColor();
    }
}
