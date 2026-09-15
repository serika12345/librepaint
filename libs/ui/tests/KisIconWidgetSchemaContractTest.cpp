/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/kis_iconwidget.h>

#include <QTest>

#include <type_traits>

class KisIconWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void iconWidgetSchemaRemainStable();
};

void KisIconWidgetSchemaContractTest::iconWidgetSchemaRemainStable()
{
    using Widget = KisIconWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisPopupButton, Widget>);
    static_assert(std::is_constructible_v<Widget, QWidget *, const QString &>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::setThumbnail), void (Widget::*)(const QImage &)>);
    static_assert(std::is_same_v<decltype(&Widget::setResource), void (Widget::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Widget::setBackgroundColor), void (Widget::*)(const QColor &)>);
    static_assert(std::is_same_v<decltype(&Widget::preferredIconSize), QSize (Widget::*)() const>);
}

QTEST_APPLESS_MAIN(KisIconWidgetSchemaContractTest)

#include "KisIconWidgetSchemaContractTest.moc"
