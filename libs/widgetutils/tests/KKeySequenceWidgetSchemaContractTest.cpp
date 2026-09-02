/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only */
#include "../xmlgui/kkeysequencewidget.h"
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
QTEST_GUILESS_MAIN(KKeySequenceWidgetSchemaContractTest)
#include "KKeySequenceWidgetSchemaContractTest.moc"
