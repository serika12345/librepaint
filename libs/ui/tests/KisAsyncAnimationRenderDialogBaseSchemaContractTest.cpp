/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/KisAsyncAnimationFramesSaveDialog.h"
#include "dialogs/KisAsyncAnimationRenderDialogBase.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
class AsyncAnimationRenderDialogProbe final : public KisAsyncAnimationRenderDialogBase
{
public:
    using KisAsyncAnimationRenderDialogBase::KisAsyncAnimationRenderDialogBase;

protected:
    QList<int> calcDirtyFrames() const override;
    KisAsyncAnimationRendererBase *createRenderer(KisImageSP image) override;
    void initializeRendererForFrame(KisAsyncAnimationRendererBase *renderer, KisImageSP image, int frame) override;
};

#define ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(method, ...)                                                     \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisAsyncAnimationFramesSaveDialog::method)), __VA_ARGS__>)
} // namespace

class KisAsyncAnimationRenderDialogBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeResultAndOrdinalsRemainStable();
    void constructionAndLifetimeSchemaRemainStable();
    void regenerationAndRegionSignaturesRemainStable();
    void batchModeSignaturesRemainStable();
    void framesSaveTypeConstructionAndLifetimeSchemaRemainStable();
    void framesSaveRegenerationAndMaskSignaturesRemainStable();
    void framesSaveFileCollectionSignaturesRemainStable();
};

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::typeResultAndOrdinalsRemainStable()
{
    using Dialog = KisAsyncAnimationRenderDialogBase;
    using Result = Dialog::Result;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_abstract_v<Dialog>);
    static_assert(std::is_base_of_v<QObject, Dialog>);
    static_assert(std::is_enum_v<Result>);
    static_assert(Result::RenderComplete == 0);
    static_assert(Result::RenderCancelled == 1);
    static_assert(Result::RenderFailed == 2);
    static_assert(Result::RenderTimedOut == 3);
}

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::constructionAndLifetimeSchemaRemainStable()
{
    using Dialog = KisAsyncAnimationRenderDialogBase;
    using Probe = AsyncAnimationRenderDialogProbe;

    static_assert(std::is_constructible_v<Probe, const QString &, KisImageSP, int>);
    static_assert(std::is_same_v<decltype(Probe(std::declval<const QString &>(), std::declval<KisImageSP>())), Probe>);
    static_assert(std::has_virtual_destructor_v<Dialog>);
}

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::regenerationAndRegionSignaturesRemainStable()
{
    using Dialog = KisAsyncAnimationRenderDialogBase;

    static_assert(std::is_same_v<decltype(&Dialog::regenerateRange), Dialog::Result (Dialog::*)(KisViewManager *)>);
    static_assert(std::is_same_v<decltype(&Dialog::setRegionOfInterest), void (Dialog::*)(const KisRegion &)>);
    static_assert(std::is_same_v<decltype(&Dialog::regionOfInterest), KisRegion (Dialog::*)() const>);
}

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::batchModeSignaturesRemainStable()
{
    using Dialog = KisAsyncAnimationRenderDialogBase;

    static_assert(std::is_same_v<decltype(&Dialog::setBatchMode), void (Dialog::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Dialog::batchMode), bool (Dialog::*)() const>);
}

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::framesSaveTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Dialog = KisAsyncAnimationFramesSaveDialog;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<KisAsyncAnimationRenderDialogBase, Dialog>);
    static_assert(
        std::is_constructible_v<Dialog, KisImageSP, const QString &, int, bool, KisPropertiesConfigurationSP>);
    static_assert(std::is_constructible_v<Dialog,
                                          KisImageSP,
                                          const KisTimeSpan &,
                                          const QString &,
                                          int,
                                          bool,
                                          KisPropertiesConfigurationSP>);
    static_assert(std::has_virtual_destructor_v<Dialog>);
}

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::framesSaveRegenerationAndMaskSignaturesRemainStable()
{
    using Dialog = KisAsyncAnimationFramesSaveDialog;

    ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(regenerateRange, Dialog::Result (Dialog::*)(KisViewManager *));
    ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(savedFilesMask, QString (Dialog::*)() const);
    ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(savedFilesMaskWildcard, QString (Dialog::*)() const);
}

void KisAsyncAnimationRenderDialogBaseSchemaContractTest::framesSaveFileCollectionSignaturesRemainStable()
{
    using Dialog = KisAsyncAnimationFramesSaveDialog;

    ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(savedFiles, QStringList (Dialog::*)() const);
    ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(savedUniqueFiles, QStringList (Dialog::*)() const);
    ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE(getUniqueFrames, QList<int> (Dialog::*)() const);
}

#undef ASSERT_ANIMATION_FRAMES_SAVE_DIALOG_SIGNATURE

QTEST_APPLESS_MAIN(KisAsyncAnimationRenderDialogBaseSchemaContractTest)

#include "KisAsyncAnimationRenderDialogBaseSchemaContractTest.moc"
