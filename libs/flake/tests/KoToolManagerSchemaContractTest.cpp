/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoToolManager.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TOOL_ACTION_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolAction::method)), signature>)
#define ASSERT_TOOL_MANAGER_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolManager::method)), signature>)
} // namespace

class KoToolManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolActionTypeLifetimeAndActivationSchemaRemainStable();
    void toolActionMetadataSignaturesRemainStable();
    void toolManagerTypeLifetimeAndControllerSignaturesRemainStable();
    void toolManagerActivationAndResourceSignaturesRemainStable();
    void toolManagerSwitchingAndNotificationSignaturesRemainStable();
};

void KoToolManagerSchemaContractTest::toolActionTypeLifetimeAndActivationSchemaRemainStable()
{
    using Action = KoToolAction;

    static_assert(std::is_class_v<Action>);
    static_assert(std::is_base_of_v<QObject, Action>);
    static_assert(std::is_constructible_v<Action, KoToolFactoryBase *>);
    static_assert(std::has_virtual_destructor_v<Action>);
    ASSERT_TOOL_ACTION_SIGNATURE(trigger, void (Action::*)());
    ASSERT_TOOL_ACTION_SIGNATURE(changed, void (Action::*)());
}

void KoToolManagerSchemaContractTest::toolActionMetadataSignaturesRemainStable()
{
    using Action = KoToolAction;

    ASSERT_TOOL_ACTION_SIGNATURE(buttonGroupId, int (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(iconName, QString (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(iconText, QString (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(id, QString (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(priority, int (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(section, QString (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(shortcut, QKeySequence (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(toolFactory, KoToolFactoryBase * (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(toolTip, QString (Action::*)() const);
    ASSERT_TOOL_ACTION_SIGNATURE(visibilityCode, QString (Action::*)() const);
}

void KoToolManagerSchemaContractTest::toolManagerTypeLifetimeAndControllerSignaturesRemainStable()
{
    using Manager = KoToolManager;
    using InstanceSignature = Manager *(*)();

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_default_constructible_v<Manager>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(static_cast<InstanceSignature>(&Manager::instance)), InstanceSignature>);
    ASSERT_TOOL_MANAGER_SIGNATURE(priv, Manager::Private * (Manager::*)());
    ASSERT_TOOL_MANAGER_SIGNATURE(addController, void (Manager::*)(KoCanvasController *));
    ASSERT_TOOL_MANAGER_SIGNATURE(removeCanvasController, void (Manager::*)(KoCanvasController *));
    ASSERT_TOOL_MANAGER_SIGNATURE(attemptCanvasControllerRemoval, void (Manager::*)(QObject *));
    ASSERT_TOOL_MANAGER_SIGNATURE(activeCanvasController, KoCanvasController * (Manager::*)() const);
    ASSERT_TOOL_MANAGER_SIGNATURE(changedCanvas, void (Manager::*)(const KoCanvasBase *));
    ASSERT_TOOL_MANAGER_SIGNATURE(currentLayerChanged,
                                  void (Manager::*)(const KoCanvasController *, const KoShapeLayer *));
}

void KoToolManagerSchemaContractTest::toolManagerActivationAndResourceSignaturesRemainStable()
{
    using Manager = KoToolManager;

    ASSERT_TOOL_MANAGER_SIGNATURE(activeToolId, QString (Manager::*)() const);
    ASSERT_TOOL_MANAGER_SIGNATURE(initializeToolActions, void (Manager::*)());
    ASSERT_TOOL_MANAGER_SIGNATURE(initializeCurrentToolForCanvas, void (Manager::*)());
    ASSERT_TOOL_MANAGER_SIGNATURE(preferredToolForSelection, QString (Manager::*)(const QList<KoShape *> &));
    ASSERT_TOOL_MANAGER_SIGNATURE(requestToolActivation, void (Manager::*)(KoCanvasController *));
    ASSERT_TOOL_MANAGER_SIGNATURE(toolActionList, QList<KoToolAction *> (Manager::*)() const);
    ASSERT_TOOL_MANAGER_SIGNATURE(toolById, KoToolBase * (Manager::*)(KoCanvasBase *, const QString &) const);
    ASSERT_TOOL_MANAGER_SIGNATURE(currentInputDevice, KoInputDevice (Manager::*)() const);
    ASSERT_TOOL_MANAGER_SIGNATURE(setConverter, void (Manager::*)(KoDerivedResourceConverterSP, KoToolBase *));
    ASSERT_TOOL_MANAGER_SIGNATURE(setAbstractResource,
                                  void (Manager::*)(KoAbstractCanvasResourceInterfaceSP, KoToolBase *));
}

void KoToolManagerSchemaContractTest::toolManagerSwitchingAndNotificationSignaturesRemainStable()
{
    using Manager = KoToolManager;

    ASSERT_TOOL_MANAGER_SIGNATURE(aboutToChangeTool, void (Manager::*)(KoCanvasController *));
    ASSERT_TOOL_MANAGER_SIGNATURE(addedTool, void (Manager::*)(KoToolAction *, KoCanvasController *));
    ASSERT_TOOL_MANAGER_SIGNATURE(changedStatusText, void (Manager::*)(const QString &));
    ASSERT_TOOL_MANAGER_SIGNATURE(changedTool, void (Manager::*)(KoCanvasController *));
    ASSERT_TOOL_MANAGER_SIGNATURE(createOpacityResource, void (Manager::*)(bool, KoToolBase *));
    ASSERT_TOOL_MANAGER_SIGNATURE(inputDeviceChanged, void (Manager::*)(const KoInputDevice &));
    ASSERT_TOOL_MANAGER_SIGNATURE(switchBackRequested, void (Manager::*)());
    ASSERT_TOOL_MANAGER_SIGNATURE(switchInputDeviceRequested, void (Manager::*)(const KoInputDevice &));
    ASSERT_TOOL_MANAGER_SIGNATURE(switchToolRequested, void (Manager::*)(const QString &));
    ASSERT_TOOL_MANAGER_SIGNATURE(textModeChanged, void (Manager::*)(bool));
    ASSERT_TOOL_MANAGER_SIGNATURE(themeChanged, void (Manager::*)());
    ASSERT_TOOL_MANAGER_SIGNATURE(toolCodesSelected, void (Manager::*)(const QList<QString> &));
    ASSERT_TOOL_MANAGER_SIGNATURE(toolOptionWidgetsChanged,
                                  void (Manager::*)(KoCanvasController *, const QList<QPointer<QWidget>> &));
}

QTEST_APPLESS_MAIN(KoToolManagerSchemaContractTest)

#include "KoToolManagerSchemaContractTest.moc"
