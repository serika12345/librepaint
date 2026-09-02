/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only */
#include "../xmlgui/KisShortcutsEditor.h"
#include "../xmlgui/kkeysequencewidget.h"
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
QTEST_GUILESS_MAIN(KKeySequenceWidgetSchemaContractTest)
#include "KKeySequenceWidgetSchemaContractTest.moc"
