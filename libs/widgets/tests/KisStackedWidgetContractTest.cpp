/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_stacked_widget.h>

#include <QPointer>
#include <QStackedWidget>
#include <QTest>
#include <QWidget>

#include <type_traits>

namespace
{

class HintWidget : public QWidget
{
public:
    HintWidget(const QSize &preferredHint, const QSize &minimumHint, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_preferredHint(preferredHint)
        , m_minimumHint(minimumHint)
    {
    }

    QSize sizeHint() const override
    {
        return m_preferredHint;
    }

    QSize minimumSizeHint() const override
    {
        return m_minimumHint;
    }

private:
    QSize m_preferredHint;
    QSize m_minimumHint;
};

} // namespace

class KisStackedWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void followsParentOwnershipAndDisablesValueTransfer();
    void emptyStackUsesBaseMinimumFallbacks();
    void minimumHintTracksOnlyTheCurrentPage();
    void preferredHintUsesTheCurrentPageMinimumHint();
};

void KisStackedWidgetContractTest::followsParentOwnershipAndDisablesValueTransfer()
{
    QVERIFY(!std::is_copy_constructible_v<KisStackedWidget>);
    QVERIFY(!std::is_copy_assignable_v<KisStackedWidget>);
    QVERIFY(!std::is_move_constructible_v<KisStackedWidget>);
    QVERIFY(!std::is_move_assignable_v<KisStackedWidget>);

    auto *parent = new QWidget;
    QPointer<KisStackedWidget> stack = new KisStackedWidget(parent);
    QPointer<QWidget> page = new QWidget;
    stack->addWidget(page);

    QCOMPARE(stack->parentWidget(), parent);
    QCOMPARE(page->parentWidget(), stack.data());

    delete parent;
    QVERIFY(stack.isNull());
    QVERIFY(page.isNull());

    QPointer<KisStackedWidget> polymorphicStack = new KisStackedWidget;
    QStackedWidget *base = polymorphicStack.data();
    delete base;
    QVERIFY(polymorphicStack.isNull());
}

void KisStackedWidgetContractTest::emptyStackUsesBaseMinimumFallbacks()
{
    KisStackedWidget stack;
    stack.setMinimumSize(QSize(37, 19));

    QVERIFY(!stack.currentWidget());
    QCOMPARE(stack.sizeHint(), stack.minimumSize());
    QCOMPARE(stack.minimumSizeHint(), stack.QStackedWidget::minimumSizeHint());
}

void KisStackedWidgetContractTest::minimumHintTracksOnlyTheCurrentPage()
{
    KisStackedWidget stack;
    auto *first = new HintWidget(QSize(120, 90), QSize(30, 20));
    auto *second = new HintWidget(QSize(220, 160), QSize(50, 40));
    stack.addWidget(first);
    stack.addWidget(second);

    QCOMPARE(stack.currentWidget(), first);
    QCOMPARE(stack.minimumSizeHint(), first->minimumSizeHint());

    stack.setCurrentWidget(second);
    QCOMPARE(stack.currentWidget(), second);
    QCOMPARE(stack.minimumSizeHint(), second->minimumSizeHint());
}

void KisStackedWidgetContractTest::preferredHintUsesTheCurrentPageMinimumHint()
{
    KisStackedWidget stack;
    auto *first = new HintWidget(QSize(120, 90), QSize(30, 20));
    auto *second = new HintWidget(QSize(220, 160), QSize(50, 40));
    stack.addWidget(first);
    stack.addWidget(second);

    QVERIFY(first->sizeHint() != first->minimumSizeHint());
    QCOMPARE(stack.sizeHint(), first->minimumSizeHint());
    QVERIFY(stack.sizeHint() != first->sizeHint());

    stack.setCurrentWidget(second);
    QVERIFY(second->sizeHint() != second->minimumSizeHint());
    QCOMPARE(stack.sizeHint(), second->minimumSizeHint());
    QVERIFY(stack.sizeHint() != second->sizeHint());
}

QTEST_MAIN(KisStackedWidgetContractTest)

#include "KisStackedWidgetContractTest.moc"
