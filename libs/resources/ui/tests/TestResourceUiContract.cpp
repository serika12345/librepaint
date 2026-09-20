/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceCacheDb.h>
#include <KisResourceItemChooser.h>
#include <KisResourceItemChooserSync.h>
#include <KisResourceItemListView.h>
#include <KisResourceLocator.h>
#include <KisResourceModelProvider.h>
#include <KisResourceTypes.h>
#include <KisTagFilterResourceProxyModel.h>

#include <ResourceTestHelper.h>

#include <QDir>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <kconfiggroup.h>
#include <ksharedconfig.h>

class TestResourceUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void selectsAnInstalledResource();
    void synchronizesPreviewCellSizeAcrossChoosers();
};

void TestResourceUiContract::initTestCase()
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

void TestResourceUiContract::cleanupTestCase()
{
    KisResourceModelProvider::testingCloseAllQueries();
    ResourceTestHelper::rmTestDb();
    ResourceTestHelper::cleanDstLocation(ResourceTestHelper::filesDestDir());
}

void TestResourceUiContract::selectsAnInstalledResource()
{
    // Consumer: Brush preset chooser users selecting an installed preset.
    // Operation: The chooser selects the first visible resource through its public item-selection operation.
    // Observable result: The selected resource is returned and announced to the owning screen.
    // Failure impact: The brush or resource manager keeps using a different resource than the one the user selected.
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));
    KisTagFilterResourceProxyModel *model = chooser.tagFilterModel();
    QVERIFY(model);
    QVERIFY(model->rowCount() > 0);

    const QModelIndex index = model->index(0, 0);
    const KoResourceSP expectedResource = model->resourceForIndex(index);
    QVERIFY(expectedResource);
    QCOMPARE(index.data(Qt::UserRole + KisAbstractResourceModel::ResourceType).toString(),
             ResourceType::PaintOpPresets);

    QSignalSpy selectedSpy(&chooser, &KisResourceItemChooser::resourceSelected);
    chooser.setCurrentItem(0);

    QCOMPARE(selectedSpy.count(), 1);
    QVERIFY(chooser.currentResource() == expectedResource);
}

void TestResourceUiContract::synchronizesPreviewCellSizeAcrossChoosers()
{
    // Consumer: Users resizing preset, brush, or gamut-mask chooser previews.
    // Operation: Two synchronized chooser grids receive a shared preview-size change.
    // Observable result: Both grids adopt the same requested cell dimensions.
    // Failure impact: Resource previews disagree in size across screens, making the shared size control unreliable.
    KisResourceItemChooserSync::instance()->setBaseLength(50);
    KisResourceItemChooser firstChooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));
    KisResourceItemChooser secondChooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));

    firstChooser.setSynced(true);
    secondChooser.setSynced(true);
    QCOMPARE(firstChooser.itemView()->gridSize(), QSize(50, 50));
    QCOMPARE(secondChooser.itemView()->gridSize(), QSize(50, 50));

    KisResourceItemChooserSync::instance()->setBaseLength(70);

    QCOMPARE(firstChooser.itemView()->gridSize(), QSize(70, 70));
    QCOMPARE(secondChooser.itemView()->gridSize(), QSize(70, 70));
    KisResourceItemChooserSync::instance()->setBaseLength(50);
}

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
