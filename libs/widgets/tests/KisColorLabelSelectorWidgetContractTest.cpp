/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_color_label_selector_widget.h"

#include "KisWrappableHBoxLayout.h"
#include "kis_color_label_button.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QPointer>
#include <QShowEvent>
#include <QSignalSpy>
#include <QStyle>
#include <QStyleOption>
#include <QTest>

namespace
{
QList<KisColorLabelButton *> colorButtons(KisColorLabelSelectorWidget &selector)
{
    return selector.findChildren<KisColorLabelButton *>(QString(), Qt::FindDirectChildrenOnly);
}

QList<QVariant> signalArguments(const QSignalSpy &spy, int index)
{
    return spy.at(index);
}

QList<KisColorLabelMouseDragFilter *> dragFilters(KisColorLabelSelectorWidget &selector)
{
    QList<KisColorLabelMouseDragFilter *> result;
    for (QObject *child : selector.children()) {
        if (auto *filter = dynamic_cast<KisColorLabelMouseDragFilter *>(child)) {
            result.append(filter);
        }
    }
    return result;
}
} // namespace

class KisColorLabelSelectorWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateEnumsAndParentLifetime();
    void buttonCheckingReportsExclusiveAndMultipleSelection();
    void currentIndexSetterDuplicatesTheChangeNotification();
    void setSelectionReplacesMultipleSelection();
    void configurationSwitchesLayoutDragAndPresentation();
    void menuWrapperOwnsItsSelector();
    void menuOffsetUsesInvertedIconDetection();
    void showEventAppliesTheCalculatedOffset();
};

void KisColorLabelSelectorWidgetContractTest::defaultStateEnumsAndParentLifetime()
{
    QCOMPARE(static_cast<int>(KisColorLabelSelectorWidget::FillIn), 0);
    QCOMPARE(static_cast<int>(KisColorLabelSelectorWidget::Outline), 1);

    auto *parent = new QWidget;
    QPointer<KisColorLabelSelectorWidget> selector = new KisColorLabelSelectorWidget(parent);

    QCOMPARE(selector->parentWidget(), parent);
    QCOMPARE(colorButtons(*selector).size(), 9);
    QCOMPARE(selector->layout()->count(), 9);
    QCOMPARE(selector->layout()->contentsMargins(), QMargins());
    QCOMPARE(selector->layout()->spacing(), 2);
    QCOMPARE(selector->currentIndex(), -1);
    QVERIFY(selector->selection().isEmpty());
    QVERIFY(selector->exclusive());
    QVERIFY(!selector->buttonWrapEnabled());
    QVERIFY(!selector->mouseDragEnabled());
    QCOMPARE(selector->selectionIndicationType(), KisColorLabelSelectorWidget::Outline);
    QCOMPARE(selector->buttonSize(), 22);
    for (int index = 0; index < 9; ++index) {
        QVERIFY(!selector->isButtonChecked(index));
    }

    delete parent;
    QVERIFY(selector.isNull());
}

void KisColorLabelSelectorWidgetContractTest::buttonCheckingReportsExclusiveAndMultipleSelection()
{
    KisColorLabelSelectorWidget selector;
    QSignalSpy toggledSpy(&selector, &KisColorLabelSelectorWidget::buttonToggled);
    QSignalSpy selectionSpy(&selector, &KisColorLabelSelectorWidget::selectionChanged);

    QVERIFY(selector.exclusive());
    QCOMPARE(selector.currentIndex(), -1);
    QVERIFY(selector.selection().isEmpty());
    QVERIFY(!selector.isButtonChecked(3));
    selector.setButtonChecked(3, true);
    QCOMPARE(selector.currentIndex(), 3);
    QCOMPARE(selector.selection(), QList<int>({3}));
    QVERIFY(selector.isButtonChecked(3));
    QCOMPARE(toggledSpy.count(), 1);
    QCOMPARE(signalArguments(toggledSpy, 0), QList<QVariant>({3, true}));
    QCOMPARE(selectionSpy.count(), 0);

    selector.setExclusive(false);
    QVERIFY(!selector.exclusive());
    QCOMPARE(selector.currentIndex(), -2);
    selector.setButtonChecked(6, true);
    QCOMPARE(selector.selection(), QList<int>({3, 6}));
    QVERIFY(selector.isButtonChecked(6));
    QCOMPARE(toggledSpy.count(), 2);
    QCOMPARE(signalArguments(toggledSpy, 1), QList<QVariant>({6, true}));
    QCOMPARE(selectionSpy.count(), 1);
}

void KisColorLabelSelectorWidgetContractTest::currentIndexSetterDuplicatesTheChangeNotification()
{
    KisColorLabelSelectorWidget selector;
    QSignalSpy currentIndexSpy(&selector, &KisColorLabelSelectorWidget::currentIndexChanged);

    selector.setCurrentIndex(4);
    QCOMPARE(selector.currentIndex(), 4);
    QCOMPARE(currentIndexSpy.count(), 2);
    QCOMPARE(signalArguments(currentIndexSpy, 0), QVariantList({QVariant(4)}));
    QCOMPARE(signalArguments(currentIndexSpy, 1), QVariantList({QVariant(4)}));

    selector.setCurrentIndex(4);
    QCOMPARE(currentIndexSpy.count(), 3);
    QCOMPARE(signalArguments(currentIndexSpy, 2), QVariantList({QVariant(4)}));
}

void KisColorLabelSelectorWidgetContractTest::setSelectionReplacesMultipleSelection()
{
    KisColorLabelSelectorWidget selector;
    selector.setExclusive(false);
    QSignalSpy toggledSpy(&selector, &KisColorLabelSelectorWidget::buttonToggled);
    QSignalSpy selectionSpy(&selector, &KisColorLabelSelectorWidget::selectionChanged);

    selector.setSelection(QList<int>({2, 6}));
    QCOMPARE(selector.selection(), QList<int>({2, 6}));
    QVERIFY(selector.isButtonChecked(2));
    QVERIFY(selector.isButtonChecked(6));
    QCOMPARE(toggledSpy.count(), 0);
    QCOMPARE(selectionSpy.count(), 1);

    selector.setSelection(QList<int>({4}));
    QCOMPARE(selector.selection(), QList<int>({4}));
    QCOMPARE(toggledSpy.count(), 0);
    QCOMPARE(selectionSpy.count(), 2);
}

void KisColorLabelSelectorWidgetContractTest::configurationSwitchesLayoutDragAndPresentation()
{
    KisColorLabelSelectorWidget selector;

    QLayout *initialLayout = selector.layout();
    selector.setButtonWrapEnabled(true);
    QVERIFY(selector.buttonWrapEnabled());
    QVERIFY(dynamic_cast<KisWrappableHBoxLayout *>(selector.layout()));
    QCOMPARE(selector.layout()->count(), 9);
    QCOMPARE(selector.layout()->contentsMargins(), QMargins());
    QCOMPARE(selector.layout()->spacing(), 2);
    QVERIFY(selector.layout() != initialLayout);

    QLayout *wrappingLayout = selector.layout();
    selector.setButtonWrapEnabled(true);
    QCOMPARE(selector.layout(), wrappingLayout);
    selector.setButtonWrapEnabled(false);
    QVERIFY(!selector.buttonWrapEnabled());
    QVERIFY(dynamic_cast<QHBoxLayout *>(selector.layout()));
    QCOMPARE(selector.layout()->count(), 9);

    selector.setMouseDragEnabled(true);
    QVERIFY(selector.mouseDragEnabled());
    QCOMPARE(dragFilters(selector).size(), 1);
    QPointer<KisColorLabelMouseDragFilter> dragFilter = dragFilters(selector).first();
    QVERIFY(dragFilter);
    selector.setMouseDragEnabled(true);
    QCOMPARE(dragFilters(selector).size(), 1);
    selector.setMouseDragEnabled(false);
    QVERIFY(!selector.mouseDragEnabled());
    QVERIFY(dragFilter.isNull());

    selector.setSelectionIndicationType(KisColorLabelSelectorWidget::FillIn);
    QCOMPARE(selector.selectionIndicationType(), KisColorLabelSelectorWidget::FillIn);
    selector.setButtonSize(30);
    QCOMPARE(selector.buttonSize(), 30);
    for (KisColorLabelButton *button : colorButtons(selector)) {
        QCOMPARE(button->selectionVisType(), KisColorLabelButton::FillIn);
        QCOMPARE(button->sizeHint(), QSize(30, 30));
    }
}

void KisColorLabelSelectorWidgetContractTest::menuWrapperOwnsItsSelector()
{
    auto *parent = new QWidget;
    QPointer<KisColorLabelSelectorWidgetMenuWrapper> wrapper = new KisColorLabelSelectorWidgetMenuWrapper(parent);
    QPointer<KisColorLabelSelectorWidget> selector = wrapper->colorLabelSelector();

    QCOMPARE(wrapper->parentWidget(), parent);
    QCOMPARE(selector->parentWidget(), wrapper.data());
    QCOMPARE(wrapper->layout()->count(), 2);

    delete parent;
    QVERIFY(wrapper.isNull());
    QVERIFY(selector.isNull());
}

void KisColorLabelSelectorWidgetContractTest::menuOffsetUsesInvertedIconDetection()
{
    KisColorLabelSelectorWidgetMenuWrapper standalone;
    QCOMPARE(standalone.calculateMenuOffset(), 0);

    QMenu iconMenu;
    QPixmap iconPixmap(4, 4);
    iconPixmap.fill(Qt::red);
    iconMenu.addAction(QIcon(iconPixmap), QStringLiteral("with icon"));
    KisColorLabelSelectorWidgetMenuWrapper iconWrapper(&iconMenu);
    QCOMPARE(iconWrapper.calculateMenuOffset(), 0);

    QMenu iconlessMenu;
    iconlessMenu.addAction(QStringLiteral("without icon"));
    KisColorLabelSelectorWidgetMenuWrapper iconlessWrapper(&iconlessMenu);
    QStyleOption option;
    option.initFrom(&iconlessWrapper);
    const int expectedOffset = iconlessWrapper.style()->pixelMetric(QStyle::PM_MenuHMargin, &option, &iconlessWrapper)
        + iconlessWrapper.style()->pixelMetric(QStyle::PM_SmallIconSize, &option, &iconlessWrapper) + 6;
    QVERIFY(expectedOffset > 0);
    QCOMPARE(iconlessWrapper.calculateMenuOffset(), expectedOffset);
}

void KisColorLabelSelectorWidgetContractTest::showEventAppliesTheCalculatedOffset()
{
    QMenu menu;
    QAction *action = menu.addAction(QStringLiteral("checkable"));
    action->setCheckable(true);
    KisColorLabelSelectorWidgetMenuWrapper wrapper(&menu);
    wrapper.resize(200, 40);
    QCOMPARE(wrapper.layout()->itemAt(0)->spacerItem()->sizeHint(), QSize(0, 0));

    QShowEvent event;
    QCoreApplication::sendEvent(&wrapper, &event);

    QCOMPARE(wrapper.layout()->itemAt(0)->spacerItem()->sizeHint(), QSize(wrapper.calculateMenuOffset(), 40));
}

QTEST_MAIN(KisColorLabelSelectorWidgetContractTest)
#include "KisColorLabelSelectorWidgetContractTest.moc"
