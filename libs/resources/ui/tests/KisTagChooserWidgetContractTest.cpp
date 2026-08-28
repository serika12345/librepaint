/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisTagChooserWidget.h>
#include <KisTagChooserWidgetConstructionSource_p.h>

#include <QComboBox>
#include <QGridLayout>
#include <QPointer>
#include <QStandardItemModel>
#include <QTest>

namespace
{
QStandardItemModel presentationModel;
KisTagModel *capturedModel = nullptr;
QString capturedResourceType;
QPointer<QAbstractItemModel> allTagsModel;
QPointer<QWidget> tagToolButton;
int toolConnectionCount = 0;
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

void KisTagChooserWidget::tagChanged(int)
{
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
};

void KisTagChooserWidgetContractTest::constructorOwnsPresentationUnderParent()
{
    presentationModel.clear();
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
    presentationModel.clear();
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

QTEST_MAIN(KisTagChooserWidgetContractTest)

#include "KisTagChooserWidgetContractTest.moc"
