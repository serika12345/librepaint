/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourcePopupAction.h>
#include <KoResourceServerProvider.h>

#include <QTest>

#include <type_traits>

class KoResourceSelectionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceServerProviderTypeAndLifetimeSchemaRemainStable();
    void resourceServerProviderAccessSignaturesRemainStable();
    void resourcePopupActionTypeLifetimeAndControlSchemaRemainStable();
    void resourcePopupActionBackgroundSignaturesRemainStable();
    void resourcePopupActionResourceSignaturesRemainStable();
};

void KoResourceSelectionSchemaContractTest::resourceServerProviderTypeAndLifetimeSchemaRemainStable()
{
    using Provider = KoResourceServerProvider;
    using InstanceSignature = Provider *(*)();

    static_assert(std::is_class_v<Provider>);
    static_assert(std::is_base_of_v<QObject, Provider>);
    static_assert(std::is_default_constructible_v<Provider>);
    static_assert(std::has_virtual_destructor_v<Provider>);
    static_assert(std::is_same_v<decltype(static_cast<InstanceSignature>(&Provider::instance)), InstanceSignature>);
}

void KoResourceSelectionSchemaContractTest::resourceServerProviderAccessSignaturesRemainStable()
{
    using Provider = KoResourceServerProvider;
    using GamutMaskServerSignature = KoResourceServer<KoGamutMask> *(*)();
    using GradientServerSignature = KoResourceServer<KoAbstractGradient> *(*)();
    using PaletteServerSignature = KoResourceServer<KoColorSet> *(*)();
    using PatternServerSignature = KoResourceServer<KoPattern> *(*)();
    using StylePresetServerSignature = KoResourceServer<KoCssStylePreset> *(*)();
    using SvgSymbolCollectionServerSignature = KoResourceServer<KoSvgSymbolCollectionResource> *(*)();

    static_assert(std::is_same_v<decltype(static_cast<GamutMaskServerSignature>(&Provider::gamutMaskServer)),
                                 GamutMaskServerSignature>);
    static_assert(std::is_same_v<decltype(static_cast<GradientServerSignature>(&Provider::gradientServer)),
                                 GradientServerSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PaletteServerSignature>(&Provider::paletteServer)),
                                 PaletteServerSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PatternServerSignature>(&Provider::patternServer)),
                                 PatternServerSignature>);
#if defined HAVE_SEEXPR
    using SeExprScriptServerSignature = KoResourceServer<KisSeExprScript> *(*)();
    static_assert(std::is_same_v<decltype(static_cast<SeExprScriptServerSignature>(&Provider::seExprScriptServer)),
                                 SeExprScriptServerSignature>);
#endif
    static_assert(std::is_same_v<decltype(static_cast<StylePresetServerSignature>(&Provider::stylePresetServer)),
                                 StylePresetServerSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SvgSymbolCollectionServerSignature>(&Provider::svgSymbolCollectionServer)),
                       SvgSymbolCollectionServerSignature>);
}

void KoResourceSelectionSchemaContractTest::resourcePopupActionTypeLifetimeAndControlSchemaRemainStable()
{
    using Action = KoResourcePopupAction;
    using SetCanvasResourcesInterfaceSignature = void (Action::*)(KoCanvasResourcesInterfaceSP);
    using UpdateIconSignature = void (Action::*)();

    static_assert(std::is_class_v<Action>);
    static_assert(std::is_base_of_v<QAction, Action>);
    static_assert(std::is_constructible_v<Action, const QString &, KoCanvasResourcesInterfaceSP>);
    static_assert(std::is_constructible_v<Action, const QString &, KoCanvasResourcesInterfaceSP, QObject *>);
    static_assert(std::has_virtual_destructor_v<Action>);
    static_assert(std::is_same_v<decltype(static_cast<SetCanvasResourcesInterfaceSignature>(
                                     &Action::setCanvasResourcesInterface)),
                                 SetCanvasResourcesInterfaceSignature>);
    static_assert(std::is_same_v<decltype(static_cast<UpdateIconSignature>(&Action::updateIcon)), UpdateIconSignature>);
}

void KoResourceSelectionSchemaContractTest::resourcePopupActionBackgroundSignaturesRemainStable()
{
    using Action = KoResourcePopupAction;
    using Background = QSharedPointer<KoShapeBackground>;
    using CurrentBackgroundSignature = Background (Action::*)() const;
    using ResourceSelectedSignature = void (Action::*)(Background);
    using SetCurrentBackgroundSignature = void (Action::*)(Background);

    static_assert(std::is_same_v<decltype(static_cast<CurrentBackgroundSignature>(&Action::currentBackground)),
                                 CurrentBackgroundSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ResourceSelectedSignature>(&Action::resourceSelected)),
                                 ResourceSelectedSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetCurrentBackgroundSignature>(&Action::setCurrentBackground)),
                                 SetCurrentBackgroundSignature>);
}

void KoResourceSelectionSchemaContractTest::resourcePopupActionResourceSignaturesRemainStable()
{
    using Action = KoResourcePopupAction;
    using CurrentResourceSignature = KoResourceSP (Action::*)() const;
    using SetCurrentResourceSignature = void (Action::*)(KoResourceSP);

    static_assert(std::is_same_v<decltype(static_cast<CurrentResourceSignature>(&Action::currentResource)),
                                 CurrentResourceSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetCurrentResourceSignature>(&Action::setCurrentResource)),
                                 SetCurrentResourceSignature>);
}

QTEST_APPLESS_MAIN(KoResourceSelectionSchemaContractTest)

#include "KoResourceSelectionSchemaContractTest.moc"
