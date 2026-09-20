/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceCacheDb.h>
#include <KisResourceItemChooser.h>
#include <KisResourceItemChooserSync.h>
#include <KisResourceItemListView.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceModelProvider.h>
#include <KisResourceTypes.h>
#include <KisResourceUserOperations.h>
#include <KisStorageChooserWidget.h>
#include <KisStorageFilterProxyModel.h>
#include <KisStorageModel.h>
#include <KisTagChooserWidget.h>
#include <KisTagFilterResourceProxyModel.h>
#include <KisTagModel.h>
#include <KisTagResourceModel.h>

#include <ResourceTestHelper.h>

#include <QAbstractButton>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QListView>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>

#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <klocalizedstring.h>

#include <functional>

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

bool triggerResourceContextMenuAction(
    KisResourceItemChooser *chooser,
    const std::function<bool(QMenu *)> &triggerAction)
{
    bool actionTriggered = false;
    QTimer actionTimer;
    actionTimer.setInterval(10);
    int attempts = 0;
    QObject::connect(&actionTimer, &QTimer::timeout, chooser, [&] {
        auto *contextMenu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        if (!contextMenu) {
            return;
        }

        if (triggerAction(contextMenu)) {
            actionTriggered = true;
            actionTimer.stop();
            contextMenu->close();
            return;
        }

        if (++attempts == 100) {
            actionTimer.stop();
            contextMenu->close();
        }
    });

    const QPoint menuPosition = chooser->itemView()->visualRect(
        chooser->itemView()->currentIndex()).center();
    QContextMenuEvent event(QContextMenuEvent::Mouse,
                            menuPosition,
                            chooser->itemView()->viewport()->mapToGlobal(menuPosition));
    actionTimer.start();
    QApplication::sendEvent(chooser->itemView()->viewport(), &event);

    return actionTriggered;
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
    void savesTheSelectedTagForTheResourceChooser();
    void assignsAResourceToAnExistingTagFromTheContextMenu();
    void removesAResourceFromTheCurrentTagFromTheContextMenu();
    void createsAndAssignsATagFromTheContextMenu();
    void createsATagFromTheTagOptionsMenu();
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

void TestResourceUiContract::savesTheSelectedTagForTheResourceChooser()
{
    // Consumer: Resource chooser users returning to a brush, pattern, or palette screen.
    // Operation: The chooser adds and selects a custom tag.
    // Observable result: The selected tag is announced and its URL is saved for the next chooser session.
    // Failure impact: The resource screen loses the user's chosen filter or shows a different tag after reopening.
    KisTagModel model(ResourceType::PaintOpPresets);
    KisTagChooserWidget chooser(&model, ResourceType::PaintOpPresets, nullptr);
    const QString tagName = QStringLiteral("UI contract tag");
    chooser.addTag(tagName);

    const KisTagSP tag = model.tagForUrl(tagName);
    QVERIFY(tag);

    QSignalSpy tagChosenSpy(&chooser, &KisTagChooserWidget::sigTagChosen);
    chooser.setCurrentItem(tag->url());

    QCOMPARE(tagChosenSpy.count(), 1);
    QVERIFY(chooser.currentlySelectedTag() == tag);
    KConfigGroup selectedTags = KSharedConfig::openConfig()->group("SelectedTags");
    QCOMPARE(selectedTags.readEntry<QString>(ResourceType::PaintOpPresets, QString()), tag->url());
}

void TestResourceUiContract::assignsAResourceToAnExistingTagFromTheContextMenu()
{
    // Consumer: Resource chooser users assigning a brush or pattern to an existing tag from its context menu.
    // Operation: The user triggers the menu action for an available tag.
    // Observable result: The selected resource is recorded under that tag.
    // Failure impact: The tag menu reports success but the resource cannot be found through the chosen tag.
    KisResourceModel resourceModel(ResourceType::PaintOpPresets);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    const KoResourceSP resource = resourceModel.resourceForIndex(resourceModel.index(0, 0));
    QVERIFY(resource);

    KisTagModel tagModel(ResourceType::PaintOpPresets);
    const QString tagName = QStringLiteral("Context action tag");
    tagModel.addTag(tagName, true, {});
    const KisTagSP tag = tagModel.tagForUrl(tagName);
    QVERIFY(tag);

    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));
    chooser.resize(600, 400);
    chooser.showTaggingBar(true);
    chooser.show();
    auto *tagChooser = chooser.findChild<KisTagChooserWidget *>();
    QVERIFY(tagChooser);
    tagChooser->setCurrentItem(KisAllTagsModel::urlAll());
    chooser.setCurrentResource(resource);
    QTRY_VERIFY(chooser.itemView()->currentIndex().isValid());

    const bool actionTriggered = triggerResourceContextMenuAction(&chooser, [&](QMenu *contextMenu) {
        for (QAction *rootAction : contextMenu->actions()) {
            QMenu *subMenu = rootAction->menu();
            if (!subMenu) {
                continue;
            }
            for (QAction *candidate : subMenu->actions()) {
                if (candidate->text() == tag->name()) {
                    candidate->trigger();
                    return true;
                }
            }
        }
        return false;
    });

    QVERIFY(actionTriggered);

    KisTagResourceModel taggedResources(ResourceType::PaintOpPresets);
    QVERIFY(taggedResources.isResourceTagged(tag, resource->resourceId()));
}

void TestResourceUiContract::removesAResourceFromTheCurrentTagFromTheContextMenu()
{
    // Consumer: Resource chooser users removing a brush or pattern from the current tag.
    // Operation: The user triggers the menu action that removes the selected resource from that tag.
    // Observable result: The tag no longer contains the selected resource.
    // Failure impact: Resources remain in a tag the user explicitly removed them from, making filtering misleading.
    KisResourceModel resourceModel(ResourceType::PaintOpPresets);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    const KoResourceSP resource = resourceModel.resourceForIndex(resourceModel.index(0, 0));
    QVERIFY(resource);

    KisTagModel tagModel(ResourceType::PaintOpPresets);
    const QString tagName = QStringLiteral("Context removal tag");
    tagModel.addTag(tagName, true, {});
    const KisTagSP tag = tagModel.tagForUrl(tagName);
    QVERIFY(tag);
    KisTagResourceModel taggedResources(ResourceType::PaintOpPresets);
    taggedResources.tagResources(tag, QVector<int>() << resource->resourceId());
    QVERIFY(taggedResources.isResourceTagged(tag, resource->resourceId()));

    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));
    chooser.resize(600, 400);
    chooser.showTaggingBar(true);
    chooser.show();
    auto *tagChooser = chooser.findChild<KisTagChooserWidget *>();
    QVERIFY(tagChooser);
    tagChooser->setCurrentItem(tag->url());
    chooser.setCurrentResource(resource);
    QTRY_VERIFY(chooser.itemView()->currentIndex().isValid());

    const bool actionTriggered = triggerResourceContextMenuAction(&chooser, [](QMenu *contextMenu) {
        for (QAction *candidate : contextMenu->actions()) {
            if (candidate->text() == i18n("Remove from this tag")) {
                candidate->trigger();
                return true;
            }
        }
        return false;
    });

    QVERIFY(actionTriggered);
    QVERIFY(!taggedResources.isResourceTagged(tag, resource->resourceId()));
}

void TestResourceUiContract::createsAndAssignsATagFromTheContextMenu()
{
    // Consumer: Resource chooser users creating a tag while assigning a brush or pattern.
    // Operation: The user enters a new tag name in the context-menu action and submits it.
    // Observable result: The new tag exists and contains the selected resource.
    // Failure impact: The new tag or its assignment disappears, so the user cannot filter to the resource they just organized.
    KisResourceModel resourceModel(ResourceType::PaintOpPresets);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    const KoResourceSP resource = resourceModel.resourceForIndex(resourceModel.index(0, 0));
    QVERIFY(resource);

    KisTagModel tagModel(ResourceType::PaintOpPresets);
    const QString tagName = QStringLiteral("Context menu new tag");
    KisResourceItemChooser chooser(
        KisResourceUiDescriptor(ResourceType::PaintOpPresets, false));
    chooser.resize(600, 400);
    chooser.showTaggingBar(true);
    chooser.show();
    auto *tagChooser = chooser.findChild<KisTagChooserWidget *>();
    QVERIFY(tagChooser);
    tagChooser->setCurrentItem(KisAllTagsModel::urlAll());
    chooser.setCurrentResource(resource);
    QTRY_VERIFY(chooser.itemView()->currentIndex().isValid());

    const bool actionTriggered = triggerResourceContextMenuAction(&chooser, [&](QMenu *contextMenu) {
        for (QAction *rootAction : contextMenu->actions()) {
            QMenu *subMenu = rootAction->menu();
            if (!subMenu) {
                continue;
            }
            for (QAction *candidate : subMenu->actions()) {
                auto *inputAction = qobject_cast<QWidgetAction *>(candidate);
                if (!inputAction) {
                    continue;
                }

                QLineEdit *tagNameEdit = inputAction->defaultWidget()->findChild<QLineEdit *>();
                QPushButton *submitButton = inputAction->defaultWidget()->findChild<QPushButton *>();
                if (tagNameEdit && submitButton) {
                    tagNameEdit->setText(tagName);
                    submitButton->click();
                    return true;
                }
            }
        }
        return false;
    });

    QVERIFY(actionTriggered);

    const KisTagSP tag = tagModel.tagForUrl(tagName);
    QVERIFY(tag);
    KisTagResourceModel taggedResources(ResourceType::PaintOpPresets);
    QVERIFY(taggedResources.isResourceTagged(tag, resource->resourceId()));
}

void TestResourceUiContract::createsATagFromTheTagOptionsMenu()
{
    // Consumer: Resource chooser users creating a tag from the tag options button.
    // Operation: The user opens the tag options menu, enters a name, and submits it.
    // Observable result: The new tag is available for filtering resources.
    // Failure impact: Users cannot create organizational tags from the chooser's tag controls.
    KisTagModel tagModel(ResourceType::PaintOpPresets);
    KisTagChooserWidget chooser(&tagModel, ResourceType::PaintOpPresets, nullptr);
    chooser.resize(300, 40);
    chooser.show();

    QToolButton *tagOptionsButton = nullptr;
    for (QToolButton *candidate : chooser.findChildren<QToolButton *>()) {
        if (candidate->menu()) {
            tagOptionsButton = candidate;
            break;
        }
    }
    QVERIFY(tagOptionsButton);
    QMenu *menu = tagOptionsButton->menu();
    QVERIFY(menu);

    const QString tagName = QStringLiteral("Tag options new tag");
    bool actionTriggered = false;
    QTimer actionTimer;
    actionTimer.setInterval(10);
    int attempts = 0;
    connect(&actionTimer, &QTimer::timeout, &chooser, [&] {
        auto *popup = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        if (!popup) {
            return;
        }

        for (QAction *candidate : popup->actions()) {
            auto *inputAction = qobject_cast<QWidgetAction *>(candidate);
            if (!inputAction || !inputAction->isVisible()) {
                continue;
            }

            QLineEdit *tagNameEdit = inputAction->defaultWidget()->findChild<QLineEdit *>();
            QPushButton *submitButton = inputAction->defaultWidget()->findChild<QPushButton *>();
            if (tagNameEdit && submitButton) {
                tagNameEdit->setText(tagName);
                submitButton->click();
                actionTriggered = true;
                actionTimer.stop();
                popup->close();
                return;
            }
        }

        if (++attempts == 100) {
            actionTimer.stop();
            popup->close();
        }
    });

    actionTimer.start();
    QTest::mouseClick(tagOptionsButton, Qt::LeftButton);

    QVERIFY(actionTriggered);
    QVERIFY(tagModel.tagForUrl(tagName));
}

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
