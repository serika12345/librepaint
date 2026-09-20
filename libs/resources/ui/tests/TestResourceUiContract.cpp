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
#include <KisResourceUserOperations.h>
#include <KisStorageChooserWidget.h>
#include <KisStorageFilterProxyModel.h>
#include <KisStorageModel.h>
#include <KisTagFilterResourceProxyModel.h>

#include <ResourceTestHelper.h>

#include <QApplication>
#include <QAbstractButton>
#include <QDir>
#include <QListView>
#include <QMessageBox>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>

#include <kconfiggroup.h>
#include <ksharedconfig.h>

namespace
{
QListView *storagePopupView()
{
    for (QWidget *widget : QApplication::allWidgets()) {
        auto *view = qobject_cast<QListView *>(widget);
        auto *model = view ? qobject_cast<KisStorageFilterProxyModel *>(view->model()) : nullptr;
        if (model && model->sourceModel() == KisStorageModel::instance()) {
            return view;
        }
    }

    return nullptr;
}

void answerNextMessageBox(QMessageBox::StandardButton answer,
                          bool *wasShown,
                          QString *messageText = nullptr)
{
    QTimer::singleShot(0, [answer, wasShown, messageText] {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!messageBox) {
            return;
        }

        *wasShown = true;
        if (messageText) {
            *messageText = messageBox->text();
        }

        if (QAbstractButton *button = messageBox->button(answer)) {
            button->click();
        } else {
            messageBox->reject();
        }
    });
}
}

class TestResourceUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void selectsAnInstalledResource();
    void synchronizesPreviewCellSizeAcrossChoosers();
    void togglesTheStorageShownInChooserPopup();
    void asksBeforeOverwritingAResourceFile();
    void preservesAResourceWhenDuplicateRenameIsCancelled();
    void reportsAnImportFailure();
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

void TestResourceUiContract::togglesTheStorageShownInChooserPopup()
{
    // Consumer: Resource chooser users enabling or disabling an installed bundle.
    // Operation: The user opens the storage popup and clicks a displayed bundle.
    // Observable result: The displayed bundle changes active state and can be restored by clicking it again.
    // Failure impact: A different storage is disabled, so the user loses resources they did not choose to hide.
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));
    chooser.resize(600, 400);
    chooser.show();

    auto *storageButton = chooser.findChild<KisStorageChooserWidget *>();
    QVERIFY(storageButton);
    QTest::mouseClick(storageButton, Qt::LeftButton);
    QTRY_VERIFY(storageButton->isPopupWidgetVisible());
    QTRY_VERIFY(storagePopupView());

    QListView *view = storagePopupView();
    auto *model = qobject_cast<KisStorageFilterProxyModel *>(view->model());
    QVERIFY(model);
    QVERIFY(model->rowCount() > 0);

    const QModelIndex displayedIndex = model->index(0, KisStorageModel::Id);
    const QModelIndex sourceIndex = model->mapToSource(displayedIndex);
    QVERIFY(sourceIndex.isValid());
    const bool initiallyActive = sourceIndex.data(Qt::UserRole + KisStorageModel::Active).toBool();

    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                      view->visualRect(displayedIndex).center());
    QTRY_COMPARE(sourceIndex.data(Qt::UserRole + KisStorageModel::Active).toBool(), !initiallyActive);

    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                      view->visualRect(displayedIndex).center());
    QTRY_COMPARE(sourceIndex.data(Qt::UserRole + KisStorageModel::Active).toBool(), initiallyActive);
}

void TestResourceUiContract::asksBeforeOverwritingAResourceFile()
{
    // Consumer: Resource importers replacing an existing brush, pattern, or workspace file.
    // Operation: The importer accepts or cancels the overwrite confirmation.
    // Observable result: The confirmation identifies the file and returns the chosen decision.
    // Failure impact: A user overwrites a resource without an understandable confirmation or cannot approve a requested replacement.
    bool acceptedPromptShown = false;
    QString acceptedPromptText;
    answerNextMessageBox(QMessageBox::Yes, &acceptedPromptShown, &acceptedPromptText);
    QVERIFY(KisResourceUserOperations::userAllowsOverwrite(
        nullptr, QStringLiteral("/tmp/resources/brush.bundle")));
    QVERIFY(acceptedPromptShown);
    QVERIFY(acceptedPromptText.contains(QStringLiteral("brush.bundle")));
    QVERIFY(!acceptedPromptText.contains(QStringLiteral("/tmp/resources")));

    bool cancelledPromptShown = false;
    answerNextMessageBox(QMessageBox::Cancel, &cancelledPromptShown);
    QVERIFY(!KisResourceUserOperations::userAllowsOverwrite(
        nullptr, QStringLiteral("/tmp/resources/brush.bundle")));
    QVERIFY(cancelledPromptShown);
}

void TestResourceUiContract::preservesAResourceWhenDuplicateRenameIsCancelled()
{
    // Consumer: Resource editors renaming a brush, gradient, palette, or workspace.
    // Operation: The editor requests a name already used by another installed resource and cancels the confirmation.
    // Observable result: The stored resource keeps its original name.
    // Failure impact: Cancelling a duplicate-name warning still changes the resource shown in chooser and editor screens.
    KisResourceModel model(ResourceType::PaintOpPresets);
    model.setResourceFilter(KisResourceModel::ShowAllResources);
    QVERIFY(model.rowCount() >= 2);

    const KoResourceSP resource = model.resourceForIndex(model.index(0, 0));
    const KoResourceSP duplicate = model.resourceForIndex(model.index(1, 0));
    QVERIFY(resource);
    QVERIFY(duplicate);
    const QString originalName = resource->name();

    bool promptShown = false;
    answerNextMessageBox(QMessageBox::Cancel, &promptShown);
    QVERIFY(!KisResourceUserOperations::renameResourceWithUserInput(
        nullptr, resource, duplicate->name()));
    QVERIFY(promptShown);

    KisResourceModel persistedModel(ResourceType::PaintOpPresets);
    persistedModel.setResourceFilter(KisResourceModel::ShowAllResources);
    const KoResourceSP persisted = persistedModel.resourceForId(resource->resourceId());
    QVERIFY(persisted);
    QCOMPARE(persisted->name(), originalName);
}

void TestResourceUiContract::reportsAnImportFailure()
{
    // Consumer: Resource chooser users importing a file from disk.
    // Operation: The user imports a missing resource file.
    // Observable result: The operation returns no resource and presents a failure warning.
    // Failure impact: The requested resource is absent with no explanation, so the user cannot correct the import path or file.
    bool warningShown = false;
    answerNextMessageBox(QMessageBox::Ok, &warningShown);
    const KoResourceSP imported = KisResourceUserOperations::importResourceFileWithUserInput(
        nullptr,
        QString(),
        ResourceType::PaintOpPresets,
        QStringLiteral("/tmp/librepaint-missing-resource.kpp"));
    QVERIFY(imported.isNull());
    QVERIFY(warningShown);
}

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
