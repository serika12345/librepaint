/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportResizeWidget.h"

#include <QTest>

#include <type_traits>

class KisImportExportResizeWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resizeWidgetPublicInterfaceRemainsStable();
};

void KisImportExportResizeWidgetSchemaContractTest::resizeWidgetPublicInterfaceRemainsStable()
{
    using ResizeWidget = KisImportExportResizeWidget;
    using Widget = QWidget *(ResizeWidget::*)() const;
    using Size = QSize (ResizeWidget::*)() const;
    using Resolution = double (ResizeWidget::*)() const;
    using FilterType = KisFilterStrategy *(ResizeWidget::*)() const;
    using Create = std::unique_ptr<ResizeWidget> (*)(QWidget *, int, int, double);

    static_assert(std::is_abstract_v<ResizeWidget>);
    static_assert(std::has_virtual_destructor_v<ResizeWidget>);
    static_assert(std::is_same_v<decltype(&ResizeWidget::widget), Widget>);
    static_assert(std::is_same_v<decltype(&ResizeWidget::desiredSize), Size>);
    static_assert(std::is_same_v<decltype(&ResizeWidget::desiredResolution), Resolution>);
    static_assert(std::is_same_v<decltype(&ResizeWidget::filterType), FilterType>);
    static_assert(std::is_same_v<decltype(&createImportExportResizeWidget), Create>);
}

QTEST_GUILESS_MAIN(KisImportExportResizeWidgetSchemaContractTest)

#include "KisImportExportResizeWidgetSchemaContractTest.moc"
