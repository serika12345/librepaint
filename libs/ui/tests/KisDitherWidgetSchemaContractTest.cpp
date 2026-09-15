/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisDitherWidget.h"

#include <QTest>

#include <type_traits>

class KisDitherWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ditherWidgetPublicSchemaRemainsStable();
};

void KisDitherWidgetSchemaContractTest::ditherWidgetPublicSchemaRemainsStable()
{
    using Widget = KisDitherWidget;

    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, QWidget *>);
    static_assert(std::is_same_v<decltype(&Widget::setConfiguration),
                                 void (Widget::*)(const KisFilterConfiguration &, const QString &)>);
    static_assert(std::is_same_v<decltype(&Widget::configuration),
                                 void (Widget::*)(KisPropertiesConfiguration &, const QString &) const>);
    static_assert(std::is_same_v<decltype(&Widget::factoryConfiguration),
                                 void (*)(KisPropertiesConfiguration &, const QString &)>);
    static_assert(
        std::is_same_v<
            decltype(&Widget::prepareLinkedResources),
            QList<KoResourceLoadResult> (*)(const KisFilterConfiguration &, const QString &, KisResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&Widget::sigConfigurationItemChanged), void (Widget::*)()>);

    QVERIFY(true);
}

QTEST_MAIN(KisDitherWidgetSchemaContractTest)

#include "KisDitherWidgetSchemaContractTest.moc"
