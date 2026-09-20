/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemListView.h>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KisIconToolTip.h>
#include <KisResourceModel.h>
#include <KisResourceThumbnailCache.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTest>
#include <QTextDocument>
#include <QUrl>

QString KisResourceThumbnailStorageLocation::makeAbsolute(const QString &storageLocation)
{
    return QStringLiteral("/normalized/") + storageLocation;
}

class KisResourceQueryMapper
{
public:
    static void insert(KisResourceThumbnailCache &cache,
                       const QPair<QString, QString> &key,
                       const QImage &image)
    {
        cache.insert(key, image);
    }
};

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class KisIconToolTipContractAccess
{
public:
    static QTextDocument *createDocument(KisIconToolTip &toolTip, const QModelIndex &index)
    {
        return toolTip.createDocument(index);
    }
};

class ExposedResourceItemListView : public KisResourceItemListView
{
public:
    void sendContextMenuEvent(const QPoint &position, const QPoint &globalPosition)
    {
        QContextMenuEvent event(QContextMenuEvent::Mouse, position, globalPosition);
        contextMenuEvent(&event);
    }
};

KisIconToolTip *currentIconToolTip()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *toolTip = dynamic_cast<KisIconToolTip *>(widget)) {
            return toolTip;
        }
    }
    return nullptr;
}

QImage documentThumbnail(QTextDocument *document)
{
    return document->resource(QTextDocument::ImageResource, QUrl(QStringLiteral("data:thumbnail")))
        .value<QImage>();
}

QModelIndex configureThumbnailModel(QStandardItemModel &model,
                                    const QString &filename,
                                    const QImage &thumbnail)
{
    model.setRowCount(1);
    model.setColumnCount(1);
    const QModelIndex index = model.index(0, 0);
    model.setData(index, QStringLiteral("Pattern"), Qt::DisplayRole);
    model.setData(index, thumbnail, Qt::DecorationRole);
    model.setData(index, QStringLiteral("bundle.asl"),
                  Qt::UserRole + KisAbstractResourceModel::Location);
    model.setData(index, QStringLiteral("patterns"),
                  Qt::UserRole + KisAbstractResourceModel::ResourceType);
    model.setData(index, filename,
                  Qt::UserRole + KisAbstractResourceModel::Filename);
    return index;
}

class KisResourceItemListViewContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void startsWithThumbnailGridForResourceChooser();
    void selectedViewModesArrangeResourceItems();
    void strictSelectionKeepsCurrentResourceUntilItIsRemoved();
    void fixedToolTipSizeControlsDocumentThumbnail();
    void checkerToolTipSettingControlsTransparency();
    void selectionEmitsCurrentResourceChanged();
    void clickEmitsCurrentResourceClicked();
    void contextMenuEmitsGlobalPosition();
    void scrollerStateChangesCursor();
};

void KisResourceItemListViewContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KisResourceItemListViewContractTest::init()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry("KineticScrollingEnabled", false);
    config.sync();
}

void KisResourceItemListViewContractTest::cleanupTestCase()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry("KineticScrollingEnabled");
    config.sync();
}

void KisResourceItemListViewContractTest::startsWithThumbnailGridForResourceChooser()
{
    // Consumer: resource manager and chooser screens created without a custom view mode.
    // Operation: Create the resource item list view.
    // Observable result: Resources start in a visible thumbnail grid with matching icon and cell sizes.
    // Failure impact: A resource screen can open without usable visual resource previews.
    ExposedResourceItemListView view;

    QCOMPARE(view.viewMode(), QListView::IconMode);
    QCOMPARE(view.gridSize(), view.iconSize());
    QVERIFY(view.gridSize().width() > 0);
    QVERIFY(view.gridSize().height() > 0);
}

void KisResourceItemListViewContractTest::selectedViewModesArrangeResourceItems()
{
    // Consumer: preset, palette, and resource chooser screens that switch their presentation mode.
    // Operation: Set an item size and switch between grid, horizontal strip, and detail modes.
    // Observable result: Each mode exposes resource cells at the requested size.
    // Failure impact: Resource previews overlap, use stale dimensions, or cannot fit their chooser layout.
    ExposedResourceItemListView view;
    view.resize(200, 100);
    view.show();
    QCoreApplication::processEvents();
    const QSize requestedSize(40, 30);

    view.setItemSize(requestedSize);
    QCOMPARE(view.gridSize(), requestedSize);
    QCOMPARE(view.iconSize(), requestedSize);

    view.setListViewMode(ListViewMode::IconStripHorizontal);
    QCOMPARE(view.viewMode(), QListView::IconMode);

    view.setListViewMode(ListViewMode::Detail);
    QCOMPARE(view.viewMode(), QListView::ListMode);
    QCOMPARE(view.gridSize().height(), requestedSize.height());
    QCOMPARE(view.iconSize(), requestedSize);

    view.setListViewMode(ListViewMode::IconGrid);
    QCOMPARE(view.viewMode(), QListView::IconMode);
    QCOMPARE(view.gridSize(), requestedSize);
    QCOMPARE(view.iconSize(), requestedSize);
}

void KisResourceItemListViewContractTest::strictSelectionKeepsCurrentResourceUntilItIsRemoved()
{
    // Consumer: resource choosers that must keep their active brush or pattern selected.
    // Operation: Ctrl-click an already selected resource, then remove that resource from the filtered model.
    // Observable result: Ctrl-click keeps the current selection, while removing the current resource clears it.
    // Failure impact: A chooser can unexpectedly clear the active resource or silently move selection to another one.
    ExposedResourceItemListView view;
    view.resize(120, 120);
    QStandardItemModel model(2, 1);
    model.setData(model.index(0, 0), QStringLiteral("First"));
    model.setData(model.index(1, 0), QStringLiteral("Second"));
    view.setModel(&model);
    const QModelIndex index = model.index(0, 0);
    view.setStrictSelectionMode(true);
    view.show();
    QCoreApplication::processEvents();

    view.setCurrentIndex(index);
    view.selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    QVERIFY(view.selectionModel()->isSelected(index));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.visualRect(index).center());
    QVERIFY(view.selectionModel()->isSelected(index));

    model.removeRow(0);
    QVERIFY(!view.selectionModel()->hasSelection());
}

void KisResourceItemListViewContractTest::fixedToolTipSizeControlsDocumentThumbnail()
{
    // Consumer: resource chooser views that preview patterns and gradients.
    // Operation: Set a fixed tooltip thumbnail size for a resource item.
    // Observable result: The tooltip image keeps the resource colour and fits the requested display bounds.
    // Failure impact: A chooser can show an incorrectly scaled or misleading resource preview.
    QImage thumbnail(4, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    QStandardItemModel model;
    const QModelIndex index = configureThumbnailModel(model, QStringLiteral("fixed-list.png"), thumbnail);
    KisResourceQueryMapper::insert(
        *KisResourceThumbnailCache::instance(),
        {QStringLiteral("/normalized/bundle.asl"), QStringLiteral("patterns/fixed-list.png")},
        thumbnail);
    ExposedResourceItemListView view;
    view.setModel(&model);
    view.setFixedToolTipThumbnailSize(QSize(2, 2));
    KisIconToolTip *toolTip = currentIconToolTip();

    QVERIFY(toolTip);
    QTextDocument *document = KisIconToolTipContractAccess::createDocument(*toolTip, index);
    QCOMPARE(documentThumbnail(document).deviceIndependentSize(), QSizeF(2, 1));
    QCOMPARE(documentThumbnail(document).pixelColor(0, 0), QColor(Qt::red));
}

void KisResourceItemListViewContractTest::checkerToolTipSettingControlsTransparency()
{
    // Consumer: resource chooser views that preview transparent patterns and gradients.
    // Operation: Enable and then disable checker rendering for an item tooltip.
    // Observable result: The tooltip makes transparency visible with checkers only while the setting is enabled.
    // Failure impact: Users cannot reliably distinguish transparent resource content from an empty preview.
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::transparent);
    QStandardItemModel model;
    const QModelIndex index = configureThumbnailModel(model, QStringLiteral("checker-list.png"), thumbnail);
    ExposedResourceItemListView view;
    view.setModel(&model);
    KisIconToolTip *toolTip = currentIconToolTip();
    QVERIFY(toolTip);

    view.setToolTipShouldRenderCheckers(true);
    QTextDocument *checkerDocument = KisIconToolTipContractAccess::createDocument(*toolTip, index);
    QCOMPARE(documentThumbnail(checkerDocument).pixelColor(0, 0).alpha(), 255);

    view.setToolTipShouldRenderCheckers(false);
    QTextDocument *plainDocument = KisIconToolTipContractAccess::createDocument(*toolTip, index);
    QCOMPARE(documentThumbnail(plainDocument).pixelColor(0, 0).alpha(), 0);
}

void KisResourceItemListViewContractTest::selectionEmitsCurrentResourceChanged()
{
    // Consumer: resource choosers that activate and preview the selected resource.
    // Operation: Select a resource and then clear the selection.
    // Observable result: The chooser receives the selected index followed by an invalid index.
    // Failure impact: The active resource or its preview can remain stale after selection changes.
    ExposedResourceItemListView view;
    QStandardItemModel model(1, 1);
    view.setModel(&model);
    QSignalSpy changedSpy(&view, &KisResourceItemListView::currentResourceChanged);
    const QModelIndex index = model.index(0, 0);

    view.selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(changedSpy.at(0).at(0).value<QModelIndex>(), index);

    view.selectionModel()->clearSelection();
    QCOMPARE(changedSpy.size(), 2);
    QVERIFY(!changedSpy.at(1).at(0).value<QModelIndex>().isValid());
}

void KisResourceItemListViewContractTest::clickEmitsCurrentResourceClicked()
{
    // Consumer: resource choosers that commit a user click on the active resource.
    // Operation: Click a resource index in the list view.
    // Observable result: The chooser receives one click notification for that index.
    // Failure impact: A user click cannot trigger the action associated with the selected resource.
    ExposedResourceItemListView view;
    QStandardItemModel model(1, 1);
    view.setModel(&model);
    const QModelIndex index = model.index(0, 0);
    QSignalSpy clickedSpy(&view, &KisResourceItemListView::currentResourceClicked);

    QVERIFY(QMetaObject::invokeMethod(
        &view, "clicked", Qt::DirectConnection, Q_ARG(QModelIndex, index)));

    QCOMPARE(clickedSpy.size(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).value<QModelIndex>(), index);
}

void KisResourceItemListViewContractTest::contextMenuEmitsGlobalPosition()
{
    // Consumer: resource tagging and management menus.
    // Operation: Request a context menu over the resource list.
    // Observable result: The menu handler receives the screen position of the request.
    // Failure impact: The resource action menu opens at the wrong place or cannot be shown.
    ExposedResourceItemListView view;
    QSignalSpy contextMenuSpy(&view, &KisResourceItemListView::contextMenuRequested);
    const QPoint globalPosition(140, 260);

    view.sendContextMenuEvent(QPoint(2, 3), globalPosition);

    QCOMPARE(contextMenuSpy.size(), 1);
    QCOMPARE(contextMenuSpy.at(0).at(0).toPoint(), globalPosition);
}

void KisResourceItemListViewContractTest::scrollerStateChangesCursor()
{
    // Consumer: users dragging a kinetic-scrolling resource list.
    // Operation: Press, drag, and release the scrolling gesture.
    // Observable result: The pointer changes from an open hand to a closed hand and then returns to the normal cursor.
    // Failure impact: The list gives no reliable visual feedback about whether a drag is active.
    ExposedResourceItemListView view;

    view.slotScrollerStateChange(QScroller::Pressed);
    QCOMPARE(view.cursor().shape(), Qt::OpenHandCursor);

    view.slotScrollerStateChange(QScroller::Dragging);
    QCOMPARE(view.cursor().shape(), Qt::ClosedHandCursor);

    view.slotScrollerStateChange(QScroller::Inactive);
    QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
}

QTEST_MAIN(KisResourceItemListViewContractTest)

#include "KisResourceItemListViewContractTest.moc"
