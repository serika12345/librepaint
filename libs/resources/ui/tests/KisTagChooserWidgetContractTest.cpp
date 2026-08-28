/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisTagChooserWidget.h>
#include <KisTagChooserWidgetAddSource_p.h>
#include <KisTagChooserWidgetConstructionSource_p.h>
#include <KisTagChooserWidgetSelectionSource_p.h>

#include <QComboBox>
#include <QGridLayout>
#include <QPointer>
#include <QSignalBlocker>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>

namespace
{
class TrackingItemModel : public QStandardItemModel
{
public:
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override
    {
        ++sortCount;
        lastSortColumn = column;
        lastSortOrder = order;
        QStandardItemModel::sort(column, order);
    }

    int sortCount = 0;
    int lastSortColumn = -1;
    Qt::SortOrder lastSortOrder = Qt::DescendingOrder;
};

TrackingItemModel presentationModel;
KisTagModel *capturedModel = nullptr;
QString capturedResourceType;
QPointer<QAbstractItemModel> allTagsModel;
QPointer<QWidget> tagToolButton;
int toolConnectionCount = 0;
KisTagSP currentToolTag;
QString persistedResourceType;
KisTagSP persistedTag;
int iconRefreshCount = 0;
QString addTagName;
QString addTagUrl;
KisTagSP addExistingTag;
bool addExistingTagActive = false;
QMessageBox::ButtonRole addNextChoice = QMessageBox::RejectRole;
int addReservedWarningCount = 0;
int addLookupCount = 0;
int addQuestionCount = 0;
int addReactivateCount = 0;
int addNameCount = 0;
int addValueCount = 0;
QString capturedAddLookupUrl;
QString capturedAddName;
QString capturedAddResourceType;
QPointer<QWidget> capturedReservedParent;
bool capturedReservedTagValueInput = false;
QPointer<KisTagChooserWidget> capturedQuestionParent;
bool capturedQuestionTagActive = false;
KisTagSP capturedAddTag;
KoResourceSP capturedAddResource;

KisTagSP markerTag(quintptr value)
{
    return KisTagSP(reinterpret_cast<KisTag *>(value), [](KisTag *) {});
}

KoResourceSP markerResource(quintptr value)
{
    return KoResourceSP(reinterpret_cast<KoResource *>(value),
                        [](KoResource *) {});
}

void resetAddState()
{
    addTagName = QStringLiteral("Fresh");
    addTagUrl = QStringLiteral("fresh");
    addExistingTag.clear();
    addExistingTagActive = false;
    addNextChoice = QMessageBox::RejectRole;
    addReservedWarningCount = 0;
    addLookupCount = 0;
    addQuestionCount = 0;
    addReactivateCount = 0;
    addNameCount = 0;
    addValueCount = 0;
    capturedAddLookupUrl.clear();
    capturedAddName.clear();
    capturedAddResourceType.clear();
    capturedReservedParent.clear();
    capturedReservedTagValueInput = false;
    capturedQuestionParent.clear();
    capturedQuestionTagActive = false;
    capturedAddTag.clear();
    capturedAddResource.clear();
}

void configurePresentationModel()
{
    presentationModel.clear();
    presentationModel.sortCount = 0;
    presentationModel.lastSortColumn = -1;
    presentationModel.lastSortOrder = Qt::DescendingOrder;
    presentationModel.setRowCount(2);
    presentationModel.setColumnCount(1);
    presentationModel.setData(presentationModel.index(0, 0),
                              QStringLiteral("Alpha"),
                              Qt::DisplayRole);
    presentationModel.setData(presentationModel.index(0, 0),
                              QStringLiteral("alpha"),
                              Qt::UserRole + KisAllTagsModel::Url);
    presentationModel.setData(presentationModel.index(0, 0),
                              QVariant::fromValue(markerTag(0x10)),
                              Qt::UserRole + KisAllTagsModel::KisTagRole);
    presentationModel.setData(presentationModel.index(1, 0),
                              QStringLiteral("Beta"),
                              Qt::DisplayRole);
    presentationModel.setData(presentationModel.index(1, 0),
                              QStringLiteral("beta"),
                              Qt::UserRole + KisAllTagsModel::Url);
    presentationModel.setData(presentationModel.index(1, 0),
                              QVariant::fromValue(markerTag(0x20)),
                              Qt::UserRole + KisAllTagsModel::KisTagRole);
}
}

namespace KisTagChooserWidgetSelectionSource
{
void persistSelectedTag(const QString &resourceType, KisTagSP tag)
{
    persistedResourceType = resourceType;
    persistedTag = tag;
}
}

namespace KisTagChooserWidgetConstructionSource
{
QAbstractItemModel *preparePresentationModel(KisTagModel *model)
{
    capturedModel = model;
    return &presentationModel;
}

QAbstractItemModel *createAllTagsModel(const QString &resourceType,
                                       QObject *parent)
{
    capturedResourceType = resourceType;
    allTagsModel = new QStandardItemModel(parent);
    return allTagsModel;
}

void connectTagToolButton(QWidget *, KisTagChooserWidget *)
{
    ++toolConnectionCount;
}
}

namespace KisTagChooserWidgetAddSource
{
QString tagName(KisTagSP)
{
    return addTagName;
}

QString tagUrl(KisTagSP)
{
    return addTagUrl;
}

KisTagSP tagForUrl(KisTagModel *, const QString &url)
{
    ++addLookupCount;
    capturedAddLookupUrl = url;
    return addExistingTag;
}

bool tagIsActive(KisTagSP tag)
{
    capturedAddTag = tag;
    return addExistingTagActive;
}

void warnReservedName(QWidget *parent, bool tagValueInput)
{
    ++addReservedWarningCount;
    capturedReservedParent = parent;
    capturedReservedTagValueInput = tagValueInput;
}

void reactivateTag(KisTagModel *,
                   const QString &resourceType,
                   KisTagSP tag,
                   KoResourceSP resource)
{
    ++addReactivateCount;
    capturedAddResourceType = resourceType;
    capturedAddTag = tag;
    capturedAddResource = resource;
}

void addNewTag(KisTagModel *,
               const QString &tagName,
               KoResourceSP resource)
{
    ++addNameCount;
    capturedAddName = tagName;
    capturedAddResource = resource;
}

void addNewTag(KisTagModel *, KisTagSP tag, KoResourceSP resource)
{
    ++addValueCount;
    capturedAddTag = tag;
    capturedAddResource = resource;
}
}

QWidget *KisTagChooserWidget::createTagToolButton(QWidget *parent)
{
    tagToolButton = new QWidget(parent);
    return tagToolButton;
}

void KisTagChooserWidget::setTagToolButtonCurrentTag(QWidget *toolButton,
                                                      KisTagSP tag)
{
    QCOMPARE(toolButton, tagToolButton.data());
    currentToolTag = tag;
}

void KisTagChooserWidget::refreshTagToolButtonIcons(QWidget *toolButton)
{
    QCOMPARE(toolButton, tagToolButton.data());
    ++iconRefreshCount;
}

QString KoResource::storageLocation() const
{
    return {};
}

bool KoResource::valid() const
{
    return false;
}

QString KoResource::md5Sum(bool) const
{
    return {};
}

int KoResource::version() const
{
    return 0;
}

QString KoResource::filename() const
{
    return {};
}

QStringList KisTag::defaultResources() const
{
    return {};
}

int KisTag::id() const
{
    return -1;
}

QString KisTag::url() const
{
    return {};
}

QString KisTag::name(bool) const
{
    return {};
}

bool KisTag::active() const
{
    return false;
}

QString KisTag::comment(bool) const
{
    return {};
}

QMessageBox::ButtonRole KisTagChooserWidget::overwriteTagDialog(
    KisTagChooserWidget *parent,
    bool tagIsActive)
{
    ++addQuestionCount;
    capturedQuestionParent = parent;
    capturedQuestionTagActive = tagIsActive;
    return addNextChoice;
}

void KisTagChooserWidget::tagToolRenameCurrentTag(const QString &)
{
}

void KisTagChooserWidget::tagToolDeleteCurrentTag()
{
}

void KisTagChooserWidget::tagToolUndeleteLastTag(KisTagSP)
{
}

void KisTagChooserWidget::tagToolContextMenuAboutToShow()
{
}

void KisTagChooserWidget::cacheSelectedTag()
{
}

void KisTagChooserWidget::restoreTagFromCache()
{
}

void KisTagChooserWidget::slotTagModelDataChanged(const QModelIndex &,
                                                   const QModelIndex &,
                                                   const QVector<int>)
{
}

class KisTagChooserWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorOwnsPresentationUnderParent();
    void destructorReleasesOwnedPresentation();
    void selectionFindsExactUrlAndReturnsTag();
    void tagChangePersistsSortsAndEmitsSelection();
    void iconUpdateRefreshesTagToolButton();
    void nameAddRejectsInvalidAndAddsNewValues();
    void existingNameChoiceRestoresCancelsOrReplaces();
    void tagValueUsesNameUrlAndExistingChoice();
};

void KisTagChooserWidgetContractTest::constructorOwnsPresentationUnderParent()
{
    configurePresentationModel();
    capturedModel = nullptr;
    capturedResourceType.clear();
    allTagsModel = nullptr;
    tagToolButton = nullptr;
    toolConnectionCount = 0;
    QWidget parent;
    auto *model = reinterpret_cast<KisTagModel *>(quintptr(1));
    KisTagChooserWidget widget(model, QStringLiteral("brushes"), &parent);

    QCOMPARE(widget.parentWidget(), &parent);
    QCOMPARE(capturedModel, model);
    QCOMPARE(capturedResourceType, QStringLiteral("brushes"));
    QCOMPARE(toolConnectionCount, 1);
    QVERIFY(allTagsModel);
    QCOMPARE(allTagsModel->parent(), &widget);
    QVERIFY(tagToolButton);
    QCOMPARE(tagToolButton->parentWidget(), &widget);
    QVERIFY(!tagToolButton->toolTip().isEmpty());

    auto *comboBox = widget.findChild<QComboBox *>();
    auto *layout = qobject_cast<QGridLayout *>(widget.layout());
    QVERIFY(comboBox);
    QVERIFY(layout);
    QCOMPARE(comboBox->model(), &presentationModel);
    QCOMPARE(comboBox->insertPolicy(), QComboBox::InsertAlphabetically);
    QCOMPARE(comboBox->minimumContentsLength(), 1);
    QCOMPARE(comboBox->sizeAdjustPolicy(),
             QComboBox::AdjustToMinimumContentsLengthWithIcon);
    QCOMPARE(comboBox->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    QCOMPARE(comboBox->sizePolicy().verticalPolicy(), QSizePolicy::Fixed);
    QCOMPARE(layout->spacing(), 0);
    QCOMPARE(layout->contentsMargins(), QMargins());
    QCOMPARE(layout->columnStretch(0), 3);
    QVERIFY(widget.isEnabled());
}

void KisTagChooserWidgetContractTest::destructorReleasesOwnedPresentation()
{
    configurePresentationModel();
    allTagsModel = nullptr;
    tagToolButton = nullptr;
    QPointer<KisTagChooserWidget> widget = new KisTagChooserWidget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("patterns"),
        nullptr);
    QPointer<QComboBox> comboBox = widget->findChild<QComboBox *>();

    QVERIFY(comboBox);
    QVERIFY(allTagsModel);
    QVERIFY(tagToolButton);
    delete widget.data();

    QVERIFY(widget.isNull());
    QVERIFY(comboBox.isNull());
    QVERIFY(allTagsModel.isNull());
    QVERIFY(tagToolButton.isNull());
}

void KisTagChooserWidgetContractTest::selectionFindsExactUrlAndReturnsTag()
{
    configurePresentationModel();
    KisTagChooserWidget widget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("brushes"),
        nullptr);

    widget.setCurrentItem(QStringLiteral("beta"));
    QCOMPARE(widget.currentIndex(), 1);
    QVERIFY(widget.currentlySelectedTag() == markerTag(0x20));

    widget.setCurrentItem(QStringLiteral("missing"));
    QCOMPARE(widget.currentIndex(), 1);

    auto *comboBox = widget.findChild<QComboBox *>();
    QVERIFY(comboBox);
    {
        QSignalBlocker blocker(comboBox);
        comboBox->setCurrentIndex(-1);
    }
    QVERIFY(widget.currentlySelectedTag().isNull());
}

void KisTagChooserWidgetContractTest::tagChangePersistsSortsAndEmitsSelection()
{
    configurePresentationModel();
    KisTagChooserWidget widget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("patterns"),
        nullptr);
    auto *comboBox = widget.findChild<QComboBox *>();
    QVERIFY(comboBox);
    {
        QSignalBlocker blocker(comboBox);
        comboBox->setCurrentIndex(1);
    }
    presentationModel.sortCount = 0;
    currentToolTag.clear();
    persistedResourceType.clear();
    persistedTag.clear();
    QSignalSpy chosenSpy(&widget, &KisTagChooserWidget::sigTagChosen);

    widget.tagChanged(1);

    QCOMPARE(chosenSpy.count(), 1);
    QVERIFY(qvariant_cast<KisTagSP>(chosenSpy.takeFirst().at(0))
            == markerTag(0x20));
    QCOMPARE(presentationModel.sortCount, 1);
    QCOMPARE(presentationModel.lastSortColumn,
             int(KisAllTagsModel::Name));
    QCOMPARE(presentationModel.lastSortOrder, Qt::AscendingOrder);
    QVERIFY(currentToolTag == markerTag(0x20));
    QCOMPARE(persistedResourceType, QStringLiteral("patterns"));
    QVERIFY(persistedTag == markerTag(0x20));

    widget.tagChanged(-1);
    QCOMPARE(widget.currentIndex(), 0);
}

void KisTagChooserWidgetContractTest::iconUpdateRefreshesTagToolButton()
{
    configurePresentationModel();
    KisTagChooserWidget widget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("patterns"),
        nullptr);

    iconRefreshCount = 0;
    widget.updateIcons();
    QCOMPARE(iconRefreshCount, 1);
}

void KisTagChooserWidgetContractTest::nameAddRejectsInvalidAndAddsNewValues()
{
    configurePresentationModel();
    KisTagChooserWidget widget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("brushes"),
        nullptr);

    resetAddState();
    widget.addTag(KisAllTagsModel::urlAll());
    QCOMPARE(addReservedWarningCount, 1);
    QCOMPARE(capturedReservedParent.data(), &widget);
    QVERIFY(!capturedReservedTagValueInput);
    QCOMPARE(addLookupCount, 0);
    QCOMPARE(addNameCount, 0);

    resetAddState();
    widget.addTag(QString());
    QCOMPARE(addReservedWarningCount, 0);
    QCOMPARE(addLookupCount, 0);
    QCOMPARE(addNameCount, 0);

    resetAddState();
    widget.addTag(QStringLiteral("fresh"));
    QCOMPARE(addLookupCount, 1);
    QCOMPARE(capturedAddLookupUrl, QStringLiteral("fresh"));
    QCOMPARE(addNameCount, 1);
    QCOMPARE(capturedAddName, QStringLiteral("fresh"));
    QVERIFY(capturedAddResource.isNull());

    resetAddState();
    const KoResourceSP resource = markerResource(0x30);
    widget.addTag(QStringLiteral("with-resource"), resource);
    QCOMPARE(addNameCount, 1);
    QCOMPARE(capturedAddLookupUrl, QStringLiteral("with-resource"));
    QCOMPARE(capturedAddName, QStringLiteral("with-resource"));
    QVERIFY(capturedAddResource == resource);
}

void KisTagChooserWidgetContractTest::existingNameChoiceRestoresCancelsOrReplaces()
{
    configurePresentationModel();
    KisTagChooserWidget widget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("patterns"),
        nullptr);
    const KisTagSP existing = markerTag(0x40);
    const KoResourceSP resource = markerResource(0x50);

    resetAddState();
    addExistingTag = existing;
    addNextChoice = QMessageBox::AcceptRole;
    widget.addTag(QStringLiteral("existing"), resource);
    QCOMPARE(addQuestionCount, 1);
    QCOMPARE(capturedQuestionParent.data(), &widget);
    QVERIFY(!capturedQuestionTagActive);
    QCOMPARE(addReactivateCount, 1);
    QCOMPARE(addNameCount, 0);
    QCOMPARE(capturedAddResourceType, QStringLiteral("patterns"));
    QVERIFY(capturedAddTag == existing);
    QVERIFY(capturedAddResource == resource);

    resetAddState();
    addExistingTag = existing;
    addNextChoice = QMessageBox::RejectRole;
    widget.addTag(QStringLiteral("existing"), resource);
    QCOMPARE(addQuestionCount, 1);
    QCOMPARE(addReactivateCount, 0);
    QCOMPARE(addNameCount, 0);

    resetAddState();
    addExistingTag = existing;
    addNextChoice = QMessageBox::DestructiveRole;
    widget.addTag(QStringLiteral("existing"), resource);
    QCOMPARE(addQuestionCount, 1);
    QCOMPARE(addReactivateCount, 0);
    QCOMPARE(addNameCount, 1);
    QVERIFY(capturedAddResource == resource);
}

void KisTagChooserWidgetContractTest::tagValueUsesNameUrlAndExistingChoice()
{
    configurePresentationModel();
    KisTagChooserWidget widget(
        reinterpret_cast<KisTagModel *>(quintptr(1)),
        QStringLiteral("patterns"),
        nullptr);
    const KisTagSP tag = markerTag(0x60);
    const KoResourceSP resource = markerResource(0x70);

    resetAddState();
    addTagName = KisAllTagsModel::urlAllUntagged();
    widget.addTag(tag, resource);
    QCOMPARE(addReservedWarningCount, 1);
    QCOMPARE(capturedReservedParent.data(), &widget);
    QVERIFY(capturedReservedTagValueInput);
    QCOMPARE(addLookupCount, 0);
    QCOMPARE(addValueCount, 0);

    resetAddState();
    widget.addTag(tag, resource);
    QCOMPARE(addLookupCount, 1);
    QCOMPARE(capturedAddLookupUrl, QStringLiteral("fresh"));
    QCOMPARE(addValueCount, 1);
    QVERIFY(capturedAddTag == tag);
    QVERIFY(capturedAddResource == resource);

    resetAddState();
    addExistingTag = markerTag(0x80);
    addExistingTagActive = true;
    addNextChoice = QMessageBox::AcceptRole;
    widget.addTag(tag, resource);
    QCOMPARE(addQuestionCount, 1);
    QCOMPARE(capturedQuestionParent.data(), &widget);
    QVERIFY(capturedQuestionTagActive);
    QCOMPARE(addReactivateCount, 1);
    QVERIFY(capturedAddTag == addExistingTag);
    QVERIFY(capturedAddResource == resource);
}

QTEST_MAIN(KisTagChooserWidgetContractTest)

#include "KisTagChooserWidgetContractTest.moc"
