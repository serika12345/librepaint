/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "actions/KisLayerSelectionAction.h"
#include "actions/input/KisApplicationInputActions.h"
#include "application/ui/orchestration/KisActionPlugin.h"
#include "application/ui/orchestration/KisPlatformPluginInterfaceFactory.h"
#include "application/ui/orchestration/KisQtWidgetsTweaker.h"
#include "application/ui/workspace/KisAndroidSplash.h"
#include "events/kis_cursor_override_hijacker.h"
#include "theme/KisUiFont.h"

#include <QTest>

#include <type_traits>

class KisCursorOverrideHijackerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cursorOverrideHijackerTypeConstructionAndLifetimeSchemaRemainStable();
    void androidSplashTypeAndStaticApiSchemaRemainStable();
    void qtWidgetsTweakerTypeAndFilteringSchemaRemainStable();
    void platformPluginFactoryTypeAndSingletonSchemaRemainStable();
    void platformPluginFactoryReportingAndMapperSchemaRemainStable();
    void actionPluginTypeConstructionAndLifetimeSchemaRemainStable();
    void uiFontFunctionSchemaRemainsStable();
    void layerSelectionActionTypeAndSelectionSignatureSchemaRemainStable();
    void applicationInputActionsFunctionSchemaRemainsStable();
};

void KisCursorOverrideHijackerSchemaContractTest::cursorOverrideHijackerTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Hijacker = KisCursorOverrideHijacker;

    static_assert(std::is_class_v<Hijacker>);
    static_assert(std::is_default_constructible_v<Hijacker>);
    static_assert(std::is_destructible_v<Hijacker>);
}

void KisCursorOverrideHijackerSchemaContractTest::androidSplashTypeAndStaticApiSchemaRemainStable()
{
    using Splash = KisAndroidSplash;

    static_assert(std::is_class_v<Splash>);
    static_assert(std::is_base_of_v<QObject, Splash>);
    static_assert(std::is_same_v<decltype(&Splash::instance), Splash *(*)()>);
    static_assert(std::is_same_v<decltype(&Splash::show), void (*)()>);
    static_assert(std::is_same_v<decltype(&Splash::setLoaded), void (*)(bool)>);
    static_assert(std::is_same_v<decltype(&Splash::setLoadingText), void (*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Splash::sigSplashDialogDismissed), void (Splash::*)()>);
}

void KisCursorOverrideHijackerSchemaContractTest::qtWidgetsTweakerTypeAndFilteringSchemaRemainStable()
{
    using Tweaker = KisQtWidgetsTweaker;

    static_assert(std::is_class_v<Tweaker>);
    static_assert(std::is_base_of_v<QObject, Tweaker>);
    static_assert(std::is_constructible_v<Tweaker, QObject *>);
    static_assert(std::is_destructible_v<Tweaker>);
    static_assert(std::is_same_v<decltype(&Tweaker::eventFilter), bool (Tweaker::*)(QObject *, QEvent *)>);
    static_assert(std::is_same_v<decltype(&Tweaker::instance), Tweaker *(*)()>);
}

void KisCursorOverrideHijackerSchemaContractTest::platformPluginFactoryTypeAndSingletonSchemaRemainStable()
{
    using Factory = KisPlatformPluginInterfaceFactory;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::instance), Factory *(*)()>);
}

void KisCursorOverrideHijackerSchemaContractTest::platformPluginFactoryReportingAndMapperSchemaRemainStable()
{
    using Factory = KisPlatformPluginInterfaceFactory;

#if KRITA_USE_SURFACE_COLOR_MANAGEMENT_API
    static_assert(std::is_same_v<decltype(&Factory::createSurfaceColorManager),
                                 KisSurfaceColorManagerInterface *(Factory::*)(QWindow *)>);
#endif
    static_assert(std::is_same_v<decltype(&Factory::surfaceColorManagedByOS), bool (Factory::*)()>);
    static_assert(std::is_same_v<decltype(&Factory::colorManagementReport), QString (Factory::*)(QWidget *)>);
    static_assert(std::is_same_v<decltype(&Factory::osPreferredColorSpaceReport), QString (Factory::*)(QWidget *)>);
    static_assert(std::is_same_v<decltype(&Factory::createExtendedModifiersMapper),
                                 KisExtendedModifiersMapperPluginInterface *(Factory::*)()>);
}

void KisCursorOverrideHijackerSchemaContractTest::actionPluginTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Plugin = KisActionPlugin;

    static_assert(std::is_class_v<Plugin>);
    static_assert(std::is_base_of_v<QObject, Plugin>);
    static_assert(std::is_constructible_v<Plugin, QObject *>);
    static_assert(std::has_virtual_destructor_v<Plugin>);
}

void KisCursorOverrideHijackerSchemaContractTest::uiFontFunctionSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&KisUiFont::normalFont), QFont (*)()>);
    static_assert(std::is_same_v<decltype(&KisUiFont::dockFont), QFont (*)()>);
}

void KisCursorOverrideHijackerSchemaContractTest::layerSelectionActionTypeAndSelectionSignatureSchemaRemainStable()
{
    using Action = KisLayerSelectionAction;

    static_assert(std::is_class_v<Action>);
    static_assert(
        std::is_same_v<decltype(&Action::select), void (*)(KisCanvas2 *, const QPoint &, const QPoint &, int, int)>);
}

void KisCursorOverrideHijackerSchemaContractTest::applicationInputActionsFunctionSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&createApplicationInputActions), QList<KisAbstractInputAction *> (*)()>);
    static_assert(std::is_same_v<decltype(&applicationInputCanvas), KisCanvas2 *(*)(const KisInputManager *)>);
}

QTEST_APPLESS_MAIN(KisCursorOverrideHijackerSchemaContractTest)

#include "KisCursorOverrideHijackerSchemaContractTest.moc"
