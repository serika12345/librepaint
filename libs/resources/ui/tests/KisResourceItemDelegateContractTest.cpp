/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemDelegate.h>

#include <KisResourceCacheDb.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceModelProvider.h>
#include <KisResourceTypes.h>

#include <ResourceTestHelper.h>

#include <QAbstractListModel>
#include <QDir>
#include <QPainter>
#include <QStandardPaths>
#include <QStyleOptionViewItem>
#include <QTest>

#include <kconfiggroup.h>
#include <ksharedconfig.h>

#include <utility>

class DelegateModel : public QAbstractListModel
{
public:
    DelegateModel(QString filename,
                  QImage thumbnail,
                  int resourceId = 1,
                  QString resourceType = QStringLiteral("patterns"))
        : m_filename(std::move(filename))
        , m_thumbnail(std::move(thumbnail))
        , m_resourceId(resourceId)
        , m_resourceType(std::move(resourceType))
    {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() != 0) {
            return {};
        }

        switch (role) {
        case Qt::UserRole + KisAbstractResourceModel::Id:
            return m_resourceId;
        case Qt::UserRole + KisAbstractResourceModel::Name:
            return QStringLiteral("Pattern_Name");
        case Qt::UserRole + KisAbstractResourceModel::Location:
            return QStringLiteral("bundle.asl");
        case Qt::UserRole + KisAbstractResourceModel::ResourceType:
            return m_resourceType;
        case Qt::UserRole + KisAbstractResourceModel::Filename:
            return m_filename;
        case Qt::UserRole + KisAbstractResourceModel::Thumbnail:
            return m_thumbnail;
        default:
            return {};
        }
    }

private:
    QString m_filename;
    QImage m_thumbnail;
    int m_resourceId;
    QString m_resourceType;
};

QStyleOptionViewItem delegateOption(const QRect &rect)
{
    QStyleOptionViewItem option;
    option.rect = rect;
    option.decorationSize = QSize(9, 7);
    option.palette.setColor(QPalette::Window, Qt::yellow);
    option.palette.setColor(QPalette::Highlight, Qt::green);
    return option;
}

QImage paintDelegate(KisResourceItemDelegate &delegate,
                     const QModelIndex &index,
                     const QStyleOptionViewItem &option)
{
    QImage canvas(option.rect.size(), QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    delegate.paint(&painter, option, index);
    painter.end();
    return canvas;
}

class KisResourceItemDelegateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void providesRequestedPreviewCellSize();
    void showTextControlsThumbnailLayout();
    void selectionStateReachesThumbnailPainter();
    void bundlePreviewUsesGlobalResourceThumbnail();
};

void KisResourceItemDelegateContractTest::initTestCase()
{
    ResourceTestHelper::initTestDb();
    ResourceTestHelper::createDummyLoaderRegistry();

    const QString sourceLocation = QStringLiteral(RESOURCE_TEST_DATA_DIR);
    QVERIFY2(QDir(sourceLocation).exists(), sourceLocation.toUtf8());

    const QString destinationLocation = ResourceTestHelper::filesDestDir();
    ResourceTestHelper::cleanDstLocation(destinationLocation);

    KConfigGroup config(KSharedConfig::openConfig(), "");
    config.writeEntry(KisResourceLocator::resourceLocationKey, destinationLocation);

    QVERIFY(KisResourceCacheDb::initialize(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)));
    QVERIFY(KisResourceLocator::instance()->initialize(sourceLocation) == KisResourceLocator::LocatorError::Ok);
}

void KisResourceItemDelegateContractTest::cleanupTestCase()
{
    KisResourceModelProvider::testingCloseAllQueries();
    ResourceTestHelper::rmTestDb();
    ResourceTestHelper::cleanDstLocation(ResourceTestHelper::filesDestDir());
}

void KisResourceItemDelegateContractTest::providesRequestedPreviewCellSize()
{
    // Consumer: Resource chooser views arranging preview cells.
    // Operation: The view asks the delegate for the size of a resource preview.
    // Observable result: The returned cell size matches the view's requested preview decoration.
    // Failure impact: Resource previews are clipped or leave inconsistent gaps in chooser grids.
    KisResourceItemDelegate delegate;
    const QStyleOptionViewItem option = delegateOption(QRect(0, 0, 20, 8));

    QCOMPARE(delegate.sizeHint(option, QModelIndex()), QSize(9, 7));
}

void KisResourceItemDelegateContractTest::showTextControlsThumbnailLayout()
{
    // Consumer: Resource manager users switching a resource row to its detailed presentation.
    // Operation: The resource view enables text while painting a preview item.
    // Observable result: The thumbnail moves from the full cell to the leading preview area.
    // Failure impact: Resource names and thumbnails overlap, making the selected resource hard to identify.
    KisAllResourcesModel *resourceModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    const QModelIndex resourceIndex = resourceModel->index(0, 0);
    QVERIFY(resourceIndex.isValid());
    resourceIndex.data(Qt::UserRole + KisAbstractResourceModel::Thumbnail);
    KisResourceItemDelegate delegate;
    const QStyleOptionViewItem option = delegateOption(QRect(0, 0, 20, 8));

    const QImage imageOnly = paintDelegate(delegate, resourceIndex, option);
    QVERIFY(imageOnly.pixelColor(10, 4).alpha() != 0);

    delegate.setShowText(true);
    const QImage withText = paintDelegate(delegate, resourceIndex, option);
    QCOMPARE(withText.pixelColor(10, 4).alpha(), 0);
}

void KisResourceItemDelegateContractTest::selectionStateReachesThumbnailPainter()
{
    // Consumer: Resource chooser users selecting a resource preview.
    // Operation: The view paints an item with its selected state.
    // Observable result: The selection color surrounds the resource thumbnail without replacing its image.
    // Failure impact: Users cannot distinguish the selected resource from neighboring previews.
    KisAllResourcesModel *resourceModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    const QModelIndex resourceIndex = resourceModel->index(0, 0);
    QVERIFY(resourceIndex.isValid());
    resourceIndex.data(Qt::UserRole + KisAbstractResourceModel::Thumbnail);
    KisResourceItemDelegate delegate;
    QStyleOptionViewItem option = delegateOption(QRect(0, 0, 6, 6));
    option.state = QStyle::State_Selected;

    const QImage selected = paintDelegate(delegate, resourceIndex, option);

    QCOMPARE(selected.pixelColor(0, 0), QColor(Qt::green));
    QVERIFY(selected.pixelColor(3, 3) != QColor(Qt::green));
}

void KisResourceItemDelegateContractTest::bundlePreviewUsesGlobalResourceThumbnail()
{
    // Consumer: Bundle creator users reviewing resources selected from a local list.
    // Operation: The list paints a local row that identifies an installed resource by type and ID.
    // Observable result: The row shows the matching installed resource's thumbnail.
    // Failure impact: The bundle creator displays an unrelated or blank preview for a selected resource.
    KisAllResourcesModel *globalModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    const QModelIndex globalIndex = globalModel->index(0, 0);
    QVERIFY(globalIndex.isValid());

    const int resourceId = globalIndex.data(Qt::UserRole + KisAbstractResourceModel::Id).toInt();
    const QString resourceType = globalIndex.data(Qt::UserRole + KisAbstractResourceModel::ResourceType).toString();
    QVERIFY(resourceId >= 0);
    QVERIFY(!resourceType.isEmpty());
    globalIndex.data(Qt::UserRole + KisAbstractResourceModel::Thumbnail);

    QImage localThumbnail(2, 2, QImage::Format_ARGB32);
    localThumbnail.fill(Qt::red);
    DelegateModel localModel(QStringLiteral("local.png"), localThumbnail, resourceId, resourceType);
    KisResourceItemDelegate globalDelegate;
    KisResourceItemDelegate delegate;
    delegate.setNeedIndexConversion(true);
    const QStyleOptionViewItem option = delegateOption(QRect(0, 0, 6, 6));

    const QImage expected = paintDelegate(globalDelegate, globalIndex, option);
    const QImage canvas = paintDelegate(delegate, localModel.index(0, 0), option);

    QCOMPARE(canvas, expected);
}

QTEST_MAIN(KisResourceItemDelegateContractTest)

#include "KisResourceItemDelegateContractTest.moc"
