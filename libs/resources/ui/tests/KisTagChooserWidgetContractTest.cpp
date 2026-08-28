/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisTagChooserWidget.h>
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

KisTagSP markerTag(quintptr value)
{
    return KisTagSP(reinterpret_cast<KisTag *>(value), [](KisTag *) {});
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

void KisTagChooserWidget::addTag(const QString &)
{
}

void KisTagChooserWidget::addTag(const QString &, KoResourceSP)
{
}

void KisTagChooserWidget::addTag(KisTagSP, KoResourceSP)
{
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

QTEST_MAIN(KisTagChooserWidgetContractTest)

#include "KisTagChooserWidgetContractTest.moc"
