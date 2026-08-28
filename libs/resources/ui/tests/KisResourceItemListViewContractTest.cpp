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
#include <QContextMenuEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollBar>
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

class KisResourceItemListViewContractAccess
{
public:
    static void sendResizeEvent(KisResourceItemListView &view,
                                const QSize &size,
                                const QSize &oldSize)
    {
        QResizeEvent event(size, oldSize);
        view.resizeEvent(&event);
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
    void constructorUsesIconGridDefaults();
    void listModesAndItemSizeControlLayout();
    void strictSelectionPreventsDeselectAndClearsRemovedItem();
    void fixedToolTipSizeControlsDocumentThumbnail();
    void checkerToolTipSettingControlsTransparency();
    void selectionEmitsCurrentResourceChanged();
    void clickEmitsCurrentResourceClicked();
    void contextMenuEmitsGlobalPosition();
    void resizeDoesNotEmitDeclaredSizeSignal();
    void scrollerStateChangesCursor();
    void destructionInvalidatesGuardedPointer();
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

void KisResourceItemListViewContractTest::constructorUsesIconGridDefaults()
{
    ExposedResourceItemListView view;

    QCOMPARE(view.selectionMode(), QAbstractItemView::SingleSelection);
    QCOMPARE(view.contextMenuPolicy(), Qt::DefaultContextMenu);
    QCOMPARE(view.resizeMode(), QListView::Adjust);
    QVERIFY(view.uniformItemSizes());
    QCOMPARE(view.viewMode(), QListView::IconMode);
    QCOMPARE(view.gridSize(), QSize(56, 56));
    QCOMPARE(view.iconSize(), QSize(56, 56));
}

void KisResourceItemListViewContractTest::listModesAndItemSizeControlLayout()
{
    ExposedResourceItemListView view;
    view.resize(200, 100);
    const QSize requestedSize(40, 30);

    view.setItemSize(requestedSize);
    QCOMPARE(view.gridSize(), requestedSize);
    QCOMPARE(view.iconSize(), requestedSize);

    view.setListViewMode(ListViewMode::IconStripHorizontal);
    QCOMPARE(view.viewMode(), QListView::IconMode);
    QCOMPARE(view.flow(), QListView::LeftToRight);
    QVERIFY(!view.isWrapping());
    QCOMPARE(view.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    KisResourceItemListViewContractAccess::sendResizeEvent(
        view, QSize(200, 24), QSize(200, 100));
    QCOMPARE(view.gridSize(), QSize(24, 24));
    QCOMPARE(view.iconSize(), QSize(24, 24));

    view.setListViewMode(ListViewMode::Detail);
    QCOMPARE(view.viewMode(), QListView::ListMode);
    QCOMPARE(view.flow(), QListView::TopToBottom);
    QVERIFY(!view.isWrapping());
    QCOMPARE(view.verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(view.horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(view.gridSize().height(), requestedSize.height());
    QCOMPARE(view.iconSize(), requestedSize);

    view.setListViewMode(ListViewMode::IconGrid);
    QCOMPARE(view.viewMode(), QListView::IconMode);
    QCOMPARE(view.flow(), QListView::LeftToRight);
    QVERIFY(view.isWrapping());
    QCOMPARE(view.gridSize(), requestedSize);
    QCOMPARE(view.iconSize(), requestedSize);
}

void KisResourceItemListViewContractTest::strictSelectionPreventsDeselectAndClearsRemovedItem()
{
    ExposedResourceItemListView view;
    QStandardItemModel model(2, 1);
    view.setModel(&model);
    const QModelIndex index = model.index(0, 0);
    view.setCurrentIndex(index);
    view.selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    view.setStrictSelectionMode(true);

    model.removeRow(0);
    QVERIFY(!view.selectionModel()->hasSelection());
}

void KisResourceItemListViewContractTest::fixedToolTipSizeControlsDocumentThumbnail()
{
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
}

void KisResourceItemListViewContractTest::checkerToolTipSettingControlsTransparency()
{
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
    ExposedResourceItemListView view;
    QSignalSpy contextMenuSpy(&view, &KisResourceItemListView::contextMenuRequested);
    const QPoint globalPosition(140, 260);

    view.sendContextMenuEvent(QPoint(2, 3), globalPosition);

    QCOMPARE(contextMenuSpy.size(), 1);
    QCOMPARE(contextMenuSpy.at(0).at(0).toPoint(), globalPosition);
}

void KisResourceItemListViewContractTest::resizeDoesNotEmitDeclaredSizeSignal()
{
    ExposedResourceItemListView view;
    QSignalSpy sizeSpy(&view, &KisResourceItemListView::sigSizeChanged);

    KisResourceItemListViewContractAccess::sendResizeEvent(
        view, QSize(110, 110), QSize(100, 100));

    QCOMPARE(sizeSpy.size(), 0);
}

void KisResourceItemListViewContractTest::scrollerStateChangesCursor()
{
    ExposedResourceItemListView view;

    view.slotScrollerStateChange(QScroller::Pressed);
    QCOMPARE(view.cursor().shape(), Qt::OpenHandCursor);

    view.slotScrollerStateChange(QScroller::Dragging);
    QCOMPARE(view.cursor().shape(), Qt::ClosedHandCursor);

    view.slotScrollerStateChange(QScroller::Inactive);
    QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
}

void KisResourceItemListViewContractTest::destructionInvalidatesGuardedPointer()
{
    QPointer<KisResourceItemListView> view = new KisResourceItemListView;

    QVERIFY(view);
    delete view.data();
    QVERIFY(view.isNull());
}

QTEST_MAIN(KisResourceItemListViewContractTest)

#include "KisResourceItemListViewContractTest.moc"
