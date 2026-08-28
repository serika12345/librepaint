/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <QPointer>

#include <KisResourceItemChooser.h>
#include <KisResourceItemChooserPresentationSource_p.h>
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

void KisResourceItemChooser::slotScrollerStateChanged(QScroller::State)
{
}

void KisResourceItemChooser::changeLayoutBasedOnSize()
{
    ++layoutChangeCount;
}

void KisResourceItemChooser::scrollBackwards()
{
}

void KisResourceItemChooser::scrollForwards()
{
}

void KisResourceItemChooser::activate(const QModelIndex &)
{
}

void KisResourceItemChooser::clicked(const QModelIndex &)
{
}

void KisResourceItemChooser::contextMenuRequested(const QPoint &)
{
}

void KisResourceItemChooser::baseLengthChanged(int)
{
}

void KisResourceItemChooser::afterFilterChanged()
{
}

void KisResourceItemChooser::slotSaveSplitterState()
{
}

bool KisResourceItemChooser::eventFilter(QObject *, QEvent *)
{
    return false;
}

void KisResourceItemChooser::showEvent(QShowEvent *)
{
}

void KisResourceItemChooser::resizeEvent(QResizeEvent *)
{
}

namespace KisResourceItemChooserPresentationSource
{
void setListViewMode(KisResourceItemListView *view, ListViewMode mode)
{
    QCOMPARE(view, markerView);
    ++listViewModeCount;
    capturedListViewMode = mode;
}

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

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
