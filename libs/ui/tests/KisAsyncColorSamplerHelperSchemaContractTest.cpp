/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tool/KisAsyncColorSamplerHelper.h"
#include "widgets/KisScreenColorSampler.h"

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
    void screenColorSamplerTypeConstructionAndFactorySchemaRemainStable();
    void screenColorSamplerColorStateSignaturesRemainStable();
    void screenColorSamplerInputAndCancellationSignaturesRemainStable();
    void screenColorSamplerActivationAndNotificationSignaturesRemainStable();
    void screenColorSamplingEventFilterSchemaRemainStable();
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

void KisAsyncColorSamplerHelperSchemaContractTest::screenColorSamplerTypeConstructionAndFactorySchemaRemainStable()
{
    using Sampler = KisScreenColorSampler;
    static_assert(std::is_class_v<Sampler> && std::is_base_of_v<KisScreenColorSamplerBase, Sampler>);
    static_assert(std::has_virtual_destructor_v<Sampler> && std::is_default_constructible_v<Sampler>);
    static_assert(std::is_constructible_v<Sampler, bool, QWidget *>);
    static_assert(std::is_same_v<decltype(&Sampler::createScreenColorSampler), Sampler *(*)(QWidget *)>);
}

void KisAsyncColorSamplerHelperSchemaContractTest::screenColorSamplerColorStateSignaturesRemainStable()
{
    using Sampler = KisScreenColorSampler;
    static_assert(std::is_same_v<decltype(&Sampler::currentColor), KoColor (Sampler::*)()>);
    static_assert(std::is_same_v<decltype(&Sampler::performRealColorSamplingOfCanvas), bool (Sampler::*)() const>);
    static_assert(std::is_same_v<decltype(&Sampler::setCurrentColor), void (Sampler::*)(KoColor)>);
    static_assert(std::is_same_v<decltype(&Sampler::setPerformRealColorSamplingOfCanvas), void (Sampler::*)(bool)>);
}

void KisAsyncColorSamplerHelperSchemaContractTest::screenColorSamplerInputAndCancellationSignaturesRemainStable()
{
    using Sampler = KisScreenColorSampler;
    static_assert(std::is_same_v<decltype(&Sampler::handleColorSamplingMouseMove), bool (Sampler::*)(QMouseEvent *)>);
    static_assert(
        std::is_same_v<decltype(&Sampler::handleColorSamplingMouseButtonRelease), bool (Sampler::*)(QMouseEvent *)>);
    static_assert(std::is_same_v<decltype(&Sampler::handleColorSamplingKeyPress), bool (Sampler::*)(QKeyEvent *)>);
    static_assert(std::is_same_v<decltype(&Sampler::cancel), void (Sampler::*)()>);
}

void KisAsyncColorSamplerHelperSchemaContractTest::screenColorSamplerActivationAndNotificationSignaturesRemainStable()
{
    using Sampler = KisScreenColorSampler;
    static_assert(std::is_same_v<decltype(&Sampler::sampleScreenColor), void (Sampler::*)()>);
    static_assert(std::is_same_v<decltype(&Sampler::updateIcons), void (Sampler::*)()>);
    static_assert(std::is_same_v<decltype(&Sampler::sigNewColorSampled), void (Sampler::*)(KoColor)>);
    static_assert(std::is_same_v<decltype(&Sampler::sigNewColorHovered), void (Sampler::*)(KoColor)>);
}

void KisAsyncColorSamplerHelperSchemaContractTest::screenColorSamplingEventFilterSchemaRemainStable()
{
    using Filter = KisScreenColorSamplingEventFilter;
    static_assert(std::is_class_v<Filter> && std::is_base_of_v<QObject, Filter>);
    static_assert(std::is_constructible_v<Filter, KisScreenColorSampler *, QObject *>);
    static_assert(std::is_same_v<decltype(&Filter::eventFilter), bool (Filter::*)(QObject *, QEvent *)>);
}

QTEST_MAIN(KisAsyncColorSamplerHelperSchemaContractTest)

#include "KisAsyncColorSamplerHelperSchemaContractTest.moc"
