/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisStorageChooserDelegate.h>

#include <KisStorageModel.h>

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTest>

namespace
{
QStyleOptionViewItem delegateOption()
{
    QStyleOptionViewItem option;
    option.rect = QRect(10, 10, 220, 60);
    option.decorationSize = QSize(32, 32);
    option.palette = qApp->palette();
    option.state = QStyle::State_Enabled;
    return option;
}

QImage paintStorage(KisStorageChooserDelegate &delegate,
                    const QStyleOptionViewItem &option,
                    const QModelIndex &index)
{
    QImage canvas(240, 80, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    delegate.paint(&painter, option, index);
    return canvas;
}
}

class KisStorageChooserDelegateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void providesStorageCellDimensions();
    void paintsThumbnailAndActivationState();
};

void KisStorageChooserDelegateContractTest::providesStorageCellDimensions()
{
    // Consumer: Resource chooser users opening the storage selection popup.
    // Operation: The popup asks its delegate for the cell size of a storage entry.
    // Observable result: The storage name and activation control receive a 200-pixel-wide cell below its preview.
    // Failure impact: Storage entries are clipped or overlap in the chooser popup.
    KisStorageChooserDelegate delegate;
    QStyleOptionViewItem option;
    option.decorationSize = QSize(64, 32);

    QCOMPARE(delegate.sizeHint(option, QModelIndex()), QSize(200, 40));
}

void KisStorageChooserDelegateContractTest::paintsThumbnailAndActivationState()
{
    // Consumer: Resource manager users enabling or disabling a storage.
    // Operation: The storage popup paints the same storage while inactive and active.
    // Observable result: The thumbnail remains visible and the cell rendering changes with the activation state.
    // Failure impact: Users cannot tell whether a bundle or folder is active before choosing resources from it.
    KisStorageChooserDelegate delegate;
    const QStyleOptionViewItem option = delegateOption();
    QStandardItemModel model(1, KisStorageModel::DisplayName + 1);
    const QModelIndex index = model.index(0, 0);
    QImage thumbnail(4, 4, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    model.setData(model.index(0, KisStorageModel::DisplayName),
                  QStringLiteral("Bundle_Name"),
                  Qt::DisplayRole);
    model.setData(index, QStringLiteral("Bundle"),
                  Qt::UserRole + KisStorageModel::StorageType);
    model.setData(index, thumbnail, Qt::UserRole + KisStorageModel::Thumbnail);

    model.setData(index, false, Qt::UserRole + KisStorageModel::Active);
    const QImage inactive = paintStorage(delegate, option, index);
    QCOMPARE(inactive.pixelColor(14, 14), QColor(Qt::red));

    model.setData(index, true, Qt::UserRole + KisStorageModel::Active);
    const QImage active = paintStorage(delegate, option, index);
    QCOMPARE(active.pixelColor(14, 14), QColor(Qt::red));
    QVERIFY(active != inactive);
}

QTEST_MAIN(KisStorageChooserDelegateContractTest)

#include "KisStorageChooserDelegateContractTest.moc"
