/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "compositeops/KoColorSpaceBlendingPolicy.h"

#include <KoCompositeOpIds.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QTest>

#include <type_traits>

namespace
{
struct ByteTraits {
    using channels_type = quint8;

    struct math_trait {
        static constexpr quint8 unitValue = 255;
    };
};

struct FloatTraits {
    using channels_type = float;

    struct math_trait {
        static constexpr float unitValue = 1.0f;
    };
};

const QString configKey = QStringLiteral("useSubtractiveBlendingForCmykColorSpaces");
} // namespace

class KoColorSpaceBlendingPolicyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void additivePolicyPreservesChannelValues();
    void subtractivePolicyInvertsAroundTheChannelUnit();
    void subtractiveModesPreserveTheirExactOrder();
    void subtractiveConfigurationIsReadOnce();
};

void KoColorSpaceBlendingPolicyContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry(configKey, false);
    config.sync();
}

void KoColorSpaceBlendingPolicyContractTest::cleanupTestCase()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry(configKey);
    config.sync();
}

void KoColorSpaceBlendingPolicyContractTest::additivePolicyPreservesChannelValues()
{
    using Policy = KoAdditiveBlendingPolicy<ByteTraits>;
    static_assert(std::is_same_v<Policy::channels_type, ByteTraits::channels_type>);

    for (const quint8 value : {quint8(0), quint8(17), quint8(255)}) {
        QCOMPARE(Policy::toAdditiveSpace(value), value);
        QCOMPARE(Policy::fromAdditiveSpace(value), value);
    }
}

void KoColorSpaceBlendingPolicyContractTest::subtractivePolicyInvertsAroundTheChannelUnit()
{
    using BytePolicy = KoSubtractiveBlendingPolicy<ByteTraits>;
    using FloatPolicy = KoSubtractiveBlendingPolicy<FloatTraits>;
    static_assert(std::is_same_v<BytePolicy::channels_type, ByteTraits::channels_type>);
    static_assert(std::is_same_v<FloatPolicy::channels_type, FloatTraits::channels_type>);

    QCOMPARE(BytePolicy::toAdditiveSpace(0), quint8(255));
    QCOMPARE(BytePolicy::toAdditiveSpace(17), quint8(238));
    QCOMPARE(BytePolicy::toAdditiveSpace(255), quint8(0));
    QCOMPARE(BytePolicy::fromAdditiveSpace(0), quint8(255));
    QCOMPARE(BytePolicy::fromAdditiveSpace(17), quint8(238));
    QCOMPARE(BytePolicy::fromAdditiveSpace(255), quint8(0));

    const quint8 byteValue = 73;
    QCOMPARE(BytePolicy::fromAdditiveSpace(BytePolicy::toAdditiveSpace(byteValue)), byteValue);

    QCOMPARE(FloatPolicy::toAdditiveSpace(0.25f), 0.75f);
    QCOMPARE(FloatPolicy::fromAdditiveSpace(0.75f), 0.25f);
    const float floatValue = 0.375f;
    QCOMPARE(FloatPolicy::fromAdditiveSpace(FloatPolicy::toAdditiveSpace(floatValue)), floatValue);
}

void KoColorSpaceBlendingPolicyContractTest::subtractiveModesPreserveTheirExactOrder()
{
    const QStringList expected{
        COMPOSITE_BEHIND,
        COMPOSITE_GREATER,
        COMPOSITE_OVERLAY,
        COMPOSITE_GRAIN_MERGE,
        COMPOSITE_GRAIN_EXTRACT,
        COMPOSITE_HARD_MIX,
        COMPOSITE_HARD_MIX_PHOTOSHOP,
        COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP,
        COMPOSITE_GEOMETRIC_MEAN,
        COMPOSITE_PARALLEL,
        COMPOSITE_ALLANON,
        COMPOSITE_HARD_OVERLAY,
        COMPOSITE_INTERPOLATION,
        COMPOSITE_INTERPOLATIONB,
        COMPOSITE_PENUMBRAA,
        COMPOSITE_PENUMBRAB,
        COMPOSITE_PENUMBRAC,
        COMPOSITE_PENUMBRAD,
        COMPOSITE_SCREEN,
        COMPOSITE_DODGE,
        COMPOSITE_LINEAR_DODGE,
        COMPOSITE_LIGHTEN,
        COMPOSITE_HARD_LIGHT,
        COMPOSITE_SOFT_LIGHT_IFS_ILLUSIONS,
        COMPOSITE_SOFT_LIGHT_PEGTOP_DELPHI,
        COMPOSITE_SOFT_LIGHT_SVG,
        COMPOSITE_SOFT_LIGHT_PHOTOSHOP,
        COMPOSITE_GAMMA_LIGHT,
        COMPOSITE_GAMMA_ILLUMINATION,
        COMPOSITE_VIVID_LIGHT,
        COMPOSITE_FLAT_LIGHT,
        COMPOSITE_PIN_LIGHT,
        COMPOSITE_LINEAR_LIGHT,
        COMPOSITE_PNORM_A,
        COMPOSITE_PNORM_B,
        COMPOSITE_SUPER_LIGHT,
        COMPOSITE_TINT_IFS_ILLUSIONS,
        COMPOSITE_FOG_LIGHTEN_IFS_ILLUSIONS,
        COMPOSITE_EASY_DODGE,
        COMPOSITE_BURN,
        COMPOSITE_LINEAR_BURN,
        COMPOSITE_DARKEN,
        COMPOSITE_GAMMA_DARK,
        COMPOSITE_SHADE_IFS_ILLUSIONS,
        COMPOSITE_FOG_DARKEN_IFS_ILLUSIONS,
        COMPOSITE_EASY_BURN,
        COMPOSITE_ADD,
        COMPOSITE_SUBTRACT,
        COMPOSITE_INVERSE_SUBTRACT,
        COMPOSITE_MULT,
        COMPOSITE_DIVIDE,
        COMPOSITE_MOD,
        COMPOSITE_MOD_CON,
        COMPOSITE_DIVISIVE_MOD,
        COMPOSITE_DIVISIVE_MOD_CON,
        COMPOSITE_MODULO_SHIFT,
        COMPOSITE_MODULO_SHIFT_CON,
        COMPOSITE_ARC_TANGENT,
        COMPOSITE_DIFF,
        COMPOSITE_EXCLUSION,
        COMPOSITE_EQUIVALENCE,
        COMPOSITE_ADDITIVE_SUBTRACTIVE,
        COMPOSITE_NEGATION,
        COMPOSITE_XOR,
        COMPOSITE_OR,
        COMPOSITE_AND,
        COMPOSITE_NAND,
        COMPOSITE_NOR,
        COMPOSITE_XNOR,
        COMPOSITE_IMPLICATION,
        COMPOSITE_NOT_IMPLICATION,
        COMPOSITE_CONVERSE,
        COMPOSITE_NOT_CONVERSE,
        COMPOSITE_REFLECT,
        COMPOSITE_GLOW,
        COMPOSITE_FREEZE,
        COMPOSITE_HEAT,
        COMPOSITE_GLEAT,
        COMPOSITE_HELOW,
        COMPOSITE_REEZE,
        COMPOSITE_FRECT,
        COMPOSITE_FHYRD,
        COMPOSITE_LUMINOSITY_SAI,
    };

    const QStringList actual = subtractiveBlendingModesInCmyk();
    QCOMPARE(actual, expected);
    QCOMPARE(actual.size(), 83);

    QSet<QString> uniqueModes;
    for (const QString &mode : actual) {
        uniqueModes.insert(mode);
    }
    QCOMPARE(uniqueModes.size(), actual.size());
}

void KoColorSpaceBlendingPolicyContractTest::subtractiveConfigurationIsReadOnce()
{
    QVERIFY(!useSubtractiveBlendingForCmykColorSpaces());

    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry(configKey, true);
    config.sync();

    QVERIFY(!useSubtractiveBlendingForCmykColorSpaces());
}

QTEST_GUILESS_MAIN(KoColorSpaceBlendingPolicyContractTest)

#include "KoColorSpaceBlendingPolicyContractTest.moc"
