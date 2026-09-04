/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/KoDualColorButton.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KoDualColorButtonSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void dualColorButtonTypeLifetimeAndSelectionSchemaRemainStable();
    void dualColorButtonPresentationStateAndRenderingSchemaRemainStable();
    void dualColorButtonColorStateAndConversionSignaturesRemainStable();
    void dualColorButtonDialogControlSignaturesRemainStable();
    void dualColorButtonDialogResultAndNotificationSignaturesRemainStable();
};

void KoDualColorButtonSchemaContractTest::dualColorButtonTypeLifetimeAndSelectionSchemaRemainStable()
{
    using Button = KoDualColorButton;
    static_assert(std::is_class_v<Button>);
    static_assert(std::is_enum_v<Button::Selection>);
    static_assert(Button::Foreground == 0);
    static_assert(Button::Background == 1);
    static_assert(
        std::is_constructible_v<Button, KisCanvasResourceProvider *, const KoColorDisplayRendererInterface *>);
    static_assert(std::is_constructible_v<Button,
                                          KisCanvasResourceProvider *,
                                          const KoColorDisplayRendererInterface *,
                                          QWidget *,
                                          QWidget *>);
    static_assert(std::has_virtual_destructor_v<Button>);
    static_assert(std::is_same_v<decltype(&Button::setColorDialogState), void (Button::*)(Button::Selection)>);
    static_assert(std::is_same_v<decltype(&Button::sizeHint), QSize (Button::*)() const>);
}

void KoDualColorButtonSchemaContractTest::dualColorButtonPresentationStateAndRenderingSchemaRemainStable()
{
    using Button = KoDualColorButton;
    using PixmapMember = QPixmap Button::*;
    using RectMember = QRect Button::*;
    static_assert(std::is_same_v<decltype(&Button::arrowBitmap), PixmapMember>);
    static_assert(std::is_same_v<decltype(&Button::backgroundRect), RectMember>);
    static_assert(std::is_same_v<decltype(&Button::foregroundRect), RectMember>);
    static_assert(std::is_same_v<decltype(&Button::resetArrowPixmap), PixmapMember>);
    static_assert(std::is_same_v<decltype(&Button::resetColours), bool Button::*>);
    static_assert(std::is_same_v<decltype(&Button::resetPixmap), PixmapMember>);
    static_assert(std::is_same_v<decltype(&Button::paint_icons), void (Button::*)(QPainter &)>);
    static_assert(std::is_same_v<decltype(&Button::updateArrows), void (Button::*)()>);
}

void KoDualColorButtonSchemaContractTest::dualColorButtonColorStateAndConversionSignaturesRemainStable()
{
    using Button = KoDualColorButton;
    using Action = void (Button::*)();
    using ColorGetter = KoColor (Button::*)() const;
    using ColorSetter = void (Button::*)(const KoColor &);
    static_assert(std::is_same_v<decltype(&Button::ResetColours), Action>);
    static_assert(std::is_same_v<decltype(&Button::backgroundColor), ColorGetter>);
    static_assert(std::is_same_v<decltype(&Button::foregroundColor), ColorGetter>);
    static_assert(std::is_same_v<decltype(&Button::getColorFromDisplayRenderer), QColor (Button::*)(KoColor)>);
    static_assert(std::is_same_v<decltype(&Button::setBackgroundColor), ColorSetter>);
    static_assert(std::is_same_v<decltype(&Button::setForegroundColor), ColorSetter>);
    static_assert(std::is_same_v<decltype(&Button::swapColours), Action>);
    static_assert(std::is_same_v<decltype(&Button::updateColorSpace), Action>);
}

void KoDualColorButtonSchemaContractTest::dualColorButtonDialogControlSignaturesRemainStable()
{
    using Button = KoDualColorButton;
    using Action = void (Button::*)();
    using Select = void (Button::*)(bool);
    static_assert(std::is_same_v<decltype(&Button::backgroundSelect), Select>);
    static_assert(std::is_same_v<decltype(&Button::foregroundSelect), Select>);
    static_assert(std::is_same_v<decltype(&Button::openBackgroundDialog), Action>);
    static_assert(std::is_same_v<decltype(&Button::openForegroundDialog), Action>);
    static_assert(std::is_same_v<decltype(&Button::popDialog), bool (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::setDisplayRenderer),
                                 void (Button::*)(const KoColorDisplayRendererInterface *)>);
    static_assert(std::is_same_v<decltype(std::declval<Button &>().setDisplayRenderer()), void>);
    static_assert(std::is_same_v<decltype(&Button::setPopDialog), void (Button::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Button::slotColorDialogClosed), Action>);
}

void KoDualColorButtonSchemaContractTest::dualColorButtonDialogResultAndNotificationSignaturesRemainStable()
{
    using Button = KoDualColorButton;
    using Notification = void (Button::*)(const KoColor &);
    using DialogResult = void (Button::*)(KoColor);
    static_assert(std::is_same_v<decltype(&Button::backgroundColorChanged), Notification>);
    static_assert(std::is_same_v<decltype(&Button::foregroundColorChanged), Notification>);
    static_assert(std::is_same_v<decltype(&Button::slotSetBackgroundColorFromDialog), DialogResult>);
    static_assert(std::is_same_v<decltype(&Button::slotSetForegroundColorFromDialog), DialogResult>);
}

QTEST_GUILESS_MAIN(KoDualColorButtonSchemaContractTest)
#include "KoDualColorButtonSchemaContractTest.moc"
