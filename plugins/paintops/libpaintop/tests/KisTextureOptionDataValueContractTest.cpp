/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTextureOptionData.h"

#include <QTest>

#include <array>
#include <functional>

class KisTextureOptionDataValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void supportedFeatureFlagsRemainStable();
    void texturingModeValuesRemainStable();
    void defaultsDescribeDisabledMultiplication();
    void equalityObservesEveryOptionValue();
};

void KisTextureOptionDataValueContractTest::supportedFeatureFlagsRemainStable()
{
    QCOMPARE(int(None), 0x0);
    QCOMPARE(int(SupportsLightnessMode), 0x1);
    QCOMPARE(int(SupportsGradientMode), 0x2);

    const KisBrushTextureFlags flags = SupportsLightnessMode | SupportsGradientMode;
    QVERIFY(flags.testFlag(SupportsLightnessMode));
    QVERIFY(flags.testFlag(SupportsGradientMode));
    QCOMPARE(int(flags), 0x3);
}

void KisTextureOptionDataValueContractTest::texturingModeValuesRemainStable()
{
    const std::array<KisTextureOptionData::TexturingMode, 16> modes {{
        KisTextureOptionData::MULTIPLY,
        KisTextureOptionData::SUBTRACT,
        KisTextureOptionData::LIGHTNESS,
        KisTextureOptionData::GRADIENT,
        KisTextureOptionData::DARKEN,
        KisTextureOptionData::OVERLAY,
        KisTextureOptionData::COLOR_DODGE,
        KisTextureOptionData::COLOR_BURN,
        KisTextureOptionData::LINEAR_DODGE,
        KisTextureOptionData::LINEAR_BURN,
        KisTextureOptionData::HARD_MIX_PHOTOSHOP,
        KisTextureOptionData::HARD_MIX_SOFTER_PHOTOSHOP,
        KisTextureOptionData::HEIGHT,
        KisTextureOptionData::LINEAR_HEIGHT,
        KisTextureOptionData::HEIGHT_PHOTOSHOP,
        KisTextureOptionData::LINEAR_HEIGHT_PHOTOSHOP,
    }};

    for (std::size_t index = 0; index < modes.size(); ++index) {
        QCOMPARE(int(modes[index]), int(index));
    }
}

void KisTextureOptionDataValueContractTest::defaultsDescribeDisabledMultiplication()
{
    const KisTextureOptionData data;
    QCOMPARE(data.textureData, KisEmbeddedTextureData());
    QVERIFY(!data.isEnabled);
    QCOMPARE(data.scale, 1.0);
    QCOMPARE(data.brightness, 0.0);
    QCOMPARE(data.contrast, 1.0);
    QCOMPARE(data.neutralPoint, 0.5);
    QCOMPARE(data.offsetX, 0);
    QCOMPARE(data.offsetY, 0);
    QCOMPARE(data.maximumOffsetX, 0);
    QCOMPARE(data.maximumOffsetY, 0);
    QVERIFY(!data.isRandomOffsetX);
    QVERIFY(!data.isRandomOffsetY);
    QCOMPARE(data.texturingMode, KisTextureOptionData::MULTIPLY);
    QVERIFY(!data.useSoftTexturing);
    QCOMPARE(data.cutOffPolicy, 0);
    QCOMPARE(data.cutOffLeft, 0);
    QCOMPARE(data.cutOffRight, 255);
    QVERIFY(!data.invert);
    QVERIFY(!data.autoInvertOnErase);
}

void KisTextureOptionDataValueContractTest::equalityObservesEveryOptionValue()
{
    const std::array<std::function<void(KisTextureOptionData &)>, 19> mutations {{
        [](KisTextureOptionData &data) { data.textureData.name = QStringLiteral("texture"); },
        [](KisTextureOptionData &data) { data.isEnabled = true; },
        [](KisTextureOptionData &data) { data.scale = 2.0; },
        [](KisTextureOptionData &data) { data.brightness = 0.1; },
        [](KisTextureOptionData &data) { data.contrast = 0.5; },
        [](KisTextureOptionData &data) { data.neutralPoint = 0.25; },
        [](KisTextureOptionData &data) { data.offsetX = 1; },
        [](KisTextureOptionData &data) { data.offsetY = 1; },
        [](KisTextureOptionData &data) { data.maximumOffsetX = 1; },
        [](KisTextureOptionData &data) { data.maximumOffsetY = 1; },
        [](KisTextureOptionData &data) { data.isRandomOffsetX = true; },
        [](KisTextureOptionData &data) { data.isRandomOffsetY = true; },
        [](KisTextureOptionData &data) { data.texturingMode = KisTextureOptionData::SUBTRACT; },
        [](KisTextureOptionData &data) { data.useSoftTexturing = true; },
        [](KisTextureOptionData &data) { data.cutOffPolicy = 1; },
        [](KisTextureOptionData &data) { data.cutOffLeft = 1; },
        [](KisTextureOptionData &data) { data.cutOffRight = 254; },
        [](KisTextureOptionData &data) { data.invert = true; },
        [](KisTextureOptionData &data) { data.autoInvertOnErase = true; },
    }};

    const KisTextureOptionData defaults;
    QVERIFY(defaults == KisTextureOptionData());
    for (const auto &mutate : mutations) {
        KisTextureOptionData changed;
        mutate(changed);
        QVERIFY(!(defaults == changed));
    }
}

QTEST_GUILESS_MAIN(KisTextureOptionDataValueContractTest)

#include "KisTextureOptionDataValueContractTest.moc"
