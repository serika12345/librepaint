/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemView.h>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KisResourceThumbnailStorageLocation.h>

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QPointer>
#include <QResizeEvent>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTest>

QString KisResourceThumbnailStorageLocation::makeAbsolute(const QString &storageLocation)
{
    return storageLocation;
}

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class ExposedResourceItemView : public KisResourceItemView
{
public:
    using KisResourceItemView::setViewMode;

    void sendContextMenuEvent(const QPoint &position, const QPoint &globalPosition)
    {
        QContextMenuEvent event(QContextMenuEvent::Mouse, position, globalPosition);
        contextMenuEvent(&event);
    }

    void sendResizeEvent(const QSize &size, const QSize &oldSize)
    {
        QResizeEvent event(size, oldSize);
        resizeEvent(&event);
    }
};

class KisResourceItemViewContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void constructorUsesFixedColumns();
    void fixedRowsSwitchScrollPolicies();
    void selectionEmitsCurrentResourceChanged();
    void repeatedClickEmitsCurrentResourceClicked();
    void contextMenuEmitsGlobalPosition();
    void resizeEmitsSizeChanged();
    void scrollerStateChangesCursor();
    void destructionInvalidatesGuardedPointer();
};

void KisResourceItemViewContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KisResourceItemViewContractTest::init()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry("KineticScrollingEnabled", false);
    config.sync();
}

void KisResourceItemViewContractTest::cleanupTestCase()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry("KineticScrollingEnabled");
    config.sync();
}

void KisResourceItemViewContractTest::constructorUsesFixedColumns()
{
    ExposedResourceItemView view;

    QCOMPARE(view.selectionMode(), QAbstractItemView::SingleSelection);
    QVERIFY(view.verticalHeader()->isHidden());
    QVERIFY(view.horizontalHeader()->isHidden());
    QCOMPARE(view.verticalHeader()->defaultSectionSize(),
             qMax(20, view.verticalHeader()->minimumSectionSize()));
    QCOMPARE(view.contextMenuPolicy(), Qt::DefaultContextMenu);
    QCOMPARE(view.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(view.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOn);
}

void KisResourceItemViewContractTest::fixedRowsSwitchScrollPolicies()
{
    ExposedResourceItemView view;

    view.setViewMode(KisResourceItemView::FIXED_ROWS);

    QCOMPARE(view.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOn);
    QCOMPARE(view.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
}

void KisResourceItemViewContractTest::selectionEmitsCurrentResourceChanged()
{
    ExposedResourceItemView view;
    QStandardItemModel model(1, 1);
    view.setModel(&model);
    QSignalSpy changedSpy(&view, &KisResourceItemView::currentResourceChanged);
    const QModelIndex index = model.index(0, 0);

    view.selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(changedSpy.at(0).at(0).value<QModelIndex>(), index);

    view.selectionModel()->clearSelection();
    QCOMPARE(changedSpy.size(), 2);
    QVERIFY(!changedSpy.at(1).at(0).value<QModelIndex>().isValid());
}

void KisResourceItemViewContractTest::repeatedClickEmitsCurrentResourceClicked()
{
    ExposedResourceItemView view;
    QStandardItemModel model(1, 1);
    view.setModel(&model);
    view.resize(100, 100);
    view.show();
    const QModelIndex index = model.index(0, 0);
    view.setCurrentIndex(index);
    QCoreApplication::processEvents();
    QSignalSpy clickedSpy(&view, &KisResourceItemView::currentResourceClicked);

    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier,
                      view.visualRect(index).center());

    QCOMPARE(clickedSpy.size(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).value<QModelIndex>(), index);
}

void KisResourceItemViewContractTest::contextMenuEmitsGlobalPosition()
{
    ExposedResourceItemView view;
    QSignalSpy contextMenuSpy(&view, &KisResourceItemView::contextMenuRequested);
    const QPoint globalPosition(120, 240);

    view.sendContextMenuEvent(QPoint(3, 4), globalPosition);

    QCOMPARE(contextMenuSpy.size(), 1);
    QCOMPARE(contextMenuSpy.at(0).at(0).toPoint(), globalPosition);
}

void KisResourceItemViewContractTest::resizeEmitsSizeChanged()
{
    ExposedResourceItemView view;
    QSignalSpy sizeSpy(&view, &KisResourceItemView::sigSizeChanged);

    view.sendResizeEvent(QSize(110, 110), QSize(100, 100));

    QCOMPARE(sizeSpy.size(), 1);
}

void KisResourceItemViewContractTest::scrollerStateChangesCursor()
{
    ExposedResourceItemView view;

    view.slotScrollerStateChange(QScroller::Pressed);
    QCOMPARE(view.cursor().shape(), Qt::OpenHandCursor);

    view.slotScrollerStateChange(QScroller::Dragging);
    QCOMPARE(view.cursor().shape(), Qt::ClosedHandCursor);

    view.slotScrollerStateChange(QScroller::Inactive);
    QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
}

void KisResourceItemViewContractTest::destructionInvalidatesGuardedPointer()
{
    QPointer<KisResourceItemView> view = new KisResourceItemView;

    QVERIFY(view);
    delete view.data();
    QVERIFY(view.isNull());
}

QTEST_MAIN(KisResourceItemViewContractTest)

#include "KisResourceItemViewContractTest.moc"
