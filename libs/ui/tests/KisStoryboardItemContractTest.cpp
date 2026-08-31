/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <QDomDocument>
#include <QGuiApplication>
#include <QTest>
#include <document/StoryboardItem.h>

class KisStoryboardItemContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void valueTypesAndChildKindsPreserveStableValues();
    void childPreservesDataAndWeakParentLifetime();
    void itemMutationsPreserveOrderAndParentIdentity();
    void cloningPreservesValuesWithIndependentChildren();
    void xmlRoundTripPreservesStoryboardFieldsAndComments();
};

void KisStoryboardItemContractTest::valueTypesAndChildKindsPreserveStableValues()
{
    QCOMPARE(int(StoryboardItem::FrameNumber), 0);
    QCOMPARE(int(StoryboardItem::ItemName), 1);
    QCOMPARE(int(StoryboardItem::DurationSecond), 2);
    QCOMPARE(int(StoryboardItem::DurationFrame), 3);
    QCOMPARE(int(StoryboardItem::Comments), 4);
    StoryboardComment comment;
    QVERIFY(comment.name.isEmpty());
    QVERIFY(comment.visibility);
    comment.name = QStringLiteral("注釈");
    comment.visibility = false;
    QCOMPARE(comment.name, QStringLiteral("注釈"));
    QVERIFY(!comment.visibility);
    CommentBox box;
    QCOMPARE(box.content, QVariant(QString()));
    QCOMPARE(box.scrollValue, QVariant(0));
    box.content = QStringLiteral("本文");
    box.scrollValue = 37;
    const CommentBox copiedBox(box);
    QCOMPARE(copiedBox.content, QVariant(QStringLiteral("本文")));
    QCOMPARE(copiedBox.scrollValue, QVariant(37));
    ThumbnailData thumbnail;
    QCOMPARE(thumbnail.frameNum, QVariant(QString()));
    QVERIFY(thumbnail.pixmap.value<QPixmap>().isNull());
    thumbnail.frameNum = 91;
    const ThumbnailData copiedThumbnail(thumbnail);
    QCOMPARE(copiedThumbnail.frameNum, QVariant(91));
    QVERIFY(copiedThumbnail.pixmap.value<QPixmap>().isNull());
}

void KisStoryboardItemContractTest::childPreservesDataAndWeakParentLifetime()
{
    StoryboardChild child(QStringLiteral("初期"));
    QCOMPARE(child.data(), QVariant(QStringLiteral("初期")));
    child.setData(42);
    StoryboardChild copy(child);
    QCOMPARE(copy.data(), QVariant(42));
    child.setData(73);
    QCOMPARE(copy.data(), QVariant(42));
    StoryboardItemSP parent = QSharedPointer<StoryboardItem>::create();
    child.setParent(parent);
    QCOMPARE(child.parent(), parent);
    parent.reset();
    QVERIFY(child.parent().isNull());
}

void KisStoryboardItemContractTest::itemMutationsPreserveOrderAndParentIdentity()
{
    StoryboardItemSP item = QSharedPointer<StoryboardItem>::create();
    QCOMPARE(item->childCount(), 0);
    QVERIFY(item->child(-1).isNull());
    item->appendChild(QStringLiteral("A"));
    item->appendChild(QStringLiteral("C"));
    item->insertChild(1, QStringLiteral("B"));
    QCOMPARE(item->childCount(), 3);
    QCOMPARE(item->child(1)->data(), QVariant(QStringLiteral("B")));
    QCOMPARE(item->child(1)->parent(), item);
    item->moveChild(2, 0);
    QCOMPARE(item->child(0)->data(), QVariant(QStringLiteral("C")));
    item->removeChild(1);
    QCOMPARE(item->child(1)->data(), QVariant(QStringLiteral("B")));
    QVERIFY(item->child(2).isNull());
}

void KisStoryboardItemContractTest::cloningPreservesValuesWithIndependentChildren()
{
    StoryboardItemSP original = QSharedPointer<StoryboardItem>::create();
    original->appendChild(QStringLiteral("第一"));
    original->appendChild(17);
    StoryboardItemSP copied = QSharedPointer<StoryboardItem>::create(*original);
    QCOMPARE(copied->childCount(), 2);
    QVERIFY(copied->child(0) != original->child(0));
    QVERIFY(copied->child(0)->parent().isNull());
    original->child(0)->setData(QStringLiteral("変更"));
    QCOMPARE(copied->child(0)->data(), QVariant(QStringLiteral("第一")));
    StoryboardItemSP assigned = QSharedPointer<StoryboardItem>::create();
    assigned->cloneChildrenFrom(*original);
    QCOMPARE(assigned->child(0)->parent(), assigned);
    const StoryboardItemList cloned = StoryboardItem::cloneStoryboardItemList({original, copied});
    QCOMPARE(cloned.size(), 2);
    QVERIFY(cloned.at(0) != original);
    QCOMPARE(cloned.at(0)->child(0)->data(), QVariant(QStringLiteral("変更")));
    QCOMPARE(cloned.at(1)->child(0)->parent(), cloned.at(1));
}

void KisStoryboardItemContractTest::xmlRoundTripPreservesStoryboardFieldsAndComments()
{
    StoryboardItemSP source = QSharedPointer<StoryboardItem>::create();
    ThumbnailData thumbnail;
    thumbnail.frameNum = 24;
    source->appendChild(QVariant::fromValue(thumbnail));
    source->appendChild(QStringLiteral("場面一"));
    source->appendChild(3);
    source->appendChild(72);
    CommentBox first;
    first.content = QStringLiteral("最初の注釈");
    first.scrollValue = 11;
    source->appendChild(QVariant::fromValue(first));
    CommentBox second;
    second.content = QStringLiteral("二番目");
    second.scrollValue = 29;
    source->appendChild(QVariant::fromValue(second));
    QDomDocument document;
    const QDomElement element = source->toXML(document);
    QCOMPARE(element.attribute(QStringLiteral("frame")), QStringLiteral("24"));
    QCOMPARE(element.attribute(QStringLiteral("item-name")), QStringLiteral("場面一"));
    QCOMPARE(element.attribute(QStringLiteral("duration-second")), QStringLiteral("3"));
    QCOMPARE(element.attribute(QStringLiteral("duration-frame")), QStringLiteral("72"));
    StoryboardItemSP restored = QSharedPointer<StoryboardItem>::create();
    restored->loadXML(element);
    QCOMPARE(restored->childCount(), 6);
    QCOMPARE(qvariant_cast<ThumbnailData>(restored->child(0)->data()).frameNum, QVariant(24));
    QCOMPARE(restored->child(1)->data(), QVariant(QStringLiteral("場面一")));
    const CommentBox restoredFirst = qvariant_cast<CommentBox>(restored->child(4)->data());
    const CommentBox restoredSecond = qvariant_cast<CommentBox>(restored->child(5)->data());
    QCOMPARE(restoredFirst.content, QVariant(QStringLiteral("最初の注釈")));
    QCOMPARE(restoredFirst.scrollValue.toInt(), 11);
    QCOMPARE(restoredSecond.content, QVariant(QStringLiteral("二番目")));
    QCOMPARE(restoredSecond.scrollValue.toInt(), 29);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QGuiApplication application(argc, argv);
    KisStoryboardItemContractTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "KisStoryboardItemContractTest.moc"
