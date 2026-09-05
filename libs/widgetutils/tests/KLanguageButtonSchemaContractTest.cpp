/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config/klanguagebutton.h>

#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_LANGUAGE_BUTTON_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KLanguageButton::method)), signature>)

class KLanguageButtonSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void languageButtonTypeLifetimeAndConstructionSchemaRemainStable();
    void languageButtonLocaleAndPresentationSignaturesRemainStable();
    void languageButtonPopulationSignaturesRemainStable();
    void languageButtonSelectionAndQuerySignaturesRemainStable();
    void languageButtonNotificationSignaturesRemainStable();
};

void KLanguageButtonSchemaContractTest::languageButtonTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Button = KLanguageButton;
    static_assert(std::is_class_v<Button>);
    static_assert(std::is_base_of_v<QWidget, Button>);
    static_assert(std::is_default_constructible_v<Button>);
    static_assert(std::is_constructible_v<Button, QWidget *>);
    static_assert(std::is_constructible_v<Button, const QString &>);
    static_assert(std::is_constructible_v<Button, const QString &, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Button>);
}

void KLanguageButtonSchemaContractTest::languageButtonLocaleAndPresentationSignaturesRemainStable()
{
    using Button = KLanguageButton;
    using StringSetter = void (Button::*)(const QString &);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(setLocale, StringSetter);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(setText, StringSetter);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(showLanguageCodes, void (Button::*)(bool));
}

void KLanguageButtonSchemaContractTest::languageButtonPopulationSignaturesRemainStable()
{
    using Button = KLanguageButton;
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(clear, void (Button::*)());
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(insertLanguage, void (Button::*)(const QString &, const QString &, int));
    static_assert(
        std::is_same_v<decltype(std::declval<Button &>().insertLanguage(std::declval<const QString &>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Button &>().insertLanguage(std::declval<const QString &>(),
                                                                                  std::declval<const QString &>())),
                                 void>);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(insertSeparator, void (Button::*)(int));
    static_assert(std::is_same_v<decltype(std::declval<Button &>().insertSeparator()), void>);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(loadAllLanguages, void (Button::*)());
}

void KLanguageButtonSchemaContractTest::languageButtonSelectionAndQuerySignaturesRemainStable()
{
    using Button = KLanguageButton;
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(contains, bool (Button::*)(const QString &) const);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(count, int (Button::*)() const);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(current, QString (Button::*)() const);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(setCurrentItem, void (Button::*)(const QString &));
}

void KLanguageButtonSchemaContractTest::languageButtonNotificationSignaturesRemainStable()
{
    using Button = KLanguageButton;
    using LanguageNotification = void (Button::*)(const QString &);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(activated, LanguageNotification);
    ASSERT_LANGUAGE_BUTTON_SIGNATURE(highlighted, LanguageNotification);
}

QTEST_GUILESS_MAIN(KLanguageButtonSchemaContractTest)
#include "KLanguageButtonSchemaContractTest.moc"
