/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemListWidget.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QCoreApplication>
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

class KisResourceItemListWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsWithSelectableThumbnailGridForBundleCreator();
    void switchesBundleResourcesBetweenThumbnailsAndDetails();
};

void KisResourceItemListWidgetContractTest::startsWithSelectableThumbnailGridForBundleCreator()
{
    // Consumer: the bundle-creation page that displays resources selected for an export bundle.
    // Operation: Create the selected-resources widget and Ctrl-click two resource thumbnails.
    // Observable result: Both resources stay selected in a visible thumbnail grid.
    // Failure impact: Users cannot review and remove multiple resources from the bundle they are creating.
    KisResourceItemListWidget widget;
    widget.resize(160, 160);
    widget.addItem(QStringLiteral("Brush"));
    widget.addItem(QStringLiteral("Pattern"));
    widget.show();
    QCoreApplication::processEvents();

    QCOMPARE(widget.viewMode(), QListView::IconMode);
    QCOMPARE(widget.gridSize(), widget.iconSize());
    QVERIFY(widget.gridSize().width() > 0);
    QVERIFY(widget.gridSize().height() > 0);

    const QModelIndex brush = widget.model()->index(0, 0);
    const QModelIndex pattern = widget.model()->index(1, 0);
    QTest::mouseClick(widget.viewport(), Qt::LeftButton, Qt::NoModifier,
                      widget.visualRect(brush).center());
    QTest::mouseClick(widget.viewport(), Qt::LeftButton, Qt::ControlModifier,
                      widget.visualRect(pattern).center());

    QCOMPARE(widget.selectedItems().size(), 2);
}

void KisResourceItemListWidgetContractTest::switchesBundleResourcesBetweenThumbnailsAndDetails()
{
    // Consumer: the bundle-creation view-mode control backed by saved thumbnail or detail preference.
    // Operation: Set the resource cell size and switch the selected-resource widget to detail view and back.
    // Observable result: Resource cells keep their requested dimensions in both thumbnail and detail presentation.
    // Failure impact: Restored display preferences produce overlapping or incorrectly sized bundle resources.
    KisResourceItemListWidget widget;
    widget.resize(200, 100);
    const QSize requestedSize(40, 30);

    widget.setItemSize(requestedSize);
    QCOMPARE(widget.gridSize(), requestedSize);
    QCOMPARE(widget.iconSize(), requestedSize);

    widget.setListViewMode(ListViewMode::Detail);
    QCOMPARE(widget.viewMode(), QListView::ListMode);
    QCOMPARE(widget.gridSize().height(), requestedSize.height());
    QCOMPARE(widget.iconSize(), requestedSize);

    widget.setListViewMode(ListViewMode::IconGrid);
    QCOMPARE(widget.viewMode(), QListView::IconMode);
    QCOMPARE(widget.gridSize(), requestedSize);
    QCOMPARE(widget.iconSize(), requestedSize);
}

QTEST_MAIN(KisResourceItemListWidgetContractTest)

#include "KisResourceItemListWidgetContractTest.moc"
