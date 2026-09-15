/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "operations/kis_operation_ui_widget_factory.h"

#include <QTest>

#include <type_traits>

namespace
{
class OperationWidgetProbe final : public KisOperationUIWidget
{
public:
    using KisOperationUIWidget::KisOperationUIWidget;

    void getConfiguration(KisOperationConfigurationSP) override;
};
} // namespace

class KisOperationUIWidgetFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void operationUiWidgetFactorySchemaRemainStable();
};

void KisOperationUIWidgetFactorySchemaContractTest::operationUiWidgetFactorySchemaRemainStable()
{
    using Factory = KisOperationUIWidgetFactory<OperationWidgetProbe>;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KisOperationUIFactory, Factory>);
    static_assert(std::is_constructible_v<Factory, const QString &>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::fetchConfiguration),
                                 bool (Factory::*)(KisViewManager *, KisOperationConfigurationSP)>);
}

QTEST_GUILESS_MAIN(KisOperationUIWidgetFactorySchemaContractTest)

#include "KisOperationUIWidgetFactorySchemaContractTest.moc"
