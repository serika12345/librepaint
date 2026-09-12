/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/kis_progress_widget.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PROGRESS_WIDGET_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisProgressWidget::method)), signature>)
} // namespace

class KisProgressWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void progressWidgetTypeConstructionAndProxySchemaRemainStable();
    void progressWidgetUpdaterAttachmentSchemaRemainStable();
    void progressWidgetCancellationAndVisibilitySchemaRemainStable();
};

void KisProgressWidgetSchemaContractTest::progressWidgetTypeConstructionAndProxySchemaRemainStable()
{
    using Widget = KisProgressWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_base_of_v<KisProgressInterface, Widget>);
    static_assert(std::is_constructible_v<Widget, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    ASSERT_PROGRESS_WIDGET_SIGNATURE(progressProxy, KoProgressProxy * (Widget::*)());

    QVERIFY(true);
}

void KisProgressWidgetSchemaContractTest::progressWidgetUpdaterAttachmentSchemaRemainStable()
{
    using Widget = KisProgressWidget;

    ASSERT_PROGRESS_WIDGET_SIGNATURE(attachUpdater, void (Widget::*)(KoProgressUpdater *));
    ASSERT_PROGRESS_WIDGET_SIGNATURE(detachUpdater, void (Widget::*)(KoProgressUpdater *));

    QVERIFY(true);
}

void KisProgressWidgetSchemaContractTest::progressWidgetCancellationAndVisibilitySchemaRemainStable()
{
    using Widget = KisProgressWidget;

    ASSERT_PROGRESS_WIDGET_SIGNATURE(cancel, void (Widget::*)());
    ASSERT_PROGRESS_WIDGET_SIGNATURE(correctVisibility, void (Widget::*)(int));
    ASSERT_PROGRESS_WIDGET_SIGNATURE(sigCancellationRequested, void (Widget::*)());

    QVERIFY(true);
}

#undef ASSERT_PROGRESS_WIDGET_SIGNATURE

QTEST_APPLESS_MAIN(KisProgressWidgetSchemaContractTest)

#include "KisProgressWidgetSchemaContractTest.moc"
