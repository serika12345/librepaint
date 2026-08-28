/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemListWidget.h>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KisIconToolTip.h>
#include <KisResourceModel.h>
#include <KisResourceThumbnailCache.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QApplication>
#include <QContextMenuEvent>
#include <QListWidgetItem>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalSpy>
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

class KisResourceItemListWidgetContractAccess
{
public:
    static void sendResizeEvent(KisResourceItemListWidget &widget,
                                const QSize &size,
                                const QSize &oldSize)
    {
        QResizeEvent event(size, oldSize);
        widget.resizeEvent(&event);
    }
};

class ExposedResourceItemListWidget : public KisResourceItemListWidget
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

QModelIndex addThumbnailItem(KisResourceItemListWidget &widget,
                             const QString &filename,
                             const QImage &thumbnail)
{
    auto *item = new QListWidgetItem(QStringLiteral("Pattern"), &widget);
    item->setData(Qt::DecorationRole, thumbnail);
    item->setData(Qt::UserRole + KisAbstractResourceModel::Location,
                  QStringLiteral("bundle.asl"));
    item->setData(Qt::UserRole + KisAbstractResourceModel::ResourceType,
                  QStringLiteral("patterns"));
    item->setData(Qt::UserRole + KisAbstractResourceModel::Filename, filename);
    return widget.model()->index(0, 0);
}

class KisResourceItemListWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void constructorUsesIconGridDefaults();
    void listModesAndItemSizeControlLayout();
    void strictSelectionClearsRemovedCurrentItem();
    void fixedToolTipSizeControlsDocumentThumbnail();
    void checkerToolTipSettingControlsTransparency();
    void selectionEmitsCurrentResourceChanged();
    void clickEmitsCurrentResourceClicked();
    void contextMenuEmitsGlobalPosition();
    void resizeDoesNotEmitDeclaredSizeSignal();
    void scrollerStateChangesCursor();
    void destructionInvalidatesGuardedPointer();
};

void KisResourceItemListWidgetContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KisResourceItemListWidgetContractTest::init()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry("KineticScrollingEnabled", false);
    config.sync();
}

void KisResourceItemListWidgetContractTest::cleanupTestCase()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry("KineticScrollingEnabled");
    config.sync();
}

void KisResourceItemListWidgetContractTest::constructorUsesIconGridDefaults()
{
    ExposedResourceItemListWidget widget;

    QCOMPARE(widget.selectionMode(), QAbstractItemView::ExtendedSelection);
    QCOMPARE(widget.contextMenuPolicy(), Qt::DefaultContextMenu);
    QCOMPARE(widget.resizeMode(), QListView::Adjust);
    QCOMPARE(widget.movement(), QListView::Static);
    QVERIFY(widget.uniformItemSizes());
    QCOMPARE(widget.viewMode(), QListView::IconMode);
    QCOMPARE(widget.gridSize(), QSize(56, 56));
    QCOMPARE(widget.iconSize(), QSize(56, 56));
}

void KisResourceItemListWidgetContractTest::listModesAndItemSizeControlLayout()
{
    ExposedResourceItemListWidget widget;
    widget.resize(200, 100);
    const QSize requestedSize(40, 30);

    widget.setItemSize(requestedSize);
    QCOMPARE(widget.gridSize(), requestedSize);
    QCOMPARE(widget.iconSize(), requestedSize);

    widget.setListViewMode(ListViewMode::IconStripHorizontal);
    QCOMPARE(widget.viewMode(), QListView::IconMode);
    QCOMPARE(widget.flow(), QListView::LeftToRight);
    QVERIFY(!widget.isWrapping());
    QCOMPARE(widget.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    KisResourceItemListWidgetContractAccess::sendResizeEvent(
        widget, QSize(200, 24), QSize(200, 100));
    QCOMPARE(widget.gridSize(), QSize(24, 24));
    QCOMPARE(widget.iconSize(), QSize(24, 24));

    widget.setListViewMode(ListViewMode::Detail);
    QCOMPARE(widget.viewMode(), QListView::ListMode);
    QCOMPARE(widget.flow(), QListView::TopToBottom);
    QVERIFY(!widget.isWrapping());
    QCOMPARE(widget.verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(widget.horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(widget.gridSize(), QSize(widget.width(), 24));
    QCOMPARE(widget.iconSize(), QSize(24, 24));

    widget.setListViewMode(ListViewMode::IconGrid);
    QCOMPARE(widget.viewMode(), QListView::IconMode);
    QCOMPARE(widget.flow(), QListView::LeftToRight);
    QVERIFY(widget.isWrapping());
    QCOMPARE(widget.gridSize(), QSize(24, 24));
    QCOMPARE(widget.iconSize(), QSize(24, 24));
}

void KisResourceItemListWidgetContractTest::strictSelectionClearsRemovedCurrentItem()
{
    ExposedResourceItemListWidget widget;
    widget.setSelectionMode(QAbstractItemView::SingleSelection);
    widget.addItem(QStringLiteral("First"));
    widget.addItem(QStringLiteral("Second"));
    widget.setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    widget.setStrictSelectionMode(true);

    delete widget.takeItem(0);

    QVERIFY(!widget.selectionModel()->hasSelection());
}

void KisResourceItemListWidgetContractTest::fixedToolTipSizeControlsDocumentThumbnail()
{
    QImage thumbnail(4, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    ExposedResourceItemListWidget widget;
    const QModelIndex index = addThumbnailItem(widget, QStringLiteral("fixed-widget.png"), thumbnail);
    KisResourceQueryMapper::insert(
        *KisResourceThumbnailCache::instance(),
        {QStringLiteral("/normalized/bundle.asl"), QStringLiteral("patterns/fixed-widget.png")},
        thumbnail);
    widget.setFixedToolTipThumbnailSize(QSize(2, 2));
    KisIconToolTip *toolTip = currentIconToolTip();

    QVERIFY(toolTip);
    QTextDocument *document = KisIconToolTipContractAccess::createDocument(*toolTip, index);
    QCOMPARE(documentThumbnail(document).deviceIndependentSize(), QSizeF(2, 1));
}

void KisResourceItemListWidgetContractTest::checkerToolTipSettingControlsTransparency()
{
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::transparent);
    ExposedResourceItemListWidget widget;
    const QModelIndex index = addThumbnailItem(widget, QStringLiteral("checker-widget.png"), thumbnail);
    KisIconToolTip *toolTip = currentIconToolTip();
    QVERIFY(toolTip);

    widget.setToolTipShouldRenderCheckers(true);
    QTextDocument *checkerDocument = KisIconToolTipContractAccess::createDocument(*toolTip, index);
    QCOMPARE(documentThumbnail(checkerDocument).pixelColor(0, 0).alpha(), 255);

    widget.setToolTipShouldRenderCheckers(false);
    QTextDocument *plainDocument = KisIconToolTipContractAccess::createDocument(*toolTip, index);
    QCOMPARE(documentThumbnail(plainDocument).pixelColor(0, 0).alpha(), 0);
}

void KisResourceItemListWidgetContractTest::selectionEmitsCurrentResourceChanged()
{
    ExposedResourceItemListWidget widget;
    widget.addItem(QStringLiteral("Pattern"));
    QSignalSpy changedSpy(&widget, &KisResourceItemListWidget::currentResourceChanged);
    const QModelIndex index = widget.model()->index(0, 0);

    widget.selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(changedSpy.at(0).at(0).value<QModelIndex>(), index);

    widget.selectionModel()->clearSelection();
    QCOMPARE(changedSpy.size(), 2);
    QVERIFY(!changedSpy.at(1).at(0).value<QModelIndex>().isValid());
}

void KisResourceItemListWidgetContractTest::clickEmitsCurrentResourceClicked()
{
    ExposedResourceItemListWidget widget;
    widget.addItem(QStringLiteral("Pattern"));
    const QModelIndex index = widget.model()->index(0, 0);
    QSignalSpy clickedSpy(&widget, &KisResourceItemListWidget::currentResourceClicked);

    QVERIFY(QMetaObject::invokeMethod(
        &widget, "clicked", Qt::DirectConnection, Q_ARG(QModelIndex, index)));

    QCOMPARE(clickedSpy.size(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).value<QModelIndex>(), index);
}

void KisResourceItemListWidgetContractTest::contextMenuEmitsGlobalPosition()
{
    ExposedResourceItemListWidget widget;
    QSignalSpy contextMenuSpy(&widget, &KisResourceItemListWidget::contextMenuRequested);
    const QPoint globalPosition(140, 260);

    widget.sendContextMenuEvent(QPoint(2, 3), globalPosition);

    QCOMPARE(contextMenuSpy.size(), 1);
    QCOMPARE(contextMenuSpy.at(0).at(0).toPoint(), globalPosition);
}

void KisResourceItemListWidgetContractTest::resizeDoesNotEmitDeclaredSizeSignal()
{
    ExposedResourceItemListWidget widget;
    QSignalSpy sizeSpy(&widget, &KisResourceItemListWidget::sigSizeChanged);

    KisResourceItemListWidgetContractAccess::sendResizeEvent(
        widget, QSize(110, 110), QSize(100, 100));

    QCOMPARE(sizeSpy.size(), 0);
}

void KisResourceItemListWidgetContractTest::scrollerStateChangesCursor()
{
    ExposedResourceItemListWidget widget;

    widget.slotScrollerStateChange(QScroller::Pressed);
    QCOMPARE(widget.cursor().shape(), Qt::OpenHandCursor);

    widget.slotScrollerStateChange(QScroller::Dragging);
    QCOMPARE(widget.cursor().shape(), Qt::ClosedHandCursor);

    widget.slotScrollerStateChange(QScroller::Inactive);
    QCOMPARE(widget.cursor().shape(), Qt::ArrowCursor);
}

void KisResourceItemListWidgetContractTest::destructionInvalidatesGuardedPointer()
{
    QPointer<KisResourceItemListWidget> widget = new KisResourceItemListWidget;

    QVERIFY(widget);
    delete widget.data();
    QVERIFY(widget.isNull());
}

QTEST_MAIN(KisResourceItemListWidgetContractTest)

#include "KisResourceItemListWidgetContractTest.moc"
