/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisOptionCollectionWidget.h"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QTest>

namespace
{
[[noreturn]] void unexpectedAssert(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

QBoxLayout *boxLayout(QWidget *widget)
{
    return qobject_cast<QBoxLayout *>(widget->layout());
}

QBoxLayout *wrapperLayout(QWidget *child)
{
    return boxLayout(child->parentWidget());
}

QBoxLayout *wrapperContentLayout(QWidget *child)
{
    return qobject_cast<QBoxLayout *>(wrapperLayout(child)->itemAt(0)->layout());
}
} // namespace

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    unexpectedAssert(assertion, file, line);
}

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    unexpectedAssert(assertion, file, line);
}

void kis_assert_x_exception(const char *assertion, const char *, const char *, const char *file, int line)
{
    unexpectedAssert(assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    unexpectedAssert(assertion, file, line);
}

class KisOptionCollectionWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyStateAndParentLifetime();
    void collectionPreservesOrderIdsAndTypedPaths();
    void collectionTakeAndRemoveLeaveWrapperResidue();
    void collectionAppliesVisibilitySeparatorsMarginsAndOrientation();
    void headerManagesPrimaryWidgetOwnershipAndSizing();
    void headerDelegatesCollectionOperations();
};

void KisOptionCollectionWidgetContractTest::emptyStateAndParentLifetime()
{
    auto *parent = new QWidget;
    QPointer<KisOptionCollectionWidget> collection = new KisOptionCollectionWidget(parent);
    QPointer<KisOptionCollectionWidgetWithHeader> header =
        new KisOptionCollectionWidgetWithHeader(QStringLiteral("Options"), parent);

    QCOMPARE(collection->parentWidget(), parent);
    QCOMPARE(collection->size(), 0);
    QCOMPARE(collection->numberOfVisibleWidgets(), 0);
    QCOMPARE(header->parentWidget(), parent);
    QCOMPARE(header->size(), 0);
    QCOMPARE(header->numberOfVisibleWidgets(), 0);
    QVERIFY(!header->primaryWidget());

    delete parent;
    QVERIFY(collection.isNull());
    QVERIFY(header.isNull());
}

void KisOptionCollectionWidgetContractTest::collectionPreservesOrderIdsAndTypedPaths()
{
    KisOptionCollectionWidget collection;
    auto *first = new QLabel(QStringLiteral("first"));
    auto *second = new QLabel(QStringLiteral("second"));
    auto *nested = new KisOptionCollectionWidget;
    auto *leaf = new QLabel(QStringLiteral("leaf"));
    nested->appendWidget(QStringLiteral("leaf"), leaf);

    collection.appendWidget(QStringLiteral("second"), second);
    collection.insertWidget(0, QStringLiteral("first"), first);
    collection.appendWidget(QStringLiteral("nested"), nested);

    QCOMPARE(collection.size(), 3);
    QVERIFY(collection.containsWidget(QStringLiteral("first")));
    QVERIFY(collection.containsWidget(QStringLiteral("second")));
    QCOMPARE(collection.widgetIndexFromId(QStringLiteral("first")), 0);
    QCOMPARE(collection.widgetIndexFromId(QStringLiteral("second")), 1);
    QCOMPARE(collection.widget(0), first);
    QCOMPARE(collection.widget(QStringLiteral("second")), second);
    QCOMPARE(collection.widgetAs<QLabel *>(0), first);
    QCOMPARE(collection.widgetAs<QLabel *>(QStringLiteral("second")), second);
    QCOMPARE(collection.findWidget(QStringLiteral("/nested/leaf")), leaf);
    QCOMPARE(collection.findWidgetAs<QLabel *>(QStringLiteral("nested/leaf")), leaf);
    QCOMPARE(first->parentWidget()->parentWidget(), &collection);
    QCOMPARE(nested->parentWidget()->parentWidget(), &collection);
}

void KisOptionCollectionWidgetContractTest::collectionTakeAndRemoveLeaveWrapperResidue()
{
    KisOptionCollectionWidget collection;
    QPointer<QLabel> first = new QLabel(QStringLiteral("first"));
    QPointer<QLabel> second = new QLabel(QStringLiteral("second"));
    QPointer<QLabel> third = new QLabel(QStringLiteral("third"));
    QPointer<QLabel> fourth = new QLabel(QStringLiteral("fourth"));
    collection.appendWidget(QStringLiteral("first"), first);
    collection.appendWidget(QStringLiteral("second"), second);
    collection.appendWidget(QStringLiteral("third"), third);
    collection.appendWidget(QStringLiteral("fourth"), fourth);

    QCOMPARE(collection.numberOfVisibleWidgets(), 4);

    QWidget *takenByIndex = collection.takeWidget(0);
    QCOMPARE(takenByIndex, first.data());
    QVERIFY(!takenByIndex->parent());
    QCOMPARE(collection.size(), 3);
    QCOMPARE(collection.numberOfVisibleWidgets(), 4);

    QWidget *takenById = collection.takeWidget(QStringLiteral("second"));
    QCOMPARE(takenById, second.data());
    QVERIFY(!takenById->parent());
    QCOMPARE(collection.size(), 2);
    QCOMPARE(collection.numberOfVisibleWidgets(), 4);

    collection.removeWidget(0);
    QVERIFY(third.isNull());
    QCOMPARE(collection.size(), 1);
    QCOMPARE(collection.numberOfVisibleWidgets(), 4);

    collection.removeWidget(QStringLiteral("fourth"));
    QVERIFY(fourth.isNull());
    QCOMPARE(collection.size(), 0);
    QCOMPARE(collection.numberOfVisibleWidgets(), 4);

    delete takenByIndex;
    delete takenById;
}

void KisOptionCollectionWidgetContractTest::collectionAppliesVisibilitySeparatorsMarginsAndOrientation()
{
    KisOptionCollectionWidget collection;
    auto *first = new QLabel(QStringLiteral("first"));
    auto *second = new QLabel(QStringLiteral("second"));
    auto *nested = new KisOptionCollectionWidget;
    nested->appendWidget(QStringLiteral("leaf"), new QLabel(QStringLiteral("leaf")));
    collection.appendWidget(QStringLiteral("first"), first);
    collection.appendWidget(QStringLiteral("second"), second);
    collection.appendWidget(QStringLiteral("nested"), nested);

    collection.setWidgetVisible(0, false);
    QVERIFY(first->parentWidget()->isHidden());
    QCOMPARE(collection.numberOfVisibleWidgets(), 2);
    collection.setWidgetVisible(QStringLiteral("first"), true);
    QVERIFY(!first->parentWidget()->isHidden());
    QCOMPARE(collection.numberOfVisibleWidgets(), 3);

    collection.setWidgetsMargin(7);
    const QMargins margins = wrapperContentLayout(first)->contentsMargins();
    QCOMPARE(margins.left(), 7);
    QCOMPARE(margins.right(), 7);

    collection.setSeparatorsVisible(true);
    QCOMPARE(boxLayout(&collection)->spacing(), 0);
    QCOMPARE(wrapperLayout(first)->count(), 2);
    QCOMPARE(wrapperLayout(nested)->count(), 1);
    collection.setSeparatorsVisible(false);
    QCOMPARE(boxLayout(&collection)->spacing(), 5);
    QCOMPARE(wrapperLayout(first)->count(), 1);

    collection.setOrientation(Qt::Horizontal, true);
    QCOMPARE(boxLayout(&collection)->direction(), QBoxLayout::LeftToRight);
    QCOMPARE(wrapperLayout(first)->direction(), QBoxLayout::LeftToRight);
    QCOMPARE(boxLayout(nested)->direction(), QBoxLayout::LeftToRight);
}

void KisOptionCollectionWidgetContractTest::headerManagesPrimaryWidgetOwnershipAndSizing()
{
    KisOptionCollectionWidgetWithHeader header(QStringLiteral("Brush options"));
    QPointer<QLabel> first = new QLabel(QStringLiteral("primary"));
    first->setMinimumSize(80, 24);
    header.setPrimaryWidget(first);

    QCOMPARE(header.primaryWidget(), first.data());
    QCOMPARE(header.primaryWidgetAs<QLabel *>(), first.data());
    QCOMPARE(first->parentWidget(), &header);
    QVERIFY(header.minimumSizeHint().width() >= first->minimumSizeHint().width());

    header.setPrimaryWidgetVisible(false);
    QVERIFY(first->isHidden());
    header.setPrimaryWidgetVisible(true);
    QVERIFY(!first->isHidden());

    QPointer<QLabel> replacement = new QLabel(QStringLiteral("replacement"));
    header.setPrimaryWidget(replacement);
    QVERIFY(first.isNull());
    QCOMPARE(header.primaryWidget(), replacement.data());

    QWidget *taken = header.takePrimaryWidget();
    QCOMPARE(taken, replacement.data());
    QVERIFY(!taken->parent());
    QVERIFY(!header.primaryWidget());
    delete taken;

    QPointer<QLabel> removed = new QLabel(QStringLiteral("removed"));
    header.setPrimaryWidget(removed);
    header.removePrimaryWidget();
    QVERIFY(removed.isNull());
    QVERIFY(!header.primaryWidget());
}

void KisOptionCollectionWidgetContractTest::headerDelegatesCollectionOperations()
{
    KisOptionCollectionWidgetWithHeader header(QStringLiteral("Options"));
    QPointer<QLabel> first = new QLabel(QStringLiteral("first"));
    QPointer<QLabel> second = new QLabel(QStringLiteral("second"));
    QPointer<QLabel> third = new QLabel(QStringLiteral("third"));
    QPointer<QLabel> fourth = new QLabel(QStringLiteral("fourth"));
    auto *nested = new KisOptionCollectionWidget;
    auto *leaf = new QLabel(QStringLiteral("leaf"));
    nested->appendWidget(QStringLiteral("leaf"), leaf);

    header.appendWidget(QStringLiteral("second"), second);
    header.insertWidget(0, QStringLiteral("first"), first);
    header.appendWidget(QStringLiteral("third"), third);
    header.appendWidget(QStringLiteral("fourth"), fourth);
    header.appendWidget(QStringLiteral("nested"), nested);

    QCOMPARE(header.size(), 5);
    QCOMPARE(header.numberOfVisibleWidgets(), 5);
    QVERIFY(header.containsWidget(QStringLiteral("first")));
    QCOMPARE(header.widgetIndexFromId(QStringLiteral("first")), 0);
    QCOMPARE(header.widget(0), first.data());
    QCOMPARE(header.widget(QStringLiteral("second")), second.data());
    QCOMPARE(header.widgetAs<QLabel *>(0), first.data());
    QCOMPARE(header.widgetAs<QLabel *>(QStringLiteral("second")), second.data());
    QCOMPARE(header.findWidget(QStringLiteral("nested/leaf")), leaf);
    QCOMPARE(header.findWidgetAs<QLabel *>(QStringLiteral("/nested/leaf")), leaf);

    header.setWidgetVisible(0, false);
    QCOMPARE(header.numberOfVisibleWidgets(), 4);
    header.setWidgetVisible(QStringLiteral("first"), true);
    QCOMPARE(header.numberOfVisibleWidgets(), 5);
    header.setWidgetsMargin(4);
    QCOMPARE(wrapperContentLayout(first)->contentsMargins().left(), 4);
    header.setSeparatorsVisible(true);
    QCOMPARE(wrapperLayout(first)->count(), 2);
    header.setOrientation(Qt::Horizontal, false);
    QCOMPARE(boxLayout(&header)->direction(), QBoxLayout::LeftToRight);
    QCOMPARE(wrapperLayout(first)->direction(), QBoxLayout::LeftToRight);

    header.removeWidget(0);
    QVERIFY(first.isNull());
    header.removeWidget(QStringLiteral("second"));
    QVERIFY(second.isNull());

    QWidget *takenByIndex = header.takeWidget(0);
    QCOMPARE(takenByIndex, third.data());
    QVERIFY(!takenByIndex->parent());
    QWidget *takenById = header.takeWidget(QStringLiteral("fourth"));
    QCOMPARE(takenById, fourth.data());
    QVERIFY(!takenById->parent());

    QCOMPARE(header.size(), 1);
    QCOMPARE(header.numberOfVisibleWidgets(), 5);

    delete takenByIndex;
    delete takenById;
}

QTEST_MAIN(KisOptionCollectionWidgetContractTest)

#include "KisOptionCollectionWidgetContractTest.moc"
