/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "psd_types.h"

#include <QTest>

#include <type_traits>

class PsdFormatValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileLimitsAndStorageEnumsRemainStable();
    void colorSamplerIdentifiersRemainStable();
    void layerEffectEnumsRemainStable();
};

void PsdFormatValuesContractTest::fileLimitsAndStorageEnumsRemainStable()
{
    QCOMPARE(MAX_CHANNELS, 56);
    QCOMPARE(MAX_PSD_SIZE, 30000);
    static_assert(std::is_same_v<Fixed, qint32>);

    QCOMPARE(int(psd_byte_order::psdBigEndian), 0);
    QCOMPARE(int(psd_byte_order::psdLittleEndian), 1);
    QCOMPARE(int(psd_byte_order::psdInvalidByteOrder), 255);

    QCOMPARE(int(Uncompressed), 0);
    QCOMPARE(int(RLE), 1);
    QCOMPARE(int(ZIP), 2);
    QCOMPARE(int(ZIPWithPrediction), 3);
    QCOMPARE(int(Unknown), 255);

    QCOMPARE(int(Bitmap), 0);
    QCOMPARE(int(Grayscale), 1);
    QCOMPARE(int(Indexed), 2);
    QCOMPARE(int(RGB), 3);
    QCOMPARE(int(CMYK), 4);
    QCOMPARE(int(MultiChannel), 7);
    QCOMPARE(int(DuoTone), 8);
    QCOMPARE(int(Lab), 9);
    QCOMPARE(int(Gray16), 10);
    QCOMPARE(int(RGB48), 11);
    QCOMPARE(int(Lab48), 12);
    QCOMPARE(int(CMYK64), 13);
    QCOMPARE(int(DeepMultichannel), 14);
    QCOMPARE(int(Duotone16), 15);
    QCOMPARE(int(COLORMODE_UNKNOWN), 9000);
}

void PsdFormatValuesContractTest::colorSamplerIdentifiersRemainStable()
{
    QCOMPARE(int(psd_color_sampler::RGB), 0);
    QCOMPARE(int(psd_color_sampler::HSB), 1);
    QCOMPARE(int(psd_color_sampler::CMYK), 2);
    QCOMPARE(int(psd_color_sampler::PANTONE), 3);
    QCOMPARE(int(psd_color_sampler::FOCOLTONE), 4);
    QCOMPARE(int(psd_color_sampler::TRUMATCH), 5);
    QCOMPARE(int(psd_color_sampler::TOYO), 6);
    QCOMPARE(int(psd_color_sampler::LAB), 7);
    QCOMPARE(int(psd_color_sampler::GRAYSCALE), 8);
    QCOMPARE(int(psd_color_sampler::HKS), 9);
    QCOMPARE(int(psd_color_sampler::DIC), 10);
    QCOMPARE(int(psd_color_sampler::TOTAL_INK), 11);
    QCOMPARE(int(psd_color_sampler::MONITOR_RGB), 12);
    QCOMPARE(int(psd_color_sampler::DUOTONE), 13);
    QCOMPARE(int(psd_color_sampler::OPACITY), 14);
    QCOMPARE(int(psd_color_sampler::ANPA), 3000);
}

void PsdFormatValuesContractTest::layerEffectEnumsRemainStable()
{
    QCOMPARE(int(psd_gradient_style_linear), 0);
    QCOMPARE(int(psd_gradient_style_radial), 1);
    QCOMPARE(int(psd_gradient_style_angle), 2);
    QCOMPARE(int(psd_gradient_style_reflected), 3);
    QCOMPARE(int(psd_gradient_style_diamond), 4);

    QCOMPARE(int(psd_color_stop_type_foreground_color), 0);
    QCOMPARE(int(psd_color_stop_type_background_Color), 1);
    QCOMPARE(int(psd_color_stop_type_user_stop), 2);

    QCOMPARE(int(psd_technique_softer), 0);
    QCOMPARE(int(psd_technique_precise), 1);
    QCOMPARE(int(psd_technique_slope_limit), 2);

    QCOMPARE(int(psd_stroke_outside), 0);
    QCOMPARE(int(psd_stroke_inside), 1);
    QCOMPARE(int(psd_stroke_center), 2);

    QCOMPARE(int(psd_fill_solid_color), 0);
    QCOMPARE(int(psd_fill_gradient), 1);
    QCOMPARE(int(psd_fill_pattern), 2);

    QCOMPARE(int(psd_glow_center), 0);
    QCOMPARE(int(psd_glow_edge), 1);

    QCOMPARE(int(psd_bevel_outer_bevel), 0);
    QCOMPARE(int(psd_bevel_inner_bevel), 1);
    QCOMPARE(int(psd_bevel_emboss), 2);
    QCOMPARE(int(psd_bevel_pillow_emboss), 3);
    QCOMPARE(int(psd_bevel_stroke_emboss), 4);

    QCOMPARE(int(psd_direction_up), 0);
    QCOMPARE(int(psd_direction_down), 1);

    QCOMPARE(int(psd_other), 0);
    QCOMPARE(int(psd_open_folder), 1);
    QCOMPARE(int(psd_closed_folder), 2);
    QCOMPARE(int(psd_bounding_divider), 3);
}

QTEST_GUILESS_MAIN(PsdFormatValuesContractTest)

#include "PsdFormatValuesContractTest.moc"
