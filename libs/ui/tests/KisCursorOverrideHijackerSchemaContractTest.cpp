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
#include "canvas/kis_abstract_perspective_grid.h"
#include "canvas/kis_canvas_controls_manager.h"
#include "document/KisTextPropertiesManager.h"
#include "events/kis_cursor_override_hijacker.h"
#include "platform/osx.h"
#include "theme/KisUiFont.h"
#include "tool/kis_tool_canvas_utils.h"
#include "utils/KisRecentDocumentsModelWrapper.h"

#include <QTest>

#include <type_traits>

namespace
{
class PerspectiveGridProbe : public KisAbstractPerspectiveGrid
{
public:
    using KisAbstractPerspectiveGrid::KisAbstractPerspectiveGrid;

    bool contains(const QPointF &) const override
    {
        return false;
    }
    qreal distance(const QPointF &) const override
    {
        return 0.0;
    }
    bool isActive() const override
    {
        return false;
    }
};
} // namespace

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
    void canvasToolUtilityFunctionSchemaRemainsStable();
    void macOSMouseCoalescingFunctionSchemaRemainsStable();
    void abstractPerspectiveGridTypeAndGeometrySchemaRemainStable();
    void canvasControlsManagerTypeAndConfigurationSchemaRemainStable();
    void textPropertiesManagerTypeAndProviderSchemaRemainStable();
    void recentDocumentsModelWrapperSchemaRemainsStable();
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

void KisCursorOverrideHijackerSchemaContractTest::canvasToolUtilityFunctionSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&KisToolUtils::shapeHoverInfoCrossLayer),
                                 QPainterPath (*)(KoCanvasBase *, const QPointF &, QString &, bool *, bool)>);
    static_assert(std::is_same_v<decltype(&KisToolUtils::selectShapeCrossLayer),
                                 bool (*)(KoCanvasBase *, const QPointF &, const QString &, bool)>);
    static_assert(
        std::is_same_v<decltype(&KisToolUtils::showBrushSizeFloatingMessage), void (*)(KoCanvasBase *, qreal)>);
}

void KisCursorOverrideHijackerSchemaContractTest::macOSMouseCoalescingFunctionSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&isMouseCoalescingEnabled), bool (*)()>);
    static_assert(std::is_same_v<decltype(&setMouseCoalescingEnabled), void (*)(bool)>);
}

void KisCursorOverrideHijackerSchemaContractTest::abstractPerspectiveGridTypeAndGeometrySchemaRemainStable()
{
    using Grid = KisAbstractPerspectiveGrid;

    static_assert(std::is_class_v<Grid>);
    static_assert(std::is_base_of_v<QObject, Grid>);
    static_assert(std::is_abstract_v<Grid>);
    static_assert(std::is_constructible_v<PerspectiveGridProbe, QObject *>);
    static_assert(std::has_virtual_destructor_v<Grid>);
    static_assert(std::is_same_v<decltype(&Grid::contains), bool (Grid::*)(const QPointF &) const>);
    static_assert(std::is_same_v<decltype(&Grid::distance), qreal (Grid::*)(const QPointF &) const>);
    static_assert(std::is_same_v<decltype(&Grid::isActive), bool (Grid::*)() const>);
}

void KisCursorOverrideHijackerSchemaContractTest::canvasControlsManagerTypeAndConfigurationSchemaRemainStable()
{
    using Manager = KisCanvasControlsManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager, KisViewManager *>);
    static_assert(std::is_destructible_v<Manager>);
    static_assert(std::is_same_v<decltype(&Manager::setup), void (Manager::*)(KisActionManager *)>);
    static_assert(std::is_same_v<decltype(&Manager::setView), void (Manager::*)(QPointer<KisView>)>);
}

void KisCursorOverrideHijackerSchemaContractTest::textPropertiesManagerTypeAndProviderSchemaRemainStable()
{
    using Manager = KisTextPropertiesManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager, QObject *>);
    static_assert(std::is_destructible_v<Manager>);
    static_assert(
        std::is_same_v<decltype(&Manager::setCanvasResourceProvider), void (Manager::*)(KisCanvasResourceProvider *)>);
    static_assert(std::is_same_v<decltype(&Manager::setTextPropertiesInterface),
                                 void (Manager::*)(KoSvgTextPropertiesInterface *)>);
}

void KisCursorOverrideHijackerSchemaContractTest::recentDocumentsModelWrapperSchemaRemainsStable()
{
    using Wrapper = KisRecentDocumentsModelWrapper;

    static_assert(std::is_class_v<Wrapper>);
    static_assert(std::is_base_of_v<QObject, Wrapper>);
    static_assert(std::is_same_v<decltype(Wrapper::ICON_SIZE_LENGTH), const int>);
    static_assert(Wrapper::ICON_SIZE_LENGTH == 200);
    static_assert(std::is_same_v<decltype(&Wrapper::instance), Wrapper *(*)()>);
    static_assert(std::is_same_v<decltype(&Wrapper::model), QStandardItemModel &(Wrapper::*)()>);
    static_assert(std::is_same_v<decltype(&Wrapper::sigModelIsUpToDate), void (Wrapper::*)()>);
}

QTEST_APPLESS_MAIN(KisCursorOverrideHijackerSchemaContractTest)

#include "KisCursorOverrideHijackerSchemaContractTest.moc"
