/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisProofingConfiguration.h"

#include <QTest>

#include <cstring>

namespace
{

QColor observedWarningColor;
const KoColorSpace *observedWarningColorSpace = nullptr;
int warningColorConstructionCount = 0;

alignas(void *) unsigned char rgb8ColorSpaceToken;

const KoColorSpace *rgb8ColorSpace()
{
    return reinterpret_cast<const KoColorSpace *>(&rgb8ColorSpaceToken);
}

} // namespace

struct KoColorSpaceRegistry::Private {
};

KoColorSpaceRegistry::KoColorSpaceRegistry()
    : d(new Private)
{
}

KoColorSpaceRegistry::~KoColorSpaceRegistry()
{
    delete d;
}

KoColorSpaceRegistry *KoColorSpaceRegistry::instance()
{
    static KoColorSpaceRegistry registry;
    return &registry;
}

const KoColorSpace *KoColorSpaceRegistry::rgb8(const QString &)
{
    return rgb8ColorSpace();
}

KoColor::KoColor(const QColor &color, const KoColorSpace *colorSpace)
    : m_colorSpace(colorSpace)
    , m_size(sizeof(QRgb))
{
    observedWarningColor = color;
    observedWarningColorSpace = colorSpace;
    ++warningColorConstructionCount;

    const QRgb rgba = color.rgba();
    std::memcpy(m_data, &rgba, sizeof(rgba));
}

bool KoColor::operator==(const KoColor &other) const
{
    return m_colorSpace == other.m_colorSpace && m_size == other.m_size
        && std::memcmp(m_data, other.m_data, m_size) == 0;
}

class KisProofingConfigurationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultConfigurationPreservesProofingDefaultsAndEnumValues();
    void legacyAdaptationStateUsesHalfThreshold();
    void displayIntentFollowsSelectedTransform();
    void displayFlagsPreserveModeAndProofingBits();
    void equalityComparesEveryConfigurationValue();
};

void KisProofingConfigurationContractTest::defaultConfigurationPreservesProofingDefaultsAndEnumValues()
{
    QCOMPARE(static_cast<int>(KisProofingConfiguration::Monitor), 0);
    QCOMPARE(static_cast<int>(KisProofingConfiguration::Paper), 1);
    QCOMPARE(static_cast<int>(KisProofingConfiguration::Custom), 2);

    observedWarningColor = QColor();
    observedWarningColorSpace = nullptr;
    warningColorConstructionCount = 0;

    {
        KisProofingConfiguration config;

        QCOMPARE(config.conversionIntent, KoColorConversionTransformation::IntentRelativeColorimetric);
        QCOMPARE(config.displayIntent, KoColorConversionTransformation::IntentAbsoluteColorimetric);
        QVERIFY(config.useBlackPointCompensationFirstTransform);
        QCOMPARE(config.displayFlags,
                 KoColorConversionTransformation::ConversionFlags(KoColorConversionTransformation::HighQuality));
        QCOMPARE(config.proofingProfile, QStringLiteral("Chemical proof"));
        QCOMPARE(config.proofingModel, QStringLiteral("CMYKA"));
        QCOMPARE(config.proofingDepth, QStringLiteral("U8"));
        QCOMPARE(config.displayMode, KisProofingConfiguration::Paper);
        QCOMPARE(config.warningColor.colorSpace(), rgb8ColorSpace());

        QCOMPARE(warningColorConstructionCount, 1);
        QCOMPARE(observedWarningColor, QColor(Qt::green));
        QCOMPARE(observedWarningColorSpace, rgb8ColorSpace());
    }
}

void KisProofingConfigurationContractTest::legacyAdaptationStateUsesHalfThreshold()
{
    KisProofingConfiguration config;
    QCOMPARE(config.legacyAdaptationState(), 1.0);

    config.setLegacyAdaptationState(0.49);
    QCOMPARE(config.legacyAdaptationState(), 0.0);
    QVERIFY(config.displayFlags.testFlag(KoColorConversionTransformation::NoAdaptationAbsoluteIntent));

    config.setLegacyAdaptationState(0.5);
    QCOMPARE(config.legacyAdaptationState(), 1.0);
    QVERIFY(!config.displayFlags.testFlag(KoColorConversionTransformation::NoAdaptationAbsoluteIntent));

    config.setLegacyAdaptationState(1.0);
    QCOMPARE(config.legacyAdaptationState(), 1.0);
}

void KisProofingConfigurationContractTest::displayIntentFollowsSelectedTransform()
{
    KisProofingConfiguration config;

    config.displayMode = KisProofingConfiguration::Monitor;
    QCOMPARE(config.determineDisplayIntent(KoColorConversionTransformation::IntentSaturation),
             KoColorConversionTransformation::IntentSaturation);

    config.displayMode = KisProofingConfiguration::Paper;
    QCOMPARE(config.determineDisplayIntent(KoColorConversionTransformation::IntentPerceptual),
             KoColorConversionTransformation::IntentAbsoluteColorimetric);

    config.displayMode = KisProofingConfiguration::Custom;
    config.displayIntent = KoColorConversionTransformation::IntentRelativeColorimetric;
    QCOMPARE(config.determineDisplayIntent(KoColorConversionTransformation::IntentPerceptual),
             KoColorConversionTransformation::IntentRelativeColorimetric);
}

void KisProofingConfigurationContractTest::displayFlagsPreserveModeAndProofingBits()
{
    KisProofingConfiguration config;
    config.displayFlags = KoColorConversionTransformation::ConversionFlags(
        KoColorConversionTransformation::GamutCheck | KoColorConversionTransformation::SoftProofing);

    const KoColorConversionTransformation::ConversionFlags monitorFlags(
        KoColorConversionTransformation::LowQuality | KoColorConversionTransformation::BlackpointCompensation
        | KoColorConversionTransformation::NoAdaptationAbsoluteIntent);

    config.displayMode = KisProofingConfiguration::Monitor;
    KoColorConversionTransformation::ConversionFlags result = config.determineDisplayFlags(monitorFlags);
    QVERIFY(result.testFlag(KoColorConversionTransformation::LowQuality));
    QVERIFY(result.testFlag(KoColorConversionTransformation::BlackpointCompensation));
    QVERIFY(result.testFlag(KoColorConversionTransformation::GamutCheck));
    QVERIFY(result.testFlag(KoColorConversionTransformation::SoftProofing));
    QVERIFY(!result.testFlag(KoColorConversionTransformation::NoAdaptationAbsoluteIntent));

    config.displayMode = KisProofingConfiguration::Paper;
    result = config.determineDisplayFlags(monitorFlags);
    QVERIFY(result.testFlag(KoColorConversionTransformation::HighQuality));
    QVERIFY(result.testFlag(KoColorConversionTransformation::GamutCheck));
    QVERIFY(result.testFlag(KoColorConversionTransformation::SoftProofing));
    QVERIFY(!result.testFlag(KoColorConversionTransformation::LowQuality));
    QVERIFY(!result.testFlag(KoColorConversionTransformation::NoAdaptationAbsoluteIntent));

    config.displayMode = KisProofingConfiguration::Custom;
    config.displayFlags.setFlag(KoColorConversionTransformation::LowQuality);
    config.displayFlags.setFlag(KoColorConversionTransformation::NoAdaptationAbsoluteIntent);
    QCOMPARE(config.determineDisplayFlags(monitorFlags), config.displayFlags);
}

void KisProofingConfigurationContractTest::equalityComparesEveryConfigurationValue()
{
    const KisProofingConfiguration baseline;
    const KisProofingConfiguration identical;
    QVERIFY(baseline == identical);
    QVERIFY(!(baseline != identical));

    const auto verifyDifference = [&baseline](const auto &mutate) {
        KisProofingConfiguration changed(baseline);
        mutate(changed);
        QVERIFY(baseline != changed);
        QVERIFY(!(baseline == changed));
    };

    verifyDifference([](KisProofingConfiguration &config) {
        config.conversionIntent = KoColorConversionTransformation::IntentPerceptual;
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.displayIntent = KoColorConversionTransformation::IntentPerceptual;
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.useBlackPointCompensationFirstTransform = false;
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.displayFlags = KoColorConversionTransformation::LowQuality;
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.warningColor.data()[0] ^= 0xff;
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.proofingProfile = QStringLiteral("proof-profile");
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.proofingModel = QStringLiteral("RGBA");
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.proofingDepth = QStringLiteral("U16");
    });
    verifyDifference([](KisProofingConfiguration &config) {
        config.displayMode = KisProofingConfiguration::Monitor;
    });
}

QTEST_GUILESS_MAIN(KisProofingConfigurationContractTest)

#include "KisProofingConfigurationContractTest.moc"
