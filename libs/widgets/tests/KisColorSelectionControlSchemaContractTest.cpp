/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSelectorInterface.h"
#include "KisScreenColorSamplerBase.h"
#include "KoColorPopupAction.h"
#include "kis_color_button.h"
#include "kis_spinbox_color_selector.h"

#include <QTest>

#include <type_traits>

namespace KisColorSelectionControlContractTypes
{

class ColorSelectorInterfaceConstructionProbe final : public KisColorSelectorInterface
{
public:
    using KisColorSelectorInterface::KisColorSelectorInterface;

    KoColor getCurrentColor() const override;
    void slotSetColor(const KoColor &color) override;
};

class ScreenColorSamplerConstructionProbe final : public KisScreenColorSamplerBase
{
public:
    using KisScreenColorSamplerBase::KisScreenColorSamplerBase;

    void updateIcons() override;
};

} // namespace KisColorSelectionControlContractTypes

class KisColorSelectionControlSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorButtonTypeAndLifetimeSchemaRemainStable();
    void colorButtonColorAndPaletteSignaturesRemainStable();
    void colorButtonGeometryAndNotificationSignaturesRemainStable();
    void colorPopupActionTypeLifetimeAndColorSchemaRemainStable();
    void colorPopupActionEditingAndNotificationSignaturesRemainStable();
    void selectorInterfaceTypeAndConfigSchemaRemainStable();
    void selectorInterfaceColorAndNotificationSchemaRemainStable();
    void screenSamplerTypeAndNotificationSchemaRemainStable();
    void spinboxSelectorTypeAndAlphaSchemaRemainStable();
    void spinboxSelectorColorAndNotificationSchemaRemainStable();
};

void KisColorSelectionControlSchemaContractTest::colorButtonTypeAndLifetimeSchemaRemainStable()
{
    using Button = KisColorButton;

    static_assert(std::is_class_v<Button>);
    static_assert(std::is_base_of_v<QPushButton, Button>);
    static_assert(std::is_default_constructible_v<Button>);
    static_assert(std::is_constructible_v<Button, QWidget *>);
    static_assert(std::is_constructible_v<Button, const KoColor &>);
    static_assert(std::is_constructible_v<Button, const KoColor &, QWidget *>);
    static_assert(std::is_constructible_v<Button, const KoColor &, const KoColor &>);
    static_assert(std::is_constructible_v<Button, const KoColor &, const KoColor &, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Button>);
}

void KisColorSelectionControlSchemaContractTest::colorButtonColorAndPaletteSignaturesRemainStable()
{
    using Button = KisColorButton;

    static_assert(std::is_same_v<decltype(&Button::color), KoColor (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::defaultColor), KoColor (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::isAlphaChannelEnabled), bool (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::paletteViewEnabled), bool (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::setAlphaChannelEnabled), void (Button::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Button::setColor), void (Button::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Button::setDefaultColor), void (Button::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Button::setPaletteViewEnabled), void (Button::*)(bool)>);
}

void KisColorSelectionControlSchemaContractTest::colorButtonGeometryAndNotificationSignaturesRemainStable()
{
    using Button = KisColorButton;

    static_assert(std::is_same_v<decltype(&Button::minimumSizeHint), QSize (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::sizeHint), QSize (Button::*)() const>);
    static_assert(std::is_same_v<decltype(&Button::changed), void (Button::*)(const KoColor &)>);
}

void KisColorSelectionControlSchemaContractTest::colorPopupActionTypeLifetimeAndColorSchemaRemainStable()
{
    using Action = KoColorPopupAction;

    static_assert(std::is_class_v<Action>);
    static_assert(std::is_base_of_v<QAction, Action>);
    static_assert(std::is_default_constructible_v<Action>);
    static_assert(std::is_constructible_v<Action, QObject *>);
    static_assert(std::has_virtual_destructor_v<Action>);
    static_assert(std::is_same_v<decltype(&Action::currentColor), QColor (Action::*)() const>);
    static_assert(std::is_same_v<decltype(&Action::currentKoColor), KoColor (Action::*)() const>);
}

void KisColorSelectionControlSchemaContractTest::colorPopupActionEditingAndNotificationSignaturesRemainStable()
{
    using Action = KoColorPopupAction;
    using KoColorSetter = void (Action::*)(const KoColor &);
    using QColorSetter = void (Action::*)(const QColor &);

    static_assert(std::is_same_v<decltype(static_cast<KoColorSetter>(&Action::setCurrentColor)), KoColorSetter>);
    static_assert(std::is_same_v<decltype(static_cast<QColorSetter>(&Action::setCurrentColor)), QColorSetter>);
    static_assert(std::is_same_v<decltype(&Action::updateIcon), void (Action::*)()>);
    static_assert(std::is_same_v<decltype(&Action::colorChanged), void (Action::*)(const KoColor &)>);
}

void KisColorSelectionControlSchemaContractTest::selectorInterfaceTypeAndConfigSchemaRemainStable()
{
    using Selector = KisColorSelectorInterface;
    using ConstructionProbe = KisColorSelectionControlContractTypes::ColorSelectorInterfaceConstructionProbe;

    static_assert(std::is_class_v<Selector>);
    static_assert(std::is_base_of_v<QWidget, Selector>);
    static_assert(std::is_abstract_v<Selector>);
    static_assert(std::is_default_constructible_v<ConstructionProbe>);
    static_assert(std::is_constructible_v<ConstructionProbe, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Selector>);
    static_assert(std::is_same_v<decltype(&Selector::setConfig), void (Selector::*)(bool, bool)>);
    static_assert(std::is_same_v<decltype(&Selector::setDisplayRenderer),
                                 void (Selector::*)(const KoColorDisplayRendererInterface *)>);
}

void KisColorSelectionControlSchemaContractTest::selectorInterfaceColorAndNotificationSchemaRemainStable()
{
    using Selector = KisColorSelectorInterface;

    static_assert(std::is_same_v<decltype(&Selector::getCurrentColor), KoColor (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::sigNewColor), void (Selector::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Selector::slotSetColor), void (Selector::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Selector::slotSetColorSpace), void (Selector::*)(const KoColorSpace *)>);
}

void KisColorSelectionControlSchemaContractTest::screenSamplerTypeAndNotificationSchemaRemainStable()
{
    using Sampler = KisScreenColorSamplerBase;
    using ConstructionProbe = KisColorSelectionControlContractTypes::ScreenColorSamplerConstructionProbe;

    static_assert(std::is_class_v<Sampler>);
    static_assert(std::is_base_of_v<QWidget, Sampler>);
    static_assert(std::is_abstract_v<Sampler>);
    static_assert(std::is_default_constructible_v<ConstructionProbe>);
    static_assert(std::is_constructible_v<ConstructionProbe, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Sampler>);
    static_assert(std::is_same_v<decltype(&Sampler::updateIcons), void (Sampler::*)()>);
    static_assert(std::is_same_v<decltype(&Sampler::sigNewColorSampled), void (Sampler::*)(KoColor)>);
}

void KisColorSelectionControlSchemaContractTest::spinboxSelectorTypeAndAlphaSchemaRemainStable()
{
    using Selector = KisSpinboxColorSelector;

    static_assert(std::is_class_v<Selector>);
    static_assert(std::is_base_of_v<QWidget, Selector>);
    static_assert(std::is_constructible_v<Selector, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Selector>);
    static_assert(std::is_same_v<decltype(&Selector::chooseAlpha), void (Selector::*)(bool)>);
}

void KisColorSelectionControlSchemaContractTest::spinboxSelectorColorAndNotificationSchemaRemainStable()
{
    using Selector = KisSpinboxColorSelector;

    static_assert(std::is_same_v<decltype(&Selector::sigNewColor), void (Selector::*)(KoColor)>);
    static_assert(std::is_same_v<decltype(&Selector::slotSetColor), void (Selector::*)(KoColor)>);
    static_assert(std::is_same_v<decltype(&Selector::slotSetColorSpace), void (Selector::*)(const KoColorSpace *)>);
}

QTEST_APPLESS_MAIN(KisColorSelectionControlSchemaContractTest)

#include "KisColorSelectionControlSchemaContractTest.moc"
