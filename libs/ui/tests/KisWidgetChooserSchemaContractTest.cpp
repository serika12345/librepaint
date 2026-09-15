/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/kis_widget_chooser.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisWidgetChooserSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void widgetChooserTypeAndSelectionSchemaRemainStable();
    void widgetChooserRegistrationAndPopupSchemaRemainStable();
};

void KisWidgetChooserSchemaContractTest::widgetChooserTypeAndSelectionSchemaRemainStable()
{
    using Widget = KisWidgetChooser;

    static_assert(std::is_base_of_v<QFrame, Widget>);
    static_assert(std::is_constructible_v<Widget, int, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::chooseWidget), QWidget *(Widget::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(static_cast<QWidget *(Widget::*)(const QString &) const>(&Widget::getWidget)),
                                 QWidget *(Widget::*)(const QString &) const>);
    static_assert(
        std::is_same_v<decltype(std::declval<Widget &>().template addWidget<QWidget>(std::declval<const QString &>(),
                                                                                     std::declval<const QString &>())),
                       QWidget *>);

    QVERIFY(true);
}

void KisWidgetChooserSchemaContractTest::widgetChooserRegistrationAndPopupSchemaRemainStable()
{
    using Widget = KisWidgetChooser;

    static_assert(std::is_same_v<decltype(&Widget::addLabelWidget),
                                 void (Widget::*)(const QString &, const QString &, QWidget *)>);
    static_assert(std::is_same_v<decltype(&Widget::showPopupWidget), void (Widget::*)()>);
    static_assert(std::is_same_v<decltype(&Widget::updateThemedIcons), void (Widget::*)()>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KisWidgetChooserSchemaContractTest)

#include "KisWidgetChooserSchemaContractTest.moc"
