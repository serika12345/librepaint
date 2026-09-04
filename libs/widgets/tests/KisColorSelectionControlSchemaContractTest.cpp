/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSelectorInterface.h"
#include "KisScreenColorSamplerBase.h"
#include "KisVisualColorSelector.h"
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
    void visualColorSelectorTypeLifetimeAndRenderModeSchemaRemainStable();
    void visualColorSelectorModelAndConfigurationSignaturesRemainStable();
    void visualColorSelectorColorAndRendererSignaturesRemainStable();
    void visualColorSelectorPresentationAndProofSignaturesRemainStable();
    void visualColorSelectorGamutAndNotificationSignaturesRemainStable();
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

void KisColorSelectionControlSchemaContractTest::visualColorSelectorTypeLifetimeAndRenderModeSchemaRemainStable()
{
    using Selector = KisVisualColorSelector;

    static_assert(std::is_class_v<Selector>);
    static_assert(std::is_base_of_v<KisColorSelectorInterface, Selector>);
    static_assert(std::is_enum_v<Selector::RenderMode>);
    static_assert(Selector::StaticBackground == 0);
    static_assert(Selector::DynamicBackground == 1);
    static_assert(Selector::CompositeBackground == 2);
    static_assert(std::is_default_constructible_v<Selector>);
    static_assert(std::is_constructible_v<Selector, QWidget *>);
    static_assert(std::is_constructible_v<Selector, QWidget *, KisVisualColorModelSP>);
    static_assert(std::has_virtual_destructor_v<Selector>);
    static_assert(std::is_same_v<decltype(&Selector::renderMode), Selector::RenderMode (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::setRenderMode), void (Selector::*)(Selector::RenderMode)>);
}

void KisColorSelectionControlSchemaContractTest::visualColorSelectorModelAndConfigurationSignaturesRemainStable()
{
    using Selector = KisVisualColorSelector;

    static_assert(std::is_same_v<decltype(&Selector::selectorModel), KisVisualColorModelSP (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::setSelectorModel), void (Selector::*)(KisVisualColorModelSP)>);
    static_assert(
        std::is_same_v<decltype(&Selector::configuration), const KisColorSelectorConfiguration &(Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::setConfiguration),
                                 void (Selector::*)(const KisColorSelectorConfiguration *)>);
    static_assert(std::is_same_v<decltype(&Selector::setConfig), void (Selector::*)(bool, bool)>);
    static_assert(std::is_same_v<decltype(&Selector::setAcceptTabletEvents), void (Selector::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Selector::setMinimumSliderWidth), void (Selector::*)(int)>);
}

void KisColorSelectionControlSchemaContractTest::visualColorSelectorColorAndRendererSignaturesRemainStable()
{
    using Selector = KisVisualColorSelector;

    static_assert(std::is_same_v<decltype(&Selector::getCurrentColor), KoColor (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::slotSetColor), void (Selector::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Selector::slotSetColorSpace), void (Selector::*)(const KoColorSpace *)>);
    static_assert(std::is_same_v<decltype(&Selector::displayRenderer),
                                 const KoColorDisplayRendererInterface *(Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::setDisplayRenderer),
                                 void (Selector::*)(const KoColorDisplayRendererInterface *)>);
    static_assert(std::is_same_v<decltype(&Selector::autoAdjustExposure), bool (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::setAutoAdjustExposure), void (Selector::*)(bool)>);
}

void KisColorSelectionControlSchemaContractTest::visualColorSelectorPresentationAndProofSignaturesRemainStable()
{
    using Selector = KisVisualColorSelector;

    static_assert(std::is_same_v<decltype(&Selector::minimumSizeHint), QSize (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::proofColors), bool (Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::setProofColors), void (Selector::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Selector::setSliderPosition), void (Selector::*)(Qt::Edge)>);
    static_assert(std::is_same_v<decltype(&Selector::sigInteraction), void (Selector::*)(bool)>);
}

void KisColorSelectionControlSchemaContractTest::visualColorSelectorGamutAndNotificationSignaturesRemainStable()
{
    using Selector = KisVisualColorSelector;

    static_assert(std::is_same_v<decltype(&Selector::activeGamutMask), KoGamutMask *(Selector::*)() const>);
    static_assert(std::is_same_v<decltype(&Selector::slotGamutMaskChanged), void (Selector::*)(KoGamutMaskSP)>);
    static_assert(std::is_same_v<decltype(&Selector::slotGamutMaskUnset), void (Selector::*)()>);
    static_assert(std::is_same_v<decltype(&Selector::slotGamutMaskPreviewUpdate), void (Selector::*)()>);
    static_assert(std::is_same_v<decltype(&Selector::sigGamutMaskSupportChanged), void (Selector::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Selector::slotConfigurationChanged), void (Selector::*)()>);
}

QTEST_APPLESS_MAIN(KisColorSelectionControlSchemaContractTest)

#include "KisColorSelectionControlSchemaContractTest.moc"
