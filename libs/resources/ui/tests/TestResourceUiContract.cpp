/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <QPointer>
#include <QStandardItemModel>
#include <QWheelEvent>

#include <KisResourceItemChooser.h>
#include <KisResourceItemChooserInputSource_p.h>
#include <KisResourceItemChooserPresentationSource_p.h>
#include <KisResourceItemChooserSelectionSource_p.h>
#include <KisResourceItemChooser_p.h>
#include <KisResourceTypes.h>
#include <KisResourceUiDescriptor.h>

namespace
{
int constructionCount = 0;
QString constructedResourceType;
bool constructedPreview = false;
KisResourceItemChooser::Layout initialLayout =
    KisResourceItemChooser::Layout::NotSet;
int layoutChangeCount = 0;
int listViewModeCount = 0;
ListViewMode capturedListViewMode = ListViewMode::IconGrid;
int taggingBarCount = 0;
bool capturedTaggingBarVisible = false;
int viewModeButtonCount = 0;
bool capturedViewModeButtonVisible = false;
int storageButtonCount = 0;
bool capturedStorageButtonVisible = false;
int rowHeightCount = 0;
int capturedRowHeight = 0;
int columnWidthCount = 0;
int capturedColumnWidth = 0;
int delegateCount = 0;
QAbstractItemDelegate *capturedDelegate = nullptr;
int importExportCount = 0;
bool capturedImportExportVisible = false;
int previewOrientationCount = 0;
Qt::Orientation capturedPreviewOrientation = Qt::Horizontal;
QSize configuredViewSize;
bool viewSizeReceivedMarker = false;
int iconUpdateCount = 0;
bool configuredCurrentIndexSelected = false;
QStandardItemModel *configuredSelectionModel = nullptr;
QModelIndex configuredResourceIndex;
int currentIndexSelectionCount = 0;
int resourceIndexCount = 0;
int setCurrentIndexCount = 0;
QModelIndex capturedCurrentIndex;
int rowCountCount = 0;
int columnCountCount = 0;
int indexCount = 0;
int resourceForIndexCount = 0;
int resourceNameCount = 0;
int indexForRowCount = 0;
int capturedRow = -1;
int resourceValidityCount = 0;
bool configuredResourceValid = false;
bool selectionSourcePointersCorrect = true;
KoResourceSP capturedIndexedResource;
KoResourceSP capturedValidityResource;
QList<QModelIndex> previewIndexes;
bool previewTiled = false;
bool previewGrayscale = false;
int buttonStateUpdateCount = 0;
bool emitListViewModeOnRelayout = false;
ListViewMode relayoutViewMode = ListViewMode::IconGrid;
KisResourceItemChooser *expectedInputChooser = nullptr;
int connectBaseLengthCount = 0;
int disconnectBaseLengthCount = 0;
int configuredBaseLength = 50;
QList<int> itemSizeLengths;
int setBaseLengthCount = 0;
int capturedBaseLength = 0;
int cursorUpdateCount = 0;
QScroller::State capturedScrollerState = QScroller::Inactive;
bool inputSourcePointersCorrect = true;

KoResourceSP markerResource(quintptr value)
{
    return KoResourceSP(reinterpret_cast<KoResource *>(value),
                        [](KoResource *) {});
}

const KoResourceSP firstResource = markerResource(0xb0);
const KoResourceSP secondResource = markerResource(0xc0);
const KoResourceSP thirdResource = markerResource(0xd0);
const KoResourceSP reentrantResource = markerResource(0xe0);

auto *const markerFilter =
    reinterpret_cast<KisTagFilterResourceProxyModel *>(quintptr(0x10));
auto *const markerManager =
    reinterpret_cast<KisResourceTaggingManager *>(quintptr(0x20));
auto *const markerViewButton =
    reinterpret_cast<KisPopupButton *>(quintptr(0x30));
auto *const markerStorageButton =
    reinterpret_cast<KisStorageChooserWidget *>(quintptr(0x40));
auto *const markerView =
    reinterpret_cast<KisResourceItemListView *>(quintptr(0x50));
auto *const markerFrame = reinterpret_cast<QFrame *>(quintptr(0x60));
auto *const markerSplitter = reinterpret_cast<QSplitter *>(quintptr(0x70));
auto *const markerImportButton = reinterpret_cast<QToolButton *>(quintptr(0x80));
auto *const markerDeleteButton = reinterpret_cast<QToolButton *>(quintptr(0x90));

void resetPresentationState()
{
    initialLayout = KisResourceItemChooser::Layout::NotSet;
    layoutChangeCount = 0;
    listViewModeCount = 0;
    capturedListViewMode = ListViewMode::IconGrid;
    taggingBarCount = 0;
    capturedTaggingBarVisible = false;
    viewModeButtonCount = 0;
    capturedViewModeButtonVisible = false;
    storageButtonCount = 0;
    capturedStorageButtonVisible = false;
    rowHeightCount = 0;
    capturedRowHeight = 0;
    columnWidthCount = 0;
    capturedColumnWidth = 0;
    delegateCount = 0;
    capturedDelegate = nullptr;
    importExportCount = 0;
    capturedImportExportVisible = false;
    previewOrientationCount = 0;
    capturedPreviewOrientation = Qt::Horizontal;
    configuredViewSize = QSize(320, 180);
    viewSizeReceivedMarker = false;
    iconUpdateCount = 0;
    emitListViewModeOnRelayout = false;
    relayoutViewMode = ListViewMode::IconGrid;
}

void resetSelectionState()
{
    configuredCurrentIndexSelected = false;
    configuredSelectionModel = nullptr;
    configuredResourceIndex = QModelIndex();
    currentIndexSelectionCount = 0;
    resourceIndexCount = 0;
    setCurrentIndexCount = 0;
    capturedCurrentIndex = QModelIndex();
    rowCountCount = 0;
    columnCountCount = 0;
    indexCount = 0;
    resourceForIndexCount = 0;
    resourceNameCount = 0;
    indexForRowCount = 0;
    capturedRow = -1;
    resourceValidityCount = 0;
    configuredResourceValid = false;
    selectionSourcePointersCorrect = true;
    capturedIndexedResource.clear();
    capturedValidityResource.clear();
    previewIndexes.clear();
    previewTiled = false;
    previewGrayscale = false;
    buttonStateUpdateCount = 0;
}

void resetInputState()
{
    expectedInputChooser = nullptr;
    connectBaseLengthCount = 0;
    disconnectBaseLengthCount = 0;
    configuredBaseLength = 50;
    itemSizeLengths.clear();
    setBaseLengthCount = 0;
    capturedBaseLength = 0;
    cursorUpdateCount = 0;
    capturedScrollerState = QScroller::Inactive;
    inputSourcePointersCorrect = true;
}
}

void KisResourceItemChooser::constructPresentation()
{
    ++constructionCount;
    constructedResourceType = d->resourceType;
    constructedPreview = d->usePreview;
    d->layout = initialLayout;
    d->tagFilterProxyModel = markerFilter;
    d->tagManager = markerManager;
    d->viewModeButton = markerViewButton;
    d->storagePopupButton = markerStorageButton;
    d->view = markerView;
    d->importExportBtns = markerFrame;
    d->resourcesSplitter = markerSplitter;
    d->importButton = markerImportButton;
    d->deleteButton = markerDeleteButton;
}

void KisResourceItemChooser::slotButtonClicked(int)
{
}

void KisResourceItemChooser::changeLayoutBasedOnSize()
{
    ++layoutChangeCount;
    if (emitListViewModeOnRelayout) {
        applyListViewModeAndNotify(relayoutViewMode);
    }
}

void KisResourceItemChooser::scrollBackwards()
{
}

void KisResourceItemChooser::scrollForwards()
{
}

void KisResourceItemChooser::contextMenuRequested(const QPoint &)
{
}

void KisResourceItemChooser::afterFilterChanged()
{
}

void KisResourceItemChooser::slotSaveSplitterState()
{
}

void KisResourceItemChooser::showEvent(QShowEvent *)
{
}

void KisResourceItemChooser::resizeEvent(QResizeEvent *)
{
}

void KisResourceItemChooser::updateButtonState()
{
    ++buttonStateUpdateCount;
}

void KisResourceItemChooser::updatePreview(const QModelIndex &index)
{
    previewIndexes.append(index);
    previewTiled = d->tiledPreview;
    previewGrayscale = d->grayscalePreview;
}

namespace KisResourceItemChooserPresentationSource
{
void setListViewMode(KisResourceItemListView *view, ListViewMode mode)
{
    QCOMPARE(view, markerView);
    ++listViewModeCount;
    capturedListViewMode = mode;
}
}

namespace KisResourceItemChooserSelectionSource
{
bool currentIndexIsSelected(KisResourceItemListView *view)
{
    selectionSourcePointersCorrect &= view == markerView;
    ++currentIndexSelectionCount;
    return configuredCurrentIndexSelected;
}

QModelIndex indexForResource(KisTagFilterResourceProxyModel *model,
                             KoResourceSP resource)
{
    selectionSourcePointersCorrect &= model == markerFilter;
    ++resourceIndexCount;
    capturedIndexedResource = resource;
    return configuredResourceIndex;
}

void setCurrentIndex(KisResourceItemListView *view, const QModelIndex &index)
{
    QCOMPARE(view, markerView);
    ++setCurrentIndexCount;
    capturedCurrentIndex = index;
}

int rowCount(KisTagFilterResourceProxyModel *model)
{
    selectionSourcePointersCorrect &= model == markerFilter;
    ++rowCountCount;
    return configuredSelectionModel ? configuredSelectionModel->rowCount() : 0;
}

int columnCount(KisTagFilterResourceProxyModel *model)
{
    selectionSourcePointersCorrect &= model == markerFilter;
    ++columnCountCount;
    return configuredSelectionModel ? configuredSelectionModel->columnCount()
                                    : 0;
}

QModelIndex index(KisTagFilterResourceProxyModel *model, int row, int column)
{
    selectionSourcePointersCorrect &= model == markerFilter;
    ++indexCount;
    return configuredSelectionModel
        ? configuredSelectionModel->index(row, column)
        : QModelIndex();
}

KoResourceSP resourceForIndex(KisTagFilterResourceProxyModel *model,
                              const QModelIndex &index)
{
    selectionSourcePointersCorrect &= model == markerFilter;
    ++resourceForIndexCount;
    if (index.row() == 0 && index.column() == 0) {
        return firstResource;
    }
    if (index.row() == 0 && index.column() == 1) {
        return secondResource;
    }
    return thirdResource;
}

QString resourceName(KoResourceSP resource)
{
    ++resourceNameCount;
    if (resource == firstResource) {
        return QStringLiteral("first");
    }
    if (resource == secondResource) {
        return QStringLiteral("second");
    }
    return QStringLiteral("third");
}

QModelIndex indexForRow(KisResourceItemListView *view, int row)
{
    selectionSourcePointersCorrect &= view == markerView;
    ++indexForRowCount;
    capturedRow = row;
    return configuredSelectionModel
        ? configuredSelectionModel->index(row, 0)
        : QModelIndex();
}

bool resourceIsValid(KoResourceSP resource)
{
    ++resourceValidityCount;
    capturedValidityResource = resource;
    return configuredResourceValid;
}
}

namespace KisResourceItemChooserInputSource
{
void connectBaseLength(KisResourceItemChooser *chooser)
{
    inputSourcePointersCorrect &= chooser == expectedInputChooser;
    ++connectBaseLengthCount;
}

void disconnectBaseLength(KisResourceItemChooser *chooser)
{
    inputSourcePointersCorrect &= chooser == expectedInputChooser;
    ++disconnectBaseLengthCount;
}

int baseLength()
{
    return configuredBaseLength;
}

void setBaseLength(int length)
{
    ++setBaseLengthCount;
    capturedBaseLength = length;
    configuredBaseLength = length;
}

void setItemSize(KisResourceItemListView *view, int length)
{
    inputSourcePointersCorrect &= view == markerView;
    itemSizeLengths.append(length);
}

void updateCursor(QWidget *widget, QScroller::State state)
{
    inputSourcePointersCorrect &= widget == expectedInputChooser;
    ++cursorUpdateCount;
    capturedScrollerState = state;
}
}

namespace KisResourceItemChooserPresentationSource
{
void showTaggingBar(KisResourceTaggingManager *manager, bool visible)
{
    QCOMPARE(manager, markerManager);
    ++taggingBarCount;
    capturedTaggingBarVisible = visible;
}

void showViewModeButton(KisPopupButton *button, bool visible)
{
    QCOMPARE(button, markerViewButton);
    ++viewModeButtonCount;
    capturedViewModeButtonVisible = visible;
}

void showStorageButton(KisStorageChooserWidget *button, bool visible)
{
    QCOMPARE(button, markerStorageButton);
    ++storageButtonCount;
    capturedStorageButtonVisible = visible;
}

void setRowHeight(KisResourceItemListView *view, int height)
{
    QCOMPARE(view, markerView);
    ++rowHeightCount;
    capturedRowHeight = height;
}

void setColumnWidth(KisResourceItemListView *view, int width)
{
    QCOMPARE(view, markerView);
    ++columnWidthCount;
    capturedColumnWidth = width;
}

void setItemDelegate(KisResourceItemListView *view,
                     QAbstractItemDelegate *delegate)
{
    QCOMPARE(view, markerView);
    ++delegateCount;
    capturedDelegate = delegate;
}

void showImportExportButtons(QFrame *frame, bool visible)
{
    QCOMPARE(frame, markerFrame);
    ++importExportCount;
    capturedImportExportVisible = visible;
}

void setPreviewOrientation(QSplitter *splitter, Qt::Orientation orientation)
{
    QCOMPARE(splitter, markerSplitter);
    ++previewOrientationCount;
    capturedPreviewOrientation = orientation;
}

QSize viewSize(KisResourceItemListView *view)
{
    viewSizeReceivedMarker = view == markerView;
    return configuredViewSize;
}

void updateIcons(KisPopupButton *viewModeButton,
                 QToolButton *importButton,
                 QToolButton *deleteButton,
                 KisStorageChooserWidget *storageButton,
                 KisResourceTaggingManager *tagManager)
{
    QCOMPARE(viewModeButton, markerViewButton);
    QCOMPARE(importButton, markerImportButton);
    QCOMPARE(deleteButton, markerDeleteButton);
    QCOMPARE(storageButton, markerStorageButton);
    QCOMPARE(tagManager, markerManager);
    ++iconUpdateCount;
}
}

QString KoResource::storageLocation() const
{
    return {};
}

bool KoResource::valid() const
{
    return false;
}

QString KoResource::md5Sum(bool) const
{
    return {};
}

int KoResource::version() const
{
    return 0;
}

QString KoResource::filename() const
{
    return {};
}

class TestResourceUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void descriptorPreservesCatalogIdentity();
    void chooserTypesKeepStableValues();
    void chooserOwnsDescriptorAndDelegatesPresentation();
    void chooserDestructionInvalidatesGuard();
    void responsivenessRelayoutsOnlyOnEnableTransitions();
    void listAndTagPresentationDelegateToOwnedComponents();
    void buttonAndPreviewControlsPreserveRequestedState();
    void updateViewRefreshesOwnedIcons();
    void currentResourceTracksVisibleAndHiddenSelections();
    void activationSelectsValidResourcesAndBlocksReentry();
    void nameAndRowSelectionResolveIndexes();
    void previewModesAffectTheNextPreview();
    void clickingSelectedResourceEmitsCurrentResource();
    void responsiveLayoutForwardsViewModeNotification();
    void synchronizationTracksBaseLengthWhileEnabled();
    void controlWheelAdjustsSynchronizedBaseLength();
    void scrollerStateUpdatesTheChooserCursor();
};

void TestResourceUiContract::descriptorPreservesCatalogIdentity()
{
    const KisResourceUiDescriptor descriptor(ResourceType::Palettes, true);

    QCOMPARE(descriptor.resourceType(), ResourceType::Palettes);
    QVERIFY(descriptor.previewEnabled());
}

void TestResourceUiContract::chooserTypesKeepStableValues()
{
    QCOMPARE(int(KisResourceItemChooser::Button_Import), 0);
    QCOMPARE(int(KisResourceItemChooser::Button_Remove), 1);
    QCOMPARE(int(KisResourceItemChooser::Layout::NotSet), 0);
    QCOMPARE(int(KisResourceItemChooser::Layout::Vertical), 1);
    QCOMPARE(int(KisResourceItemChooser::Layout::Horizontal2Rows), 2);
    QCOMPARE(int(KisResourceItemChooser::Layout::Horizontal1Row), 3);
}

void TestResourceUiContract::chooserOwnsDescriptorAndDelegatesPresentation()
{
    const KisResourceUiDescriptor descriptor(ResourceType::Patterns, false);
    QWidget parent;
    constructionCount = 0;
    constructedResourceType.clear();
    constructedPreview = true;
    KisResourceItemChooser chooser(descriptor, &parent);

    QCOMPARE(chooser.parentWidget(), &parent);
    QCOMPARE(constructionCount, 1);
    QCOMPARE(constructedResourceType, ResourceType::Patterns);
    QVERIFY(!constructedPreview);
    QCOMPARE(chooser.descriptor().resourceType(), ResourceType::Patterns);
    QVERIFY(!chooser.descriptor().previewEnabled());
}

void TestResourceUiContract::chooserDestructionInvalidatesGuard()
{
    constructionCount = 0;
    QPointer<KisResourceItemChooser> chooser = new KisResourceItemChooser(
        KisResourceUiDescriptor(ResourceType::Palettes, true));

    QCOMPARE(constructionCount, 1);
    QVERIFY(chooser);
    delete chooser.data();
    QVERIFY(chooser.isNull());
}

void TestResourceUiContract::responsivenessRelayoutsOnlyOnEnableTransitions()
{
    resetPresentationState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));

    chooser.setResponsiveness(false);
    QCOMPARE(layoutChangeCount, 0);
    chooser.setResponsiveness(true);
    QCOMPARE(layoutChangeCount, 1);
    chooser.setResponsiveness(true);
    QCOMPARE(layoutChangeCount, 1);
    chooser.setResponsiveness(false);
    chooser.setResponsiveness(true);
    QCOMPARE(layoutChangeCount, 2);
}

void TestResourceUiContract::listAndTagPresentationDelegateToOwnedComponents()
{
    resetPresentationState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    auto *delegate =
        reinterpret_cast<QAbstractItemDelegate *>(quintptr(0xa0));

    chooser.setListViewMode(ListViewMode::Detail);
    QCOMPARE(listViewModeCount, 1);
    QCOMPARE(capturedListViewMode, ListViewMode::Detail);
    chooser.showTaggingBar(true);
    QCOMPARE(taggingBarCount, 1);
    QVERIFY(capturedTaggingBarVisible);
    QCOMPARE(chooser.tagFilterModel(), markerFilter);
    chooser.setRowHeight(37);
    QCOMPARE(rowHeightCount, 1);
    QCOMPARE(capturedRowHeight, 37);
    chooser.setColumnWidth(41);
    QCOMPARE(columnWidthCount, 1);
    QCOMPARE(capturedColumnWidth, 41);
    chooser.setItemDelegate(delegate);
    QCOMPARE(delegateCount, 1);
    QCOMPARE(capturedDelegate, delegate);
    QCOMPARE(chooser.viewSize(), QSize(320, 180));
    QVERIFY(viewSizeReceivedMarker);
    QCOMPARE(chooser.itemView(), markerView);

    resetPresentationState();
    initialLayout = KisResourceItemChooser::Layout::Horizontal1Row;
    KisResourceItemChooser horizontalChooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    horizontalChooser.setListViewMode(ListViewMode::IconGrid);
    QCOMPARE(listViewModeCount, 0);
}

void TestResourceUiContract::buttonAndPreviewControlsPreserveRequestedState()
{
    resetPresentationState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));

    chooser.showViewModeBtn(true);
    QCOMPARE(viewModeButtonCount, 1);
    QVERIFY(capturedViewModeButtonVisible);
    QCOMPARE(chooser.viewModeButton(), markerViewButton);
    chooser.showStorageBtn(false);
    QCOMPARE(storageButtonCount, 1);
    QVERIFY(!capturedStorageButtonVisible);
    chooser.showImportExportBtns(false);
    QCOMPARE(importExportCount, 1);
    QVERIFY(!capturedImportExportVisible);
    chooser.setPreviewOrientation(Qt::Vertical);
    QCOMPARE(previewOrientationCount, 1);
    QCOMPARE(capturedPreviewOrientation, Qt::Vertical);
}

void TestResourceUiContract::updateViewRefreshesOwnedIcons()
{
    resetPresentationState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));

    chooser.updateView();
    QCOMPARE(iconUpdateCount, 1);
}

void TestResourceUiContract::currentResourceTracksVisibleAndHiddenSelections()
{
    resetSelectionState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));

    chooser.setCurrentResource(firstResource);
    QCOMPARE(resourceIndexCount, 1);
    QVERIFY(capturedIndexedResource == firstResource);
    QCOMPARE(setCurrentIndexCount, 1);
    QVERIFY(!capturedCurrentIndex.isValid());
    QCOMPARE(previewIndexes, QList<QModelIndex> {QModelIndex()});
    QVERIFY(!chooser.currentResource());
    QVERIFY(chooser.currentResource(true) == firstResource);

    configuredCurrentIndexSelected = true;
    QVERIFY(chooser.currentResource() == firstResource);
    QCOMPARE(currentIndexSelectionCount, 2);
    QVERIFY(selectionSourcePointersCorrect);
}

void TestResourceUiContract::activationSelectsValidResourcesAndBlocksReentry()
{
    resetSelectionState();
    QStandardItemModel model(1, 1);
    configuredSelectionModel = &model;
    configuredResourceValid = true;
    configuredCurrentIndexSelected = true;
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    KoResourceSP selectedResource;
    int selectedCount = 0;
    connect(&chooser,
            &KisResourceItemChooser::resourceSelected,
            &chooser,
            [&](KoResourceSP resource) {
                ++selectedCount;
                selectedResource = resource;
                chooser.setCurrentResource(reentrantResource);
            });

    QVERIFY(QMetaObject::invokeMethod(&chooser,
                                      "activate",
                                      Q_ARG(QModelIndex, model.index(0, 0))));
    QCOMPARE(resourceValidityCount, 1);
    QVERIFY(capturedValidityResource == firstResource);
    QCOMPARE(selectedCount, 1);
    QVERIFY(selectionSourcePointersCorrect);
    QVERIFY(selectedResource == firstResource);
    QCOMPARE(resourceIndexCount, 0);
    QCOMPARE(previewIndexes, QList<QModelIndex> {model.index(0, 0)});
    QCOMPARE(buttonStateUpdateCount, 1);
    QVERIFY(chooser.currentResource() == firstResource);

    configuredResourceValid = false;
    QVERIFY(QMetaObject::invokeMethod(&chooser,
                                      "activate",
                                      Q_ARG(QModelIndex, model.index(0, 0))));
    QCOMPARE(selectedCount, 1);
}

void TestResourceUiContract::nameAndRowSelectionResolveIndexes()
{
    resetSelectionState();
    QStandardItemModel model(2, 2);
    configuredSelectionModel = &model;
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));

    chooser.setCurrentResource(QStringLiteral("second"));
    QCOMPARE(setCurrentIndexCount, 1);
    QCOMPARE(capturedCurrentIndex, model.index(0, 1));
    QCOMPARE(previewIndexes, QList<QModelIndex> {model.index(0, 1)});

    chooser.setCurrentItem(1);
    QCOMPARE(indexForRowCount, 1);
    QCOMPARE(capturedRow, 1);
    QCOMPARE(setCurrentIndexCount, 2);
    QCOMPARE(capturedCurrentIndex, model.index(1, 0));
    QCOMPARE(previewIndexes.last(), model.index(1, 0));

    chooser.setCurrentItem(5);
    QCOMPARE(indexForRowCount, 2);
    QCOMPARE(setCurrentIndexCount, 2);
    QCOMPARE(previewIndexes.size(), 2);
    QVERIFY(selectionSourcePointersCorrect);
}

void TestResourceUiContract::previewModesAffectTheNextPreview()
{
    resetSelectionState();
    QStandardItemModel model(1, 1);
    configuredSelectionModel = &model;
    configuredResourceIndex = model.index(0, 0);
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));

    chooser.setPreviewTiled(true);
    chooser.setGrayscalePreview(true);
    chooser.setCurrentResource(firstResource);
    QVERIFY(previewTiled);
    QVERIFY(previewGrayscale);

    chooser.setPreviewTiled(false);
    chooser.setGrayscalePreview(false);
    chooser.setCurrentResource(secondResource);
    QVERIFY(!previewTiled);
    QVERIFY(!previewGrayscale);
    QVERIFY(capturedIndexedResource == secondResource);
    QVERIFY(selectionSourcePointersCorrect);
}

void TestResourceUiContract::clickingSelectedResourceEmitsCurrentResource()
{
    resetSelectionState();
    configuredCurrentIndexSelected = true;
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    chooser.setCurrentResource(firstResource);
    KoResourceSP clickedResource;
    int clickedCount = 0;
    connect(&chooser,
            &KisResourceItemChooser::resourceClicked,
            &chooser,
            [&](KoResourceSP resource) {
                ++clickedCount;
                clickedResource = resource;
            });

    QVERIFY(QMetaObject::invokeMethod(&chooser,
                                      "clicked",
                                      Q_ARG(QModelIndex, QModelIndex())));
    QCOMPARE(clickedCount, 1);
    QVERIFY(clickedResource == firstResource);

    configuredCurrentIndexSelected = false;
    QVERIFY(QMetaObject::invokeMethod(&chooser,
                                      "clicked",
                                      Q_ARG(QModelIndex, QModelIndex())));
    QCOMPARE(clickedCount, 1);
    QVERIFY(selectionSourcePointersCorrect);
}

void TestResourceUiContract::responsiveLayoutForwardsViewModeNotification()
{
    resetPresentationState();
    emitListViewModeOnRelayout = true;
    relayoutViewMode = ListViewMode::IconStripHorizontal;
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    int notificationCount = 0;
    ListViewMode notifiedMode = ListViewMode::IconGrid;
    connect(&chooser,
            &KisResourceItemChooser::listViewModeChanged,
            &chooser,
            [&](ListViewMode mode) {
                ++notificationCount;
                notifiedMode = mode;
            });

    chooser.setResponsiveness(true);
    QCOMPARE(listViewModeCount, 1);
    QCOMPARE(capturedListViewMode, ListViewMode::IconStripHorizontal);
    QCOMPARE(notificationCount, 1);
    QCOMPARE(notifiedMode, ListViewMode::IconStripHorizontal);
}

void TestResourceUiContract::synchronizationTracksBaseLengthWhileEnabled()
{
    resetInputState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    expectedInputChooser = &chooser;
    configuredBaseLength = 64;

    chooser.setSynced(true);
    QCOMPARE(connectBaseLengthCount, 1);
    QCOMPARE(itemSizeLengths, QList<int> {64});
    chooser.setSynced(true);
    QCOMPARE(connectBaseLengthCount, 1);
    QCOMPARE(itemSizeLengths, QList<int> {64});
    QVERIFY(QMetaObject::invokeMethod(&chooser,
                                      "baseLengthChanged",
                                      Q_ARG(int, 72)));
    QCOMPARE(itemSizeLengths, (QList<int> {64, 72}));

    chooser.setSynced(false);
    QCOMPARE(disconnectBaseLengthCount, 1);
    QVERIFY(QMetaObject::invokeMethod(&chooser,
                                      "baseLengthChanged",
                                      Q_ARG(int, 80)));
    QCOMPARE(itemSizeLengths, (QList<int> {64, 72}));
    chooser.setSynced(false);
    QCOMPARE(disconnectBaseLengthCount, 1);
    QVERIFY(inputSourcePointersCorrect);
}

void TestResourceUiContract::controlWheelAdjustsSynchronizedBaseLength()
{
    resetInputState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    expectedInputChooser = &chooser;
    chooser.setSynced(true);
    QWheelEvent controlWheel(QPointF(10, 10),
                             QPointF(20, 20),
                             QPoint(),
                             QPoint(0, 120),
                             Qt::NoButton,
                             Qt::ControlModifier,
                             Qt::NoScrollPhase,
                             false);

    QVERIFY(chooser.eventFilter(&chooser, &controlWheel));
    QCOMPARE(setBaseLengthCount, 1);
    QCOMPARE(capturedBaseLength, 60);

    QWheelEvent plainWheel(QPointF(10, 10),
                           QPointF(20, 20),
                           QPoint(),
                           QPoint(0, 120),
                           Qt::NoButton,
                           Qt::NoModifier,
                           Qt::NoScrollPhase,
                           false);
    QVERIFY(!chooser.eventFilter(&chooser, &plainWheel));
    QCOMPARE(setBaseLengthCount, 1);
    chooser.setSynced(false);
    QVERIFY(!chooser.eventFilter(&chooser, &controlWheel));
    QCOMPARE(setBaseLengthCount, 1);
    QVERIFY(inputSourcePointersCorrect);
}

void TestResourceUiContract::scrollerStateUpdatesTheChooserCursor()
{
    resetInputState();
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::Patterns, false));
    expectedInputChooser = &chooser;

    chooser.slotScrollerStateChanged(QScroller::Dragging);
    QCOMPARE(cursorUpdateCount, 1);
    QCOMPARE(capturedScrollerState, QScroller::Dragging);
    QVERIFY(inputSourcePointersCorrect);
}

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
