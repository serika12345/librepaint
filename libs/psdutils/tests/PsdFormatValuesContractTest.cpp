/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "psd.h"

#include <QTest>

#include <type_traits>

class PsdFormatValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileLimitsAndStorageEnumsRemainStable();
    void colorSamplerIdentifiersRemainStable();
    void layerEffectEnumsRemainStable();
    void resourceIdentityFieldsAreIndependentValues();
    void transparencyStopsPreserveSignedValues();
    void patternDefaultsMatchTheEmptyRecord();
    void patternMembersCopyAndChangeIndependently();
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

void PsdFormatValuesContractTest::resourceIdentityFieldsAreIndependentValues()
{
    const PsdResource empty;
    QVERIFY(empty.type.isEmpty());
    QVERIFY(empty.md5.isEmpty());
    QVERIFY(empty.filename.isEmpty());
    QVERIFY(empty.name.isEmpty());

    PsdResource resource;
    resource.type = QStringLiteral("patterns");
    resource.md5 = QStringLiteral("0123456789abcdef");
    resource.filename = QStringLiteral("pattern.pat");
    resource.name = QStringLiteral("Pattern");
    PsdResource copy = resource;
    QCOMPARE(copy.type, resource.type);
    QCOMPARE(copy.md5, resource.md5);
    QCOMPARE(copy.filename, resource.filename);
    QCOMPARE(copy.name, resource.name);
    copy.type = QStringLiteral("gradients");
    copy.md5.clear();
    copy.filename = QStringLiteral("gradient.ggr");
    copy.name = QStringLiteral("Gradient");
    QCOMPARE(resource.type, QStringLiteral("patterns"));
    QCOMPARE(resource.md5, QStringLiteral("0123456789abcdef"));
    QCOMPARE(resource.filename, QStringLiteral("pattern.pat"));
    QCOMPARE(resource.name, QStringLiteral("Pattern"));
}

void PsdFormatValuesContractTest::transparencyStopsPreserveSignedValues()
{
    const psd_gradient_transparency_stop empty{};
    QCOMPARE(empty.location, 0);
    QCOMPARE(empty.midpoint, 0);
    QCOMPARE(empty.opacity, 0);

    psd_gradient_transparency_stop stop{-25, 75, -12};
    psd_gradient_transparency_stop copy = stop;
    QCOMPARE(copy.location, -25);
    QCOMPARE(copy.midpoint, 75);
    QCOMPARE(copy.opacity, -12);
    copy.location = 100;
    copy.midpoint = -50;
    copy.opacity = 42;
    QCOMPARE(stop.location, -25);
    QCOMPARE(stop.midpoint, 75);
    QCOMPARE(stop.opacity, -12);
}

void PsdFormatValuesContractTest::patternDefaultsMatchTheEmptyRecord()
{
    const psd_pattern pattern;
    QCOMPARE(pattern.color_mode, Bitmap);
    QCOMPARE(pattern.height, 0);
    QCOMPARE(pattern.width, 0);
    QVERIFY(pattern.name.isEmpty());
    QVERIFY(pattern.uuid.isEmpty());
    QCOMPARE(pattern.version, 0);
    QCOMPARE(pattern.top, 0);
    QCOMPARE(pattern.left, 0);
    QCOMPARE(pattern.bottom, 0);
    QCOMPARE(pattern.right, 0);
    QCOMPARE(pattern.max_channel, 0);
    QCOMPARE(pattern.channel_number, 0);
    QVERIFY(pattern.color_table.isEmpty());
}

void PsdFormatValuesContractTest::patternMembersCopyAndChangeIndependently()
{
    psd_pattern pattern;
    pattern.color_mode = RGB;
    pattern.height = 11;
    pattern.width = 13;
    pattern.name = QStringLiteral("Dots");
    pattern.uuid = QStringLiteral("pattern-id");
    pattern.version = 2;
    pattern.top = 1;
    pattern.left = 2;
    pattern.bottom = 9;
    pattern.right = 10;
    pattern.max_channel = 4;
    pattern.channel_number = 3;
    pattern.color_table = {qRgb(1, 2, 3), qRgb(4, 5, 6)};

    psd_pattern copy = pattern;
    QCOMPARE(copy.color_mode, RGB);
    QCOMPARE(copy.height, 11);
    QCOMPARE(copy.width, 13);
    QCOMPARE(copy.name, QStringLiteral("Dots"));
    QCOMPARE(copy.uuid, QStringLiteral("pattern-id"));
    QCOMPARE(copy.version, 2);
    QCOMPARE(copy.top, 1);
    QCOMPARE(copy.left, 2);
    QCOMPARE(copy.bottom, 9);
    QCOMPARE(copy.right, 10);
    QCOMPARE(copy.max_channel, 4);
    QCOMPARE(copy.channel_number, 3);
    QCOMPARE(copy.color_table, QVector<QRgb>({qRgb(1, 2, 3), qRgb(4, 5, 6)}));

    copy.color_mode = CMYK;
    copy.height = 21;
    copy.width = 22;
    copy.name.clear();
    copy.uuid.clear();
    copy.version = 3;
    copy.top = 4;
    copy.left = 5;
    copy.bottom = 6;
    copy.right = 7;
    copy.max_channel = 8;
    copy.channel_number = 9;
    copy.color_table[0] = qRgb(7, 8, 9);
    QCOMPARE(pattern.color_mode, RGB);
    QCOMPARE(pattern.height, 11);
    QCOMPARE(pattern.width, 13);
    QCOMPARE(pattern.name, QStringLiteral("Dots"));
    QCOMPARE(pattern.uuid, QStringLiteral("pattern-id"));
    QCOMPARE(pattern.version, 2);
    QCOMPARE(pattern.top, 1);
    QCOMPARE(pattern.left, 2);
    QCOMPARE(pattern.bottom, 9);
    QCOMPARE(pattern.right, 10);
    QCOMPARE(pattern.max_channel, 4);
    QCOMPARE(pattern.channel_number, 3);
    QCOMPARE(pattern.color_table[0], qRgb(1, 2, 3));
}

QTEST_GUILESS_MAIN(PsdFormatValuesContractTest)

#include "PsdFormatValuesContractTest.moc"
