/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "canvas/KisIdleTasksManager.h"
#include "canvas/KisLayerThumbnailCache.h"
#include "canvas/kis_canvas_resource_provider.h"

#define ASSERT_PROVIDER_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(&KisCanvasResourceProvider::method), signature>)
#define ASSERT_IDLE_TASKS_MANAGER_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisIdleTasksManager::method)), signature>)
#define ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerThumbnailCache::method)), signature>)

class KisCanvasResourceProviderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeOwnershipAndResourceConnectionSchemaRemainStable();
    void colorTextHistoryAndCompositeSchemaRemainStable();
    void brushDynamicsSchemaRemainStable();
    void mirrorAndWorkspaceSchemaRemainStable();
    void activationAndNotificationSchemaRemainStable();
    void idleTasksManagerTypeAndLifetimeSchemaRemainStable();
    void idleTaskGuardTypeConstructionAndLifetimeSchemaRemainStable();
    void idleTaskGuardCopyAndMoveSchemaRemainStable();
    void idleTaskGuardStateAndControlSchemaRemainStable();
    void idleTasksManagerImageAndRegistrationSignaturesRemainStable();
    void layerThumbnailCacheTypeAndLifetimeSchemaRemainStable();
    void layerThumbnailCacheImageAndSizeSignaturesRemainStable();
    void layerThumbnailCacheNodeSignaturesRemainStable();
    void layerThumbnailCacheUpdateSignaturesRemainStable();
};

void KisCanvasResourceProviderSchemaContractTest::typeOwnershipAndResourceConnectionSchemaRemainStable()
{
    using Provider = KisCanvasResourceProvider;
    using Initialize = void (*)(KoCanvasResourceProvider *);
    using RegisterLoader = void (*)(KisResourceLoaderRegistry &);

    static_assert(std::is_class_v<Provider>);
    static_assert(std::is_base_of_v<QObject, Provider>);
    static_assert(std::is_constructible_v<Provider, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<Provider>);
    ASSERT_PROVIDER_SIGNATURE(canvas, KoCanvasBase * (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentGamutMask, KoGamutMaskSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentGradient, KoAbstractGradientSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentImage, KisImageWSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentNode, KisNodeSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentPattern, KoPatternSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentPreset, KisPaintOpPresetSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(initializeOpacityToPresetResourceConverter, Initialize);
    ASSERT_PROVIDER_SIGNATURE(initializeResourceManager, Initialize);
    ASSERT_PROVIDER_SIGNATURE(previousPreset, KisPaintOpPresetSP (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(registerBrushResourceCacheFixup, RegisterLoader);
    ASSERT_PROVIDER_SIGNATURE(registerLayerStyleResourceLoader, RegisterLoader);
    ASSERT_PROVIDER_SIGNATURE(registerPaintOpAndBrushResourceLoaders, RegisterLoader);
    ASSERT_PROVIDER_SIGNATURE(resourceManager, KoCanvasResourceProvider * (Provider::*)());
    ASSERT_PROVIDER_SIGNATURE(setPaintOpPreset, void (Provider::*)(KisPaintOpPresetSP));
    ASSERT_PROVIDER_SIGNATURE(setPreviousPaintOpPreset, void (Provider::*)(KisPaintOpPresetSP));
    ASSERT_PROVIDER_SIGNATURE(setResourceManager, void (Provider::*)(KoCanvasResourceProvider *));

    QVERIFY(true);
}

void KisCanvasResourceProviderSchemaContractTest::colorTextHistoryAndCompositeSchemaRemainStable()
{
    using Provider = KisCanvasResourceProvider;
    using ColorSignal = void (Provider::*)(const KoColor &);
    using NoArgument = void (Provider::*)();
    using TextProperty = KoSvgTextPropertyData (Provider::*)() const;
    using SetTextProperty = void (Provider::*)(KoSvgTextPropertyData);

    ASSERT_PROVIDER_SIGNATURE(bgColor, KoColor (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(characterTextPropertyData, TextProperty);
    ASSERT_PROVIDER_SIGNATURE(colorHistoryColors, QList<KoColor> (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(colorHistoryModel, KisUniqueColorSet * (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(currentCompositeOp, QString (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(fgColor, KoColor (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(setBGColor, void (Provider::*)(const KoColor &));
    ASSERT_PROVIDER_SIGNATURE(setCharacterPropertyData, SetTextProperty);
    ASSERT_PROVIDER_SIGNATURE(setColorHistoryColors, void (Provider::*)(const QList<KoColor> &));
    ASSERT_PROVIDER_SIGNATURE(setCurrentCompositeOp, void (Provider::*)(const QString &));
    ASSERT_PROVIDER_SIGNATURE(setFGColor, void (Provider::*)(const KoColor &));
    ASSERT_PROVIDER_SIGNATURE(setTextPropertyData, SetTextProperty);
    ASSERT_PROVIDER_SIGNATURE(sigBGColorChanged, ColorSignal);
    ASSERT_PROVIDER_SIGNATURE(sigCharacterPropertiesChanged, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(sigEffectiveCompositeOpChanged, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(sigFGColorChanged, ColorSignal);
    ASSERT_PROVIDER_SIGNATURE(sigFGColorUsed, ColorSignal);
    ASSERT_PROVIDER_SIGNATURE(sigTextPropertiesChanged, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(textPropertyData, TextProperty);

    QVERIFY(true);
}

void KisCanvasResourceProviderSchemaContractTest::brushDynamicsSchemaRemainStable()
{
    using Provider = KisCanvasResourceProvider;
    using Boolean = bool (Provider::*)() const;
    using Decimal = qreal (Provider::*)() const;
    using SetBoolean = void (Provider::*)(bool);
    using SetDecimal = void (Provider::*)(qreal);

    ASSERT_PROVIDER_SIGNATURE(HDRExposure, float (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(HDRGamma, float (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(brushRotation, Decimal);
    ASSERT_PROVIDER_SIGNATURE(disablePressure, Boolean);
    ASSERT_PROVIDER_SIGNATURE(eraserMode, Boolean);
    ASSERT_PROVIDER_SIGNATURE(fade, Decimal);
    ASSERT_PROVIDER_SIGNATURE(flow, Decimal);
    ASSERT_PROVIDER_SIGNATURE(gamutMaskActive, Boolean);
    ASSERT_PROVIDER_SIGNATURE(globalAlphaLock, Boolean);
    ASSERT_PROVIDER_SIGNATURE(opacity, Decimal);
    ASSERT_PROVIDER_SIGNATURE(patternSize, Decimal);
    ASSERT_PROVIDER_SIGNATURE(setBrushRotation, SetDecimal);
    ASSERT_PROVIDER_SIGNATURE(setDisablePressure, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setEraserMode, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setFade, SetDecimal);
    ASSERT_PROVIDER_SIGNATURE(setFlow, SetDecimal);
    ASSERT_PROVIDER_SIGNATURE(setGlobalAlphaLock, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setHDRExposure, void (Provider::*)(float));
    ASSERT_PROVIDER_SIGNATURE(setHDRGamma, void (Provider::*)(float));
    ASSERT_PROVIDER_SIGNATURE(setOpacity, SetDecimal);
    ASSERT_PROVIDER_SIGNATURE(setPatternSize, SetDecimal);
    ASSERT_PROVIDER_SIGNATURE(setSize, SetDecimal);
    ASSERT_PROVIDER_SIGNATURE(size, Decimal);

    QVERIFY(true);
}

void KisCanvasResourceProviderSchemaContractTest::mirrorAndWorkspaceSchemaRemainStable()
{
    using Provider = KisCanvasResourceProvider;
    using NoArgument = void (Provider::*)();
    using SetBoolean = void (Provider::*)(bool);
    using Workspace = void (Provider::*)(KisWorkspaceResourceSP);

    ASSERT_PROVIDER_SIGNATURE(mirrorHorizontal, bool (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(mirrorHorizontalHideDecorations, bool (Provider::*)());
    ASSERT_PROVIDER_SIGNATURE(mirrorHorizontalLock, bool (Provider::*)());
    ASSERT_PROVIDER_SIGNATURE(mirrorHorizontalMoveCanvasToCenter, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(mirrorModeChanged, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(mirrorVertical, bool (Provider::*)() const);
    ASSERT_PROVIDER_SIGNATURE(mirrorVerticalHideDecorations, bool (Provider::*)());
    ASSERT_PROVIDER_SIGNATURE(mirrorVerticalLock, bool (Provider::*)());
    ASSERT_PROVIDER_SIGNATURE(mirrorVerticalMoveCanvasToCenter, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(moveMirrorHorizontalCenter, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(moveMirrorVerticalCenter, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(notifyLoadingWorkspace, Workspace);
    ASSERT_PROVIDER_SIGNATURE(notifySavingWorkspace, Workspace);
    ASSERT_PROVIDER_SIGNATURE(setMirrorHorizontal, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setMirrorHorizontalHideDecorations, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setMirrorHorizontalLock, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setMirrorVertical, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setMirrorVerticalHideDecorations, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(setMirrorVerticalLock, SetBoolean);
    ASSERT_PROVIDER_SIGNATURE(sigLoadingWorkspace, Workspace);
    ASSERT_PROVIDER_SIGNATURE(sigSavingWorkspace, Workspace);

    QVERIFY(true);
}

void KisCanvasResourceProviderSchemaContractTest::activationAndNotificationSchemaRemainStable()
{
    using Provider = KisCanvasResourceProvider;
    using NoArgument = void (Provider::*)();
    using ColorSlot = void (Provider::*)(const KoColor &);

    ASSERT_PROVIDER_SIGNATURE(sigGamutMaskChanged, void (Provider::*)(KoGamutMaskSP));
    ASSERT_PROVIDER_SIGNATURE(sigGamutMaskDeactivated, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(sigGamutMaskPreviewUpdate, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(sigGamutMaskUnset, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(sigGradientChanged, void (Provider::*)(KoAbstractGradientSP));
    ASSERT_PROVIDER_SIGNATURE(sigNodeChanged, void (Provider::*)(KisNodeSP));
    ASSERT_PROVIDER_SIGNATURE(sigOnScreenResolutionChanged, void (Provider::*)(qreal, qreal));
    ASSERT_PROVIDER_SIGNATURE(sigOpacityChanged, void (Provider::*)(qreal));
    ASSERT_PROVIDER_SIGNATURE(sigPaintOpPresetChanged, void (Provider::*)(KisPaintOpPresetSP));
    ASSERT_PROVIDER_SIGNATURE(sigPatternChanged, void (Provider::*)(KoPatternSP));
    ASSERT_PROVIDER_SIGNATURE(slotGamutMaskActivated, void (Provider::*)(KoGamutMaskSP));
    ASSERT_PROVIDER_SIGNATURE(slotGamutMaskDeactivate, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(slotGamutMaskPreviewUpdate, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(slotGamutMaskUnset, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(slotGradientActivated, void (Provider::*)(KoResourceSP));
    ASSERT_PROVIDER_SIGNATURE(slotImageSizeChanged, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(slotNodeActivated, void (Provider::*)(KisNodeSP));
    ASSERT_PROVIDER_SIGNATURE(slotOnScreenResolutionChanged, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(slotPainting, NoArgument);
    ASSERT_PROVIDER_SIGNATURE(slotPatternActivated, void (Provider::*)(KoResourceSP));
    ASSERT_PROVIDER_SIGNATURE(slotSetBGColor, ColorSlot);
    ASSERT_PROVIDER_SIGNATURE(slotSetFGColor, ColorSlot);

    QVERIFY(true);
}

void KisCanvasResourceProviderSchemaContractTest::idleTasksManagerTypeAndLifetimeSchemaRemainStable()
{
    using Manager = KisIdleTasksManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_default_constructible_v<Manager>);
    static_assert(std::has_virtual_destructor_v<Manager>);
}

void KisCanvasResourceProviderSchemaContractTest::idleTaskGuardTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Guard = KisIdleTasksManager::TaskGuard;

    static_assert(std::is_class_v<Guard>);
    static_assert(std::is_default_constructible_v<Guard>);
    static_assert(std::is_constructible_v<Guard, int, QPointer<KisIdleTasksManager>>);
    static_assert(std::is_destructible_v<Guard>);
}

void KisCanvasResourceProviderSchemaContractTest::idleTaskGuardCopyAndMoveSchemaRemainStable()
{
    using Guard = KisIdleTasksManager::TaskGuard;

    static_assert(!std::is_copy_constructible_v<Guard>);
    static_assert(!std::is_copy_assignable_v<Guard>);
    static_assert(std::is_move_constructible_v<Guard>);
    static_assert(std::is_move_assignable_v<Guard>);
}

void KisCanvasResourceProviderSchemaContractTest::idleTaskGuardStateAndControlSchemaRemainStable()
{
    using Guard = KisIdleTasksManager::TaskGuard;

    static_assert(std::is_same_v<decltype(&Guard::taskId), int Guard::*>);
    static_assert(std::is_same_v<decltype(&Guard::manager), QPointer<KisIdleTasksManager> Guard::*>);
    static_assert(std::is_same_v<decltype(&Guard::isValid), bool (Guard::*)() const>);
    static_assert(std::is_same_v<decltype(&Guard::trigger), void (Guard::*)()>);
}

void KisCanvasResourceProviderSchemaContractTest::idleTasksManagerImageAndRegistrationSignaturesRemainStable()
{
    using Manager = KisIdleTasksManager;

    ASSERT_IDLE_TASKS_MANAGER_SIGNATURE(setImage, void (Manager::*)(KisImageSP));
    ASSERT_IDLE_TASKS_MANAGER_SIGNATURE(addIdleTaskWithGuard,
                                        Manager::TaskGuard (Manager::*)(KisIdleTaskStrokeStrategyFactory));
}

void KisCanvasResourceProviderSchemaContractTest::layerThumbnailCacheTypeAndLifetimeSchemaRemainStable()
{
    using Cache = KisLayerThumbnailCache;

    static_assert(std::is_class_v<Cache>);
    static_assert(std::is_default_constructible_v<Cache>);
    static_assert(std::has_virtual_destructor_v<Cache>);
}

void KisCanvasResourceProviderSchemaContractTest::layerThumbnailCacheImageAndSizeSignaturesRemainStable()
{
    using Cache = KisLayerThumbnailCache;

    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(setImage, void (Cache::*)(KisImageSP));
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(setIdleTaskManager, void (Cache::*)(KisIdleTasksManager *));
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(setImage, void (Cache::*)(KisImageSP, KisIdleTasksManager *));
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(setMaxSize, void (Cache::*)(int));
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(maxSize, int (Cache::*)() const);
}

void KisCanvasResourceProviderSchemaContractTest::layerThumbnailCacheNodeSignaturesRemainStable()
{
    using Cache = KisLayerThumbnailCache;

    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(thumbnail, QImage (Cache::*)(KisNodeSP) const);
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(notifyNodeAdded, void (Cache::*)(KisNodeSP));
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(notifyNodeRemoved, void (Cache::*)(KisNodeSP));
}

void KisCanvasResourceProviderSchemaContractTest::layerThumbnailCacheUpdateSignaturesRemainStable()
{
    using Cache = KisLayerThumbnailCache;

    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(startThumbnailsUpdate, void (Cache::*)());
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(clear, void (Cache::*)());
    ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE(sigLayerThumbnailUpdated, void (Cache::*)(KisNodeSP));
}

#undef ASSERT_IDLE_TASKS_MANAGER_SIGNATURE
#undef ASSERT_LAYER_THUMBNAIL_CACHE_SIGNATURE

QTEST_GUILESS_MAIN(KisCanvasResourceProviderSchemaContractTest)

#include "KisCanvasResourceProviderSchemaContractTest.moc"
