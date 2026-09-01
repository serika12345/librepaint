/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCanvasResourceProvider.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCanvasResourceProvider::method)), signature>)
} // namespace

class KoCanvasResourceProviderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasProviderTypeLifetimeAndPolicySchemaRemainsStable();
    void canvasResourceAccessSignaturesRemainStable();
    void canvasPresentationResourceSignaturesRemainStable();
    void canvasResourceExtensionPointSignaturesRemainStable();
    void canvasResourceInterfaceAndNotificationSignaturesRemainStable();
};

void KoCanvasResourceProviderSchemaContractTest::canvasProviderTypeLifetimeAndPolicySchemaRemainsStable()
{
    static_assert(std::is_class_v<KoCanvasResourceProvider>);
    static_assert(std::is_base_of_v<QObject, KoCanvasResourceProvider>);
    static_assert(std::is_default_constructible_v<KoCanvasResourceProvider>);
    static_assert(!std::is_copy_constructible_v<KoCanvasResourceProvider>);
    static_assert(!std::is_copy_assignable_v<KoCanvasResourceProvider>);
    static_assert(std::has_virtual_destructor_v<KoCanvasResourceProvider>);

    static_assert(std::is_enum_v<KoCanvasResourceProvider::ApplicationSpecial>);
    static_assert(KoCanvasResourceProvider::NoSpecial == 0);
    static_assert(KoCanvasResourceProvider::NoAdvancedText == 1);
}

void KoCanvasResourceProviderSchemaContractTest::canvasResourceAccessSignaturesRemainStable()
{
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setResource, void (KoCanvasResourceProvider::*)(int, const QVariant &));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setResource, void (KoCanvasResourceProvider::*)(int, const KoColor &));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setResource, void (KoCanvasResourceProvider::*)(int, KoShape *));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setResource, void (KoCanvasResourceProvider::*)(int, const KoUnit &));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(resource, QVariant (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(boolResource, bool (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(intResource, int (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(koColorResource, KoColor (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(koShapeResource, KoShape * (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(stringResource, QString (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(sizeResource, QSizeF (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(unitResource, KoUnit (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(hasResource, bool (KoCanvasResourceProvider::*)(int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(clearResource, void (KoCanvasResourceProvider::*)(int));
}

void KoCanvasResourceProviderSchemaContractTest::canvasPresentationResourceSignaturesRemainStable()
{
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setForegroundColor, void (KoCanvasResourceProvider::*)(const KoColor &));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(foregroundColor, KoColor (KoCanvasResourceProvider::*)() const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setBackgroundColor, void (KoCanvasResourceProvider::*)(const KoColor &));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(backgroundColor, KoColor (KoCanvasResourceProvider::*)() const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setHandleRadius, void (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(handleRadius, int (KoCanvasResourceProvider::*)() const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setDecorationThickness, void (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(decorationThickness, int (KoCanvasResourceProvider::*)() const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setUsingOtherColor, void (KoCanvasResourceProvider::*)(bool));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(isUsingOtherColor, bool (KoCanvasResourceProvider::*)() const);
}

void KoCanvasResourceProviderSchemaContractTest::canvasResourceExtensionPointSignaturesRemainStable()
{
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(addDerivedResourceConverter,
                                              void (KoCanvasResourceProvider::*)(KoDerivedResourceConverterSP));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(hasDerivedResourceConverter, bool (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(removeDerivedResourceConverter, void (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(setAbstractResource,
                                              void (KoCanvasResourceProvider::*)(KoAbstractCanvasResourceInterfaceSP));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(hasAbstractResource, bool (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(removeAbstractResource, void (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(addResourceUpdateMediator,
                                              void (KoCanvasResourceProvider::*)(KoResourceUpdateMediatorSP));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(hasResourceUpdateMediator, bool (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(removeResourceUpdateMediator, void (KoCanvasResourceProvider::*)(int));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(addActiveCanvasResourceDependency,
                                              void (KoCanvasResourceProvider::*)(KoActiveCanvasResourceDependencySP));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(hasActiveCanvasResourceDependency,
                                              bool (KoCanvasResourceProvider::*)(int, int) const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(removeActiveCanvasResourceDependency,
                                              void (KoCanvasResourceProvider::*)(int, int));
}

void KoCanvasResourceProviderSchemaContractTest::canvasResourceInterfaceAndNotificationSignaturesRemainStable()
{
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(canvasResourcesInterface,
                                              KoCanvasResourcesInterfaceSP (KoCanvasResourceProvider::*)() const);
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(canvasResourceChanged,
                                              void (KoCanvasResourceProvider::*)(int, const QVariant &));
    ASSERT_CANVAS_RESOURCE_PROVIDER_SIGNATURE(canvasResourceChangeAttempted,
                                              void (KoCanvasResourceProvider::*)(int, const QVariant &));
}

QTEST_GUILESS_MAIN(KoCanvasResourceProviderSchemaContractTest)

#include "KoCanvasResourceProviderSchemaContractTest.moc"
