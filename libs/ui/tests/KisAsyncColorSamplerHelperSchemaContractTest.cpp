/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tool/KisAsyncColorSamplerHelper.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAsyncColorSamplerHelper::method)), signature>)
} // namespace

class KisAsyncColorSamplerHelperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void asyncColorSamplerTypeLifetimeAndStateSchemaRemainStable();
    void asyncColorSamplerActivationAndActionSignaturesRemainStable();
    void asyncColorSamplerPreviewAndPaintingSignaturesRemainStable();
    void asyncColorSamplerGlobalColorAndOutlineSignaturesRemainStable();
    void asyncColorSamplerColorNotificationSignaturesRemainStable();
};

void KisAsyncColorSamplerHelperSchemaContractTest::asyncColorSamplerTypeLifetimeAndStateSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAsyncColorSamplerHelper>);
    static_assert(std::is_constructible_v<KisAsyncColorSamplerHelper, KisCanvas2 *>);
    static_assert(std::has_virtual_destructor_v<KisAsyncColorSamplerHelper>);
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(isActive, bool (KisAsyncColorSamplerHelper::*)() const);
}

void KisAsyncColorSamplerHelperSchemaContractTest::asyncColorSamplerActivationAndActionSignaturesRemainStable()
{
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(activate, void (KisAsyncColorSamplerHelper::*)(bool, bool));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(continueAction, void (KisAsyncColorSamplerHelper::*)(const QPointF &));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(deactivate, void (KisAsyncColorSamplerHelper::*)());
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(endAction, void (KisAsyncColorSamplerHelper::*)());
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(startAction, void (KisAsyncColorSamplerHelper::*)(const QPointF &, int, int));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(updateCursor, void (KisAsyncColorSamplerHelper::*)(bool, bool));
}

void KisAsyncColorSamplerHelperSchemaContractTest::asyncColorSamplerPreviewAndPaintingSignaturesRemainStable()
{
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(colorPreviewDocRect, QRectF (KisAsyncColorSamplerHelper::*)(const QPointF &));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(paint,
                                         void (KisAsyncColorSamplerHelper::*)(QPainter &, const KoViewConverter &));
}

void KisAsyncColorSamplerHelperSchemaContractTest::asyncColorSamplerGlobalColorAndOutlineSignaturesRemainStable()
{
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(setUpdateGlobalColor, void (KisAsyncColorSamplerHelper::*)(bool));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(sigRequestCursor, void (KisAsyncColorSamplerHelper::*)(const QCursor &));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(sigRequestCursorReset, void (KisAsyncColorSamplerHelper::*)());
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(sigRequestUpdateOutline, void (KisAsyncColorSamplerHelper::*)());
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(updateGlobalColor, bool (KisAsyncColorSamplerHelper::*)() const);
}

void KisAsyncColorSamplerHelperSchemaContractTest::asyncColorSamplerColorNotificationSignaturesRemainStable()
{
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(sigColorSelected, void (KisAsyncColorSamplerHelper::*)(const KoColor &));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(sigFinalColorSelected, void (KisAsyncColorSamplerHelper::*)(const KoColor &));
    ASSERT_ASYNC_COLOR_SAMPLER_SIGNATURE(sigRawColorSelected, void (KisAsyncColorSamplerHelper::*)(const KoColor &));
}

QTEST_MAIN(KisAsyncColorSamplerHelperSchemaContractTest)

#include "KisAsyncColorSamplerHelperSchemaContractTest.moc"
