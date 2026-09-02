/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisVisualColorModel.h"
#include "kis_color_input.h"

#include <QTest>

#include <type_traits>

namespace KisColorInputContractTypes
{

class ColorInputConstructionProbe final : public KisColorInput
{
public:
    using KisColorInput::KisColorInput;

protected:
    QWidget *createInput() override;
};

} // namespace KisColorInputContractTypes

class KisVisualColorModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visualColorModelTypeAndEnumerationSchemaRemainsStable();
    void visualColorModelLifetimeAndStateSchemaRemainsStable();
    void visualColorModelPolicySchemaRemainsStable();
    void visualColorModelConversionAndMutationSchemaRemainsStable();
    void visualColorModelNotificationSchemaRemainsStable();
    void colorInputTypeAndConstructionSchemaRemainsStable();
    void colorInputPercentageAndNotificationSchemaRemainsStable();
    void colorInputNumericValueSchemaRemainsStable();
    void colorInputHexValueSchemaRemainsStable();
    void colorInputHsvValueSchemaRemainsStable();
};

void KisVisualColorModelSchemaContractTest::visualColorModelTypeAndEnumerationSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_same_v<KisVisualColorModelSP, QSharedPointer<Model>>);
    static_assert(std::is_enum_v<Model::ColorModel>);
    static_assert(int(Model::None) == 0);
    static_assert(int(Model::Channel) == 1);
    static_assert(int(Model::HSV) == 2);
    static_assert(int(Model::HSL) == 3);
    static_assert(int(Model::HSI) == 4);
    static_assert(int(Model::HSY) == 5);
    static_assert(int(Model::YUV) == 6);
}

void KisVisualColorModelSchemaContractTest::visualColorModelLifetimeAndStateSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_default_constructible_v<Model>);
    static_assert(std::is_destructible_v<Model>);
    static_assert(std::is_same_v<decltype(&Model::currentColor), KoColor (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::channelValues), QVector4D (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::colorChannelCount), int (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::colorModel), Model::ColorModel (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::maxChannelValues), QVector4D (Model::*)() const>);
}

void KisVisualColorModelSchemaContractTest::visualColorModelPolicySchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_same_v<decltype(&Model::setMaxChannelValues), void (Model::*)(const QVector4D &)>);
    static_assert(std::is_same_v<decltype(&Model::copyState), void (Model::*)(const Model &)>);
    static_assert(std::is_same_v<decltype(&Model::setRGBColorModel), void (Model::*)(Model::ColorModel)>);
    static_assert(std::is_same_v<decltype(&Model::colorSpace), const KoColorSpace *(Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::isHSXModel), bool (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::supportsExposure), bool (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::slotLoadACSConfig), void (Model::*)()>);
}

void KisVisualColorModelSchemaContractTest::visualColorModelConversionAndMutationSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(
        std::is_same_v<decltype(&Model::convertChannelValuesToKoColor), KoColor (Model::*)(const QVector4D &) const>);
    static_assert(std::is_same_v<decltype(&Model::convertKoColorToChannelValues), QVector4D (Model::*)(KoColor) const>);
    static_assert(std::is_same_v<decltype(&Model::slotSetColor), void (Model::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Model::slotSetColorSpace), void (Model::*)(const KoColorSpace *)>);
    static_assert(std::is_same_v<decltype(&Model::slotSetChannelValues), void (Model::*)(const QVector4D &)>);
}

void KisVisualColorModelSchemaContractTest::visualColorModelNotificationSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_same_v<decltype(&Model::sigColorModelChanged), void (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::sigColorSpaceChanged), void (Model::*)()>);
    static_assert(
        std::is_same_v<decltype(&Model::sigChannelValuesChanged), void (Model::*)(const QVector4D &, quint32)>);
    static_assert(std::is_same_v<decltype(&Model::sigNewColor), void (Model::*)(const KoColor &)>);
}

void KisVisualColorModelSchemaContractTest::colorInputTypeAndConstructionSchemaRemainsStable()
{
    using InputProbe = KisColorInputContractTypes::ColorInputConstructionProbe;

    static_assert(std::is_class_v<KisColorInput>);
    static_assert(std::is_class_v<KisIntegerColorInput>);
    static_assert(std::is_class_v<KisFloatColorInput>);
    static_assert(std::is_class_v<KisHexColorInput>);
    static_assert(std::is_class_v<KisHsvColorInput>);
    static_assert(std::is_constructible_v<InputProbe,
                                          QWidget *,
                                          const KoChannelInfo *,
                                          KoColor *,
                                          KoColorDisplayRendererInterface *,
                                          bool>);
    static_assert(std::is_constructible_v<KisIntegerColorInput,
                                          QWidget *,
                                          const KoChannelInfo *,
                                          KoColor *,
                                          KoColorDisplayRendererInterface *,
                                          bool>);
    static_assert(std::is_constructible_v<KisFloatColorInput,
                                          QWidget *,
                                          const KoChannelInfo *,
                                          KoColor *,
                                          KoColorDisplayRendererInterface *,
                                          bool>);
    static_assert(
        std::is_constructible_v<KisHexColorInput, QWidget *, KoColor *, KoColorDisplayRendererInterface *, bool, bool>);
    static_assert(std::is_constructible_v<InputProbe, QWidget *, const KoChannelInfo *, KoColor *>);
    static_assert(std::is_constructible_v<KisIntegerColorInput, QWidget *, const KoChannelInfo *, KoColor *>);
    static_assert(std::is_constructible_v<KisFloatColorInput, QWidget *, const KoChannelInfo *, KoColor *>);
    static_assert(std::is_constructible_v<KisHexColorInput, QWidget *, KoColor *>);
    static_assert(std::is_constructible_v<KisHsvColorInput, QWidget *, KoColor *>);
}

void KisVisualColorModelSchemaContractTest::colorInputPercentageAndNotificationSchemaRemainsStable()
{
    using Input = KisColorInput;

    static_assert(std::is_same_v<decltype(&Input::setPercentageWise), void (Input::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Input::usePercentage), bool (Input::*)() const>);
    static_assert(std::is_same_v<decltype(&Input::updated), void (Input::*)()>);
}

void KisVisualColorModelSchemaContractTest::colorInputNumericValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&KisIntegerColorInput::setValue), void (KisIntegerColorInput::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisIntegerColorInput::update), void (KisIntegerColorInput::*)()>);
    static_assert(std::is_same_v<decltype(&KisFloatColorInput::setValue), void (KisFloatColorInput::*)(double)>);
    static_assert(std::is_same_v<decltype(&KisFloatColorInput::sliderChanged), void (KisFloatColorInput::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisFloatColorInput::update), void (KisFloatColorInput::*)()>);
}

void KisVisualColorModelSchemaContractTest::colorInputHexValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&KisHexColorInput::setValue), void (KisHexColorInput::*)()>);
    static_assert(std::is_same_v<decltype(&KisHexColorInput::update), void (KisHexColorInput::*)()>);
}

void KisVisualColorModelSchemaContractTest::colorInputHsvValueSchemaRemainsStable()
{
    using Input = KisHsvColorInput;

    static_assert(std::is_same_v<decltype(&Input::setMixMode), void (Input::*)(KisHsvColorSlider::MIX_MODE)>);
    static_assert(std::is_same_v<decltype(&Input::setHue), void (Input::*)(double)>);
    static_assert(std::is_same_v<decltype(&Input::setSaturation), void (Input::*)(double)>);
    static_assert(std::is_same_v<decltype(&Input::setValue), void (Input::*)(double)>);
    static_assert(std::is_same_v<decltype(&Input::hueSliderChanged), void (Input::*)(int)>);
    static_assert(std::is_same_v<decltype(&Input::saturationSliderChanged), void (Input::*)(int)>);
    static_assert(std::is_same_v<decltype(&Input::valueSliderChanged), void (Input::*)(int)>);
    static_assert(std::is_same_v<decltype(&Input::update), void (Input::*)()>);
    static_assert(std::is_same_v<decltype(&Input::updated), void (Input::*)()>);
}

QTEST_GUILESS_MAIN(KisVisualColorModelSchemaContractTest)

#include "KisVisualColorModelSchemaContractTest.moc"
