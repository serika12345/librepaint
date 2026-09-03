/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only */
#include "../xmlgui/KisShortcutsDialog.h"
#include "../xmlgui/KisShortcutsEditor.h"
#include "../xmlgui/kkeysequencewidget.h"
#include "../xmlgui/kmainwindow.h"
#include "../xmlgui/kshortcutwidget.h"
#include "../xmlgui/ktoggletoolbaraction.h"
#include "../xmlgui/ktoolbar.h"
#include "../xmlgui/kxmlguibuilder.h"
#include "../xmlgui/kxmlguifactory.h"
#include "../xmlgui/kxmlguiwindow.h"
#include <QTest>
#include <type_traits>
#include <utility>

class KKeySequenceWidgetSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void keySequenceWidgetTypeAndEnumerationSchemaRemainsStable();
    void keySequenceWidgetLifetimeAndCaptureSchemaRemainsStable();
    void keySequenceWidgetPolicySchemaRemainsStable();
    void keySequenceWidgetValueSchemaRemainsStable();
    void keySequenceWidgetAssociationAndNotificationSchemaRemainsStable();
    void shortcutsEditorTypeAndEnumerationSchemaRemainsStable();
    void shortcutsEditorLifetimeAndStateSchemaRemainsStable();
    void shortcutsEditorCollectionAndPolicySchemaRemainsStable();
    void shortcutsEditorPersistenceSchemaRemainsStable();
    void shortcutsEditorInteractionSchemaRemainsStable();
    void xmlGuiWindowTypeAndOptionSchemaRemainsStable();
    void xmlGuiWindowLifetimeAndFeaturePolicySchemaRemainsStable();
    void xmlGuiWindowCreationAndSetupSchemaRemainsStable();
    void xmlGuiWindowFactoryAndFinalizationSchemaRemainsStable();
    void xmlGuiWindowStateNotificationSchemaRemainsStable();
    void mainWindowTypeAndLifetimeSchemaRemainsStable();
    void mainWindowSessionRestoreSchemaRemainsStable();
    void mainWindowChromeAndToolbarSchemaRemainsStable();
    void mainWindowAutoSaveSchemaRemainsStable();
    void mainWindowPersistenceAndNotificationSchemaRemainsStable();
    void toolBarAndToggleActionTypeAndLifetimeSchemaRemainStable();
    void toolBarOwnershipAndAppearanceSignaturesRemainStable();
    void toolBarSettingsAndXmlSignaturesRemainStable();
    void toolBarClientAndGlobalPolicySignaturesRemainStable();
    void toggleToolBarActionInteractionSignaturesRemainStable();
    void xmlGuiFactoryTypeLifetimeAndConfigurationSchemaRemainsStable();
    void xmlGuiFactoryClientAndActionSignaturesRemainStable();
    void xmlGuiFactoryContainerResetAndNotificationSignaturesRemainStable();
    void xmlGuiBuilderTypeOwnershipAndTagSchemaRemainsStable();
    void xmlGuiBuilderElementLifecycleSignaturesRemainStable();
    void shortcutWidgetTypeAndLifetimeSchemaRemainStable();
    void shortcutWidgetPolicySignaturesRemainStable();
    void shortcutWidgetEditingSignaturesRemainStable();
    void shortcutsDialogTypeAndCollectionSchemaRemainStable();
    void shortcutsDialogPersistenceSchemaRemainStable();
};
void KKeySequenceWidgetSchemaContractTest::keySequenceWidgetTypeAndEnumerationSchemaRemainsStable()
{
    using W = KisKKeySequenceWidget;
    static_assert(std::is_class_v<W> && std::is_enum_v<W::Validation> && std::is_enum_v<W::ShortcutType>);
    static_assert(W::Validate == 0 && W::NoValidate == 1);
    static_assert(W::None == 0 && W::LocalShortcuts == 1 && W::StandardShortcuts == 2 && W::GlobalShortcuts == 4);
    static_assert(std::is_same_v<W::ShortcutTypes, QFlags<W::ShortcutType>>);
}
void KKeySequenceWidgetSchemaContractTest::keySequenceWidgetLifetimeAndCaptureSchemaRemainsStable()
{
    using W = KisKKeySequenceWidget;
    using A = void (W::*)();
    static_assert(std::is_constructible_v<W, QWidget *> && std::is_default_constructible_v<W>
                  && std::is_destructible_v<W>);
    static_assert(std::is_same_v<decltype(&W::captureKeySequence), A>
                  && std::is_same_v<decltype(&W::clearKeySequence), A>);
}
void KKeySequenceWidgetSchemaContractTest::keySequenceWidgetPolicySchemaRemainsStable()
{
    using W = KisKKeySequenceWidget;
    static_assert(std::is_same_v<decltype(&W::setCheckForConflictsAgainst), void (W::*)(W::ShortcutTypes)>);
    static_assert(std::is_same_v<decltype(&W::checkForConflictsAgainst), W::ShortcutTypes (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setMultiKeyShortcutsAllowed), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::multiKeyShortcutsAllowed), bool (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setModifierlessAllowed), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::isModifierlessAllowed), bool (W::*)()>);
}
void KKeySequenceWidgetSchemaContractTest::keySequenceWidgetValueSchemaRemainsStable()
{
    using W = KisKKeySequenceWidget;
    static_assert(std::is_same_v<decltype(&W::isKeySequenceAvailable), bool (W::*)(const QKeySequence &) const>);
    static_assert(std::is_same_v<decltype(&W::keySequence), QKeySequence (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setKeySequence), void (W::*)(const QKeySequence &, W::Validation)>);
    static_assert(
        std::is_same_v<decltype(std::declval<W &>().setKeySequence(std::declval<const QKeySequence &>())), void>);
    static_assert(std::is_same_v<decltype(&W::setClearButtonShown), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::setComponentName), void (W::*)(const QString &)>);
}
void KKeySequenceWidgetSchemaContractTest::keySequenceWidgetAssociationAndNotificationSchemaRemainsStable()
{
    using W = KisKKeySequenceWidget;
    static_assert(
        std::is_same_v<decltype(&W::setCheckActionCollections), void (W::*)(const QList<KisKActionCollection *> &)>);
    static_assert(std::is_same_v<decltype(&W::keySequenceChanged), void (W::*)(const QKeySequence &)>);
    static_assert(std::is_same_v<decltype(&W::stealShortcut), void (W::*)(const QKeySequence &, QAction *)>);
    static_assert(std::is_same_v<decltype(&W::applyStealShortcut), void (W::*)()>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsEditorTypeAndEnumerationSchemaRemainsStable()
{
    using E = KisShortcutsEditor;
    static_assert(std::is_class_v<E> && std::is_enum_v<E::ActionType> && std::is_enum_v<E::LetterShortcuts>);
    static_assert(int(E::WidgetAction) == int(Qt::WidgetShortcut) && int(E::WindowAction) == int(Qt::WindowShortcut));
    static_assert(int(E::ApplicationAction) == int(Qt::ApplicationShortcut) && E::GlobalAction == 4);
    static_assert(quint32(E::AllActions) == 0xffffffffu);
    static_assert(std::is_same_v<E::ActionTypes, QFlags<E::ActionType>>);
    static_assert(E::LetterShortcutsDisallowed == 0 && E::LetterShortcutsAllowed == 1);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsEditorLifetimeAndStateSchemaRemainsStable()
{
    using E = KisShortcutsEditor;
    static_assert(std::is_constructible_v<E, QWidget *, E::ActionTypes, E::LetterShortcuts>);
    static_assert(std::is_constructible_v<E, QWidget *>);
    static_assert(std::is_destructible_v<E>);
    static_assert(std::is_same_v<decltype(&E::isModified), bool (E::*)() const>);
    static_assert(std::is_same_v<decltype(&E::actionTypes), E::ActionTypes (E::*)() const>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsEditorCollectionAndPolicySchemaRemainsStable()
{
    using E = KisShortcutsEditor;
    static_assert(std::is_same_v<decltype(&E::clearCollections), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::addCollection), void (E::*)(KisKActionCollection *, const QString &)>);
    static_assert(std::is_same_v<decltype(std::declval<E &>().addCollection(nullptr)), void>);
    static_assert(std::is_same_v<decltype(&E::setActionTypes), void (E::*)(E::ActionTypes)>);
    static_assert(std::is_same_v<decltype(&E::clearConfiguration), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::undo), void (E::*)()>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsEditorPersistenceSchemaRemainsStable()
{
    using E = KisShortcutsEditor;
    static_assert(std::is_same_v<decltype(&E::save), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::commit), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::saveShortcuts), void (E::*)(KConfigGroup *) const>);
    static_assert(std::is_same_v<decltype(std::declval<const E &>().saveShortcuts()), void>);
    static_assert(std::is_same_v<decltype(&E::exportConfiguration), void (E::*)(KConfigBase *) const>);
    static_assert(std::is_same_v<decltype(&E::importConfiguration), void (E::*)(KConfigBase *, bool)>);
    static_assert(std::is_same_v<decltype(&E::allDefault), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::printShortcuts), void (E::*)() const>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsEditorInteractionSchemaRemainsStable()
{
    using E = KisShortcutsEditor;
    static_assert(std::is_same_v<decltype(&E::clearSearch), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::keyChange), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::resizeColumns), void (E::*)()>);
    static_assert(std::is_same_v<decltype(&E::searchUpdated), void (E::*)(QString)>);
    static_assert(std::is_same_v<decltype(&E::slotScrollerStateChanged), void (E::*)(QScroller::State)>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiWindowTypeAndOptionSchemaRemainsStable()
{
    using W = KXmlGuiWindow;
    static_assert(std::is_class_v<W> && std::is_enum_v<W::StandardWindowOption>);
    static_assert(std::is_same_v<W::StandardWindowOptions, QFlags<W::StandardWindowOption>>);
    static_assert(W::ToolBar == 1 && W::Keys == 2 && W::StatusBar == 4);
    static_assert(W::Save == 8 && W::Create == 16 && W::Default == 31);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiWindowLifetimeAndFeaturePolicySchemaRemainsStable()
{
    using W = KXmlGuiWindow;
    static_assert(std::is_constructible_v<W, QWidget *, Qt::WindowFlags> && std::is_default_constructible_v<W>);
    static_assert(std::is_destructible_v<W>);
    static_assert(std::is_same_v<decltype(&W::setHelpMenuEnabled), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().setHelpMenuEnabled()), void>);
    static_assert(std::is_same_v<decltype(&W::isHelpMenuEnabled), bool (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setStandardToolBarMenuEnabled), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::isStandardToolBarMenuEnabled), bool (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::createStandardStatusBarAction), void (W::*)()>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiWindowCreationAndSetupSchemaRemainsStable()
{
    using W = KXmlGuiWindow;
    using Setup = void (W::*)(W::StandardWindowOptions, const QString &);
    using SizedSetup = void (W::*)(const QSize &, W::StandardWindowOptions, const QString &);
    static_assert(std::is_same_v<decltype(&W::createGUI), void (W::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().createGUI()), void>);
    static_assert(std::is_same_v<decltype(static_cast<Setup>(&W::setupGUI)), Setup>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().setupGUI()), void>);
    static_assert(std::is_same_v<decltype(static_cast<SizedSetup>(&W::setupGUI)), SizedSetup>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().setupGUI(std::declval<const QSize &>())), void>);
    static_assert(std::is_same_v<decltype(&W::applyMainWindowSettings), void (W::*)(const KConfigGroup &)>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiWindowFactoryAndFinalizationSchemaRemainsStable()
{
    using W = KXmlGuiWindow;
    using FinalizeClient = void (W::*)(KisKXMLGUIClient *);
    using FinalizeForce = void (W::*)(bool);
    static_assert(std::is_same_v<decltype(&W::guiFactory), KisKXMLGUIFactory *(W::*)()>);
    static_assert(std::is_same_v<decltype(&W::toolBarMenuAction), QAction *(W::*)()>);
    static_assert(std::is_same_v<decltype(&W::setupToolbarMenuActions), void (W::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<FinalizeClient>(&W::finalizeGUI)), FinalizeClient>);
    static_assert(std::is_same_v<decltype(static_cast<FinalizeForce>(&W::finalizeGUI)), FinalizeForce>);
    static_assert(std::is_same_v<decltype(&W::configureToolbars), void (W::*)()>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiWindowStateNotificationSchemaRemainsStable()
{
    using W = KXmlGuiWindow;
    using State = void (W::*)(const QString &);
    using ReversedState = void (W::*)(const QString &, bool);
    static_assert(std::is_same_v<decltype(static_cast<State>(&W::slotStateChanged)), State>);
    static_assert(std::is_same_v<decltype(static_cast<ReversedState>(&W::slotStateChanged)), ReversedState>);
}

void KKeySequenceWidgetSchemaContractTest::mainWindowTypeAndLifetimeSchemaRemainsStable()
{
    using W = KisKMainWindow;
    static_assert(std::is_class_v<W>);
    static_assert(std::is_constructible_v<W, QWidget *, Qt::WindowFlags> && std::is_default_constructible_v<W>);
    static_assert(std::has_virtual_destructor_v<W>);
    static_assert(std::is_same_v<decltype(&kRestoreMainWindows<W>), void (*)()>);
}

void KKeySequenceWidgetSchemaContractTest::mainWindowSessionRestoreSchemaRemainsStable()
{
    using W = KisKMainWindow;
    static_assert(std::is_same_v<decltype(&W::canBeRestored), bool (*)(int)>);
    static_assert(std::is_same_v<decltype(&W::classNameOfToplevel), const QString (*)(int)>);
    static_assert(std::is_same_v<decltype(&W::restore), bool (W::*)(int, bool)>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().restore(1)), bool>);
    static_assert(std::is_same_v<decltype(&W::memberList), QList<W *> (*)()>);
}

void KKeySequenceWidgetSchemaContractTest::mainWindowChromeAndToolbarSchemaRemainsStable()
{
    using W = KisKMainWindow;
    static_assert(std::is_same_v<decltype(&W::hasMenuBar), bool (W::*)()>);
    static_assert(std::is_same_v<decltype(&W::toolBar), KisToolBar *(W::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().toolBar()), KisToolBar *>);
    static_assert(std::is_same_v<decltype(&W::toolBars), QList<KisToolBar *> (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::appHelpActivated), void (W::*)()>);
}

void KKeySequenceWidgetSchemaContractTest::mainWindowAutoSaveSchemaRemainsStable()
{
    using W = KisKMainWindow;
    using NamedSettings = void (W::*)(const QString &, bool);
    using GroupSettings = void (W::*)(const KConfigGroup &, bool);
    static_assert(std::is_same_v<decltype(static_cast<NamedSettings>(&W::setAutoSaveSettings)), NamedSettings>);
    static_assert(std::is_same_v<decltype(std::declval<W &>().setAutoSaveSettings()), void>);
    static_assert(std::is_same_v<decltype(static_cast<GroupSettings>(&W::setAutoSaveSettings)), GroupSettings>);
    static_assert(
        std::is_same_v<decltype(std::declval<W &>().setAutoSaveSettings(std::declval<const KConfigGroup &>())), void>);
    static_assert(std::is_same_v<decltype(&W::resetAutoSaveSettings), void (W::*)()>);
    static_assert(std::is_same_v<decltype(&W::autoSaveSettings), bool (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::autoSaveGroup), QString (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::autoSaveConfigGroup), KConfigGroup (W::*)() const>);
}

void KKeySequenceWidgetSchemaContractTest::mainWindowPersistenceAndNotificationSchemaRemainsStable()
{
    using W = KisKMainWindow;
    static_assert(std::is_same_v<decltype(&W::applyMainWindowSettings), void (W::*)(const KConfigGroup &)>);
    static_assert(std::is_same_v<decltype(&W::saveMainWindowSettings), void (W::*)(KConfigGroup &)>);
    static_assert(std::is_same_v<decltype(&W::dbusName), QString (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setSettingsDirty), void (W::*)()>);
}

void KKeySequenceWidgetSchemaContractTest::toolBarAndToggleActionTypeAndLifetimeSchemaRemainStable()
{
    using ToolBar = KisToolBar;
    using ToggleAction = KToggleToolBarAction;

    static_assert(std::is_class_v<ToolBar> && std::is_base_of_v<QToolBar, ToolBar>);
    static_assert(std::is_constructible_v<ToolBar, const QString &, QWidget *>);
    static_assert(std::is_constructible_v<ToolBar, const QString &, QWidget *, bool>);
    static_assert(std::has_virtual_destructor_v<ToolBar>);
    static_assert(std::is_class_v<ToggleAction> && std::is_base_of_v<KToggleAction, ToggleAction>);
    static_assert(std::is_constructible_v<ToggleAction, const char *, const QString &, QObject *>);
    static_assert(std::is_constructible_v<ToggleAction, ToolBar *, const QString &, QObject *>);
    static_assert(std::has_virtual_destructor_v<ToggleAction>);
}

void KKeySequenceWidgetSchemaContractTest::toolBarOwnershipAndAppearanceSignaturesRemainStable()
{
    using W = KisToolBar;

    static_assert(std::is_same_v<decltype(&W::mainWindow), KisKMainWindow *(W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setIconDimensions), void (W::*)(int)>);
    static_assert(std::is_same_v<decltype(&W::iconSizeDefault), int (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::eventFilter), bool (W::*)(QObject *, QEvent *)>);
}

void KKeySequenceWidgetSchemaContractTest::toolBarSettingsAndXmlSignaturesRemainStable()
{
    using W = KisToolBar;

    static_assert(std::is_same_v<decltype(&W::saveSettings), void (W::*)(KConfigGroup &)>);
    static_assert(std::is_same_v<decltype(&W::applySettings), void (W::*)(const KConfigGroup &)>);
    static_assert(std::is_same_v<decltype(&W::loadState), void (W::*)(const QDomElement &)>);
    static_assert(std::is_same_v<decltype(&W::saveState), void (W::*)(QDomElement &) const>);
}

void KKeySequenceWidgetSchemaContractTest::toolBarClientAndGlobalPolicySignaturesRemainStable()
{
    using W = KisToolBar;

    static_assert(std::is_same_v<decltype(&W::addXMLGUIClient), void (W::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&W::removeXMLGUIClient), void (W::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&W::toolBarsEditable), bool (*)()>);
    static_assert(std::is_same_v<decltype(&W::setToolBarsEditable), void (*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::toolBarsLocked), bool (*)()>);
    static_assert(std::is_same_v<decltype(&W::setToolBarsLocked), void (*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::emitToolbarStyleChanged), void (*)()>);
    static_assert(std::is_same_v<decltype(&W::toolBarStateModel), QObject *(*)()>);
}

void KKeySequenceWidgetSchemaContractTest::toggleToolBarActionInteractionSignaturesRemainStable()
{
    using W = KToggleToolBarAction;

    static_assert(std::is_same_v<decltype(&W::toolBar), KisToolBar *(W::*)()>);
    static_assert(std::is_same_v<decltype(&W::eventFilter), bool (W::*)(QObject *, QEvent *)>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiFactoryTypeLifetimeAndConfigurationSchemaRemainsStable()
{
    using Factory = KisKXMLGUIFactory;

    static_assert(std::is_class_v<Factory> && std::is_base_of_v<QObject, Factory>);
    static_assert(std::is_constructible_v<Factory, KisKXMLGUIBuilder *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::readConfigFile), QString (*)(const QString &, const QString &)>);
    static_assert(std::is_same_v<decltype(&Factory::saveConfigFile),
                                 bool (*)(const QDomDocument &, const QString &, const QString &)>);
    static_assert(std::is_same_v<decltype(&Factory::actionPropertiesElement), QDomElement (*)(QDomDocument &)>);
    static_assert(
        std::is_same_v<decltype(&Factory::findActionByName), QDomElement (*)(QDomElement &, const QString &, bool)>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiFactoryClientAndActionSignaturesRemainStable()
{
    using Factory = KisKXMLGUIFactory;

    static_assert(std::is_same_v<decltype(&Factory::addClient), void (Factory::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&Factory::removeClient), void (Factory::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&Factory::plugActionList),
                                 void (Factory::*)(KisKXMLGUIClient *, const QString &, const QList<QAction *> &)>);
    static_assert(
        std::is_same_v<decltype(&Factory::unplugActionList), void (Factory::*)(KisKXMLGUIClient *, const QString &)>);
    static_assert(std::is_same_v<decltype(&Factory::clients), QList<KisKXMLGUIClient *> (Factory::*)() const>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiFactoryContainerResetAndNotificationSignaturesRemainStable()
{
    using Factory = KisKXMLGUIFactory;

    static_assert(std::is_same_v<decltype(&Factory::container),
                                 QWidget *(Factory::*)(const QString &, KisKXMLGUIClient *, bool)>);
    static_assert(std::is_same_v<decltype(&Factory::containers), QList<QWidget *> (Factory::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Factory::reset), void (Factory::*)()>);
    static_assert(std::is_same_v<decltype(&Factory::resetContainer), void (Factory::*)(const QString &, bool)>);
    static_assert(std::is_same_v<decltype(&Factory::clientAdded), void (Factory::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&Factory::clientRemoved), void (Factory::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&Factory::makingChanges), void (Factory::*)(bool)>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiBuilderTypeOwnershipAndTagSchemaRemainsStable()
{
    using Builder = KisKXMLGUIBuilder;

    static_assert(std::is_class_v<Builder>);
    static_assert(std::is_constructible_v<Builder, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Builder>);
    static_assert(std::is_same_v<decltype(&Builder::builderClient), KisKXMLGUIClient *(Builder::*)() const>);
    static_assert(std::is_same_v<decltype(&Builder::setBuilderClient), void (Builder::*)(KisKXMLGUIClient *)>);
    static_assert(std::is_same_v<decltype(&Builder::widget), QWidget *(Builder::*)()>);
    static_assert(std::is_same_v<decltype(&Builder::containerTags), QStringList (Builder::*)() const>);
    static_assert(std::is_same_v<decltype(&Builder::customTags), QStringList (Builder::*)() const>);
}

void KKeySequenceWidgetSchemaContractTest::xmlGuiBuilderElementLifecycleSignaturesRemainStable()
{
    using Builder = KisKXMLGUIBuilder;

    static_assert(std::is_same_v<decltype(&Builder::createContainer),
                                 QWidget *(Builder::*)(QWidget *, int, const QDomElement &, QAction *&)>);
    static_assert(std::is_same_v<decltype(&Builder::removeContainer),
                                 void (Builder::*)(QWidget *, QWidget *, QDomElement &, QAction *)>);
    static_assert(std::is_same_v<decltype(&Builder::createCustomElement),
                                 QAction *(Builder::*)(QWidget *, int, const QDomElement &)>);
    static_assert(std::is_same_v<decltype(&Builder::removeCustomElement), void (Builder::*)(QWidget *, QAction *)>);
    static_assert(std::is_same_v<decltype(&Builder::finalizeGUI), void (Builder::*)(KisKXMLGUIClient *)>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutWidgetTypeAndLifetimeSchemaRemainStable()
{
    using W = KisKShortcutWidget;

    static_assert(std::is_class_v<W> && std::is_base_of_v<QWidget, W>);
    static_assert(std::is_constructible_v<W, QWidget *> && std::is_default_constructible_v<W>);
    static_assert(std::has_virtual_destructor_v<W>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutWidgetPolicySignaturesRemainStable()
{
    using W = KisKShortcutWidget;
    using Shortcuts = QList<QKeySequence>;
    using ActionCollections = QList<KisKActionCollection *>;

    static_assert(std::is_same_v<decltype(&W::isModifierlessAllowed), bool (W::*)()>);
    static_assert(std::is_same_v<decltype(&W::setModifierlessAllowed), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::setClearButtonsShown), void (W::*)(bool)>);
    static_assert(std::is_same_v<decltype(&W::shortcut), Shortcuts (W::*)() const>);
    static_assert(std::is_same_v<decltype(&W::setShortcut), void (W::*)(const Shortcuts &)>);
    static_assert(std::is_same_v<decltype(&W::setCheckActionCollections), void (W::*)(const ActionCollections &)>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutWidgetEditingSignaturesRemainStable()
{
    using W = KisKShortcutWidget;
    using Shortcuts = QList<QKeySequence>;

    static_assert(std::is_same_v<decltype(&W::applyStealShortcut), void (W::*)()>);
    static_assert(std::is_same_v<decltype(&W::clearShortcut), void (W::*)()>);
    static_assert(std::is_same_v<decltype(&W::shortcutChanged), void (W::*)(const Shortcuts &)>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsDialogTypeAndCollectionSchemaRemainStable()
{
    using D = KisShortcutsDialog;
    using E = KisShortcutsEditor;
    using Collections = QList<KisKActionCollection *>;

    static_assert(std::is_class_v<D> && std::is_base_of_v<QWidget, D>);
    static_assert(std::is_constructible_v<D, E::ActionTypes, E::LetterShortcuts, QWidget *>);
    static_assert(std::is_default_constructible_v<D> && std::has_virtual_destructor_v<D>);
    static_assert(std::is_same_v<decltype(defaultActionTypes), const E::ActionTypes>);
    QVERIFY(defaultActionTypes == (E::WidgetAction | E::WindowAction | E::ApplicationAction));
    static_assert(std::is_same_v<decltype(&D::addCollection), void (D::*)(KisKActionCollection *, const QString &)>);
    static_assert(std::is_same_v<decltype(std::declval<D &>().addCollection(nullptr)), void>);
    static_assert(std::is_same_v<decltype(&D::actionCollections), Collections (D::*)() const>);
    static_assert(std::is_same_v<decltype(&D::sizeHint), QSize (D::*)() const>);
}

void KKeySequenceWidgetSchemaContractTest::shortcutsDialogPersistenceSchemaRemainStable()
{
    using D = KisShortcutsDialog;

    static_assert(std::is_same_v<decltype(&D::save), void (D::*)()>);
    static_assert(std::is_same_v<decltype(&D::allDefault), void (D::*)()>);
    static_assert(std::is_same_v<decltype(&D::undo), void (D::*)()>);
    static_assert(std::is_same_v<decltype(&D::importConfiguration), void (D::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&D::exportConfiguration), void (D::*)(const QString &) const>);
    static_assert(std::is_same_v<decltype(&D::loadCustomShortcuts), void (D::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&D::saveCustomShortcuts), void (D::*)(const QString &) const>);
}

QTEST_GUILESS_MAIN(KKeySequenceWidgetSchemaContractTest)
#include "KKeySequenceWidgetSchemaContractTest.moc"
