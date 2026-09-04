/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisAutoLevelsWidget.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAutoLevelsWidget::method)), signature>)
} // namespace

class KisAutoLevelsWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void autoLevelsWidgetTypeAndLifetimeSchemaRemainStable();
    void autoLevelsWidgetContrastAdjustmentMethodSignaturesRemainStable();
    void autoLevelsWidgetClippingAndOffsetSignaturesRemainStable();
    void autoLevelsWidgetMidtonesAdjustmentSignaturesRemainStable();
    void autoLevelsWidgetOutputColorAndNotificationSignaturesRemainStable();
};

void KisAutoLevelsWidgetSchemaContractTest::autoLevelsWidgetTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAutoLevelsWidget>);
    static_assert(std::is_base_of_v<QWidget, KisAutoLevelsWidget>);
    static_assert(std::is_constructible_v<KisAutoLevelsWidget, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisAutoLevelsWidget>);
}

void KisAutoLevelsWidgetSchemaContractTest::autoLevelsWidgetContrastAdjustmentMethodSignaturesRemainStable()
{
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(lockShadowsAndHighlightsAdjustmentMethod, void (KisAutoLevelsWidget::*)());
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(
        setShadowsAndHighlightsAdjustmentMethod,
        void (KisAutoLevelsWidget::*)(KisAutoLevels::ShadowsAndHighlightsAdjustmentMethod));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(shadowsAndHighlightsAdjustmentMethod,
                                        KisAutoLevels::ShadowsAndHighlightsAdjustmentMethod (KisAutoLevelsWidget::*)()
                                            const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(unlockShadowsAndHighlightsAdjustmentMethod, void (KisAutoLevelsWidget::*)());
}

void KisAutoLevelsWidgetSchemaContractTest::autoLevelsWidgetClippingAndOffsetSignaturesRemainStable()
{
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(highlightsClipping, qreal (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(maximumInputBlackAndWhiteOffset, qreal (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setHighlightsClipping, void (KisAutoLevelsWidget::*)(qreal));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setMaximumInputBlackAndWhiteOffset, void (KisAutoLevelsWidget::*)(qreal));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setShadowsClipping, void (KisAutoLevelsWidget::*)(qreal));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(shadowsClipping, qreal (KisAutoLevelsWidget::*)() const);
}

void KisAutoLevelsWidgetSchemaContractTest::autoLevelsWidgetMidtonesAdjustmentSignaturesRemainStable()
{
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(midtonesAdjustmentAmount, qreal (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(midtonesAdjustmentMethod,
                                        KisAutoLevels::MidtonesAdjustmentMethod (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setMidtonesAdjustmentAmount, void (KisAutoLevelsWidget::*)(qreal));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setMidtonesAdjustmentMethod,
                                        void (KisAutoLevelsWidget::*)(KisAutoLevels::MidtonesAdjustmentMethod));
}

void KisAutoLevelsWidgetSchemaContractTest::autoLevelsWidgetOutputColorAndNotificationSignaturesRemainStable()
{
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(outputHighlightsColor, KoColor (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(outputMidtonesColor, KoColor (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(outputShadowsColor, KoColor (KisAutoLevelsWidget::*)() const);
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(parametersChanged, void (KisAutoLevelsWidget::*)());
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setHighlightsColor, void (KisAutoLevelsWidget::*)(const KoColor &));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setMidtonesColor, void (KisAutoLevelsWidget::*)(const KoColor &));
    ASSERT_AUTO_LEVELS_WIDGET_SIGNATURE(setShadowsColor, void (KisAutoLevelsWidget::*)(const KoColor &));
}

QTEST_MAIN(KisAutoLevelsWidgetSchemaContractTest)

#include "KisAutoLevelsWidgetSchemaContractTest.moc"
