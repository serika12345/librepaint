/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_hsv_adjustment.h"

#include <QTest>

#include <type_traits>

class KisHSVAdjustmentSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hsvFactoryTypeAndConstructionSchemaRemainStable();
    void hsvFactoryTransformationSignaturesRemainStable();
    void hsvCurveFactoryTypeAndConstructionSchemaRemainStable();
    void hsvCurveFactoryTransformationSignaturesRemainStable();
    void hsvCurveChannelValuesRemainStable();
};

void KisHSVAdjustmentSchemaContractTest::hsvFactoryTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisHSVAdjustmentFactory>);
    static_assert(std::is_default_constructible_v<KisHSVAdjustmentFactory>);
}

void KisHSVAdjustmentSchemaContractTest::hsvFactoryTransformationSignaturesRemainStable()
{
    using SupportedModelsSignature = QList<QPair<KoID, KoID>> (KisHSVAdjustmentFactory::*)() const;
    using CreateTransformationSignature =
        KoColorTransformation *(KisHSVAdjustmentFactory::*)(const KoColorSpace *, QHash<QString, QVariant>) const;

    static_assert(std::is_same_v<decltype(&KisHSVAdjustmentFactory::supportedModels), SupportedModelsSignature>);
    static_assert(
        std::is_same_v<decltype(&KisHSVAdjustmentFactory::createTransformation), CreateTransformationSignature>);
}

void KisHSVAdjustmentSchemaContractTest::hsvCurveFactoryTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisHSVCurveAdjustmentFactory>);
    static_assert(std::is_default_constructible_v<KisHSVCurveAdjustmentFactory>);
}

void KisHSVAdjustmentSchemaContractTest::hsvCurveFactoryTransformationSignaturesRemainStable()
{
    using SupportedModelsSignature = QList<QPair<KoID, KoID>> (KisHSVCurveAdjustmentFactory::*)() const;
    using CreateTransformationSignature =
        KoColorTransformation *(KisHSVCurveAdjustmentFactory::*)(const KoColorSpace *, QHash<QString, QVariant>) const;

    static_assert(std::is_same_v<decltype(&KisHSVCurveAdjustmentFactory::supportedModels), SupportedModelsSignature>);
    static_assert(
        std::is_same_v<decltype(&KisHSVCurveAdjustmentFactory::createTransformation), CreateTransformationSignature>);
}

void KisHSVAdjustmentSchemaContractTest::hsvCurveChannelValuesRemainStable()
{
    using ColorChannel = KisHSVCurve::ColorChannel;

    static_assert(std::is_enum_v<ColorChannel>);
    static_assert(KisHSVCurve::RGBChannel0 == 0);
    static_assert(KisHSVCurve::RGBChannel1 == 1);
    static_assert(KisHSVCurve::RGBChannel2 == 2);
    static_assert(KisHSVCurve::Alpha == 3);
    static_assert(KisHSVCurve::AllColors == 4);
    static_assert(KisHSVCurve::Hue == 5);
    static_assert(KisHSVCurve::Saturation == 6);
    static_assert(KisHSVCurve::Value == 7);
    static_assert(KisHSVCurve::ChannelCount == 8);
}

QTEST_APPLESS_MAIN(KisHSVAdjustmentSchemaContractTest)

#include "KisHSVAdjustmentSchemaContractTest.moc"
