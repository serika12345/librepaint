/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "psd.h"
#include "psd_additional_layer_info_block.h"
#include "psd_layer_record.h"

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
    void layerTypeValuesPreserveSerializedOrdering();
    void channelInfoDefaultsDescribeAnEmptyChannel();
    void channelInfoCopiesKeepIndependentChannelState();
    void levelRecordsValueInitializeAndCopyNumericState();
    void curveRecordsValueInitializeAndCopyTableState();
    void tonalAdjustmentRecordsPreserveDefaultsAndCopyState();
    void colorAdjustmentRecordsPreserveDefaultsAndCopyState();
    void channelMixerValueInitializesAndCopiesChannels();
    void typeFaceFixedStringsPreserveSignedMetadataAndCopyState();
    void typeStyleDefaultsAndSignedMetricsCopyIndependently();
    void typeLineDefaultsAndSignedCodesCopyIndependently();
    void typeToolTransformAndPlacementValuesCopyIndependently();
    void typeShapeSettersAndValueCopiesRemainIndependent();
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

void PsdFormatValuesContractTest::layerTypeValuesPreserveSerializedOrdering()
{
    QCOMPARE(int(psd_layer_type_normal), 0);
    QCOMPARE(int(psd_layer_type_hidden), 1);
    QCOMPARE(int(psd_layer_type_folder), 2);
    QCOMPARE(int(psd_layer_type_solid_color), 3);
    QCOMPARE(int(psd_layer_type_gradient_fill), 4);
    QCOMPARE(int(psd_layer_type_pattern_fill), 5);
    QCOMPARE(int(psd_layer_type_levels), 6);
    QCOMPARE(int(psd_layer_type_curves), 7);
    QCOMPARE(int(psd_layer_type_brightness_contrast), 8);
    QCOMPARE(int(psd_layer_type_color_balance), 9);
    QCOMPARE(int(psd_layer_type_hue_saturation), 10);
    QCOMPARE(int(psd_layer_type_selective_color), 11);
    QCOMPARE(int(psd_layer_type_threshold), 12);
    QCOMPARE(int(psd_layer_type_invert), 13);
    QCOMPARE(int(psd_layer_type_posterize), 14);
    QCOMPARE(int(psd_layer_type_channel_mixer), 15);
    QCOMPARE(int(psd_layer_type_gradient_map), 16);
    QCOMPARE(int(psd_layer_type_photo_filter), 17);
}

void PsdFormatValuesContractTest::channelInfoDefaultsDescribeAnEmptyChannel()
{
    const ChannelInfo channel;
    QCOMPARE(channel.channelId, 0);
    QCOMPARE(channel.compressionType, psd_compression_type::Unknown);
    QCOMPARE(channel.channelDataStart, 0);
    QCOMPARE(channel.channelDataLength, 0);
    QVERIFY(channel.rleRowLengths.isEmpty());
    QCOMPARE(channel.channelOffset, 0);
    QCOMPARE(channel.channelInfoPosition, 0);
}

void PsdFormatValuesContractTest::channelInfoCopiesKeepIndependentChannelState()
{
    ChannelInfo channel;
    channel.channelId = -2;
    channel.compressionType = psd_compression_type::RLE;
    channel.channelDataStart = quint64(1) << 40;
    channel.channelDataLength = (quint64(1) << 39) + 17;
    channel.rleRowLengths = {3, 5, 8};
    channel.channelOffset = 23;
    channel.channelInfoPosition = 29;

    ChannelInfo copy = channel;
    QCOMPARE(copy.channelId, -2);
    QCOMPARE(copy.compressionType, psd_compression_type::RLE);
    QCOMPARE(copy.channelDataStart, quint64(1) << 40);
    QCOMPARE(copy.channelDataLength, (quint64(1) << 39) + 17);
    QCOMPARE(copy.rleRowLengths, QVector<quint32>({3, 5, 8}));
    QCOMPARE(copy.channelOffset, 23);
    QCOMPARE(copy.channelInfoPosition, 29);

    copy.channelId = 4;
    copy.compressionType = psd_compression_type::ZIP;
    copy.channelDataStart = 31;
    copy.channelDataLength = 37;
    copy.rleRowLengths[0] = 41;
    copy.channelOffset = 43;
    copy.channelInfoPosition = 47;

    QCOMPARE(channel.channelId, -2);
    QCOMPARE(channel.compressionType, psd_compression_type::RLE);
    QCOMPARE(channel.channelDataStart, quint64(1) << 40);
    QCOMPARE(channel.channelDataLength, (quint64(1) << 39) + 17);
    QCOMPARE(channel.rleRowLengths, QVector<quint32>({3, 5, 8}));
    QCOMPARE(channel.channelOffset, 23);
    QCOMPARE(channel.channelInfoPosition, 29);
}

void PsdFormatValuesContractTest::levelRecordsValueInitializeAndCopyNumericState()
{
    const psd_layer_level_record emptyRecord{};
    QCOMPARE(emptyRecord.input_floor, 0);
    QCOMPARE(emptyRecord.input_ceiling, 2);
    QCOMPARE(emptyRecord.output_floor, 0);
    QCOMPARE(emptyRecord.output_ceiling, 0);
    QCOMPARE(emptyRecord.gamma, 0.0f);

    psd_layer_level_record record{};
    record.input_floor = 17;
    record.input_ceiling = 211;
    record.output_floor = 23;
    record.output_ceiling = 197;
    record.gamma = 2.25f;
    psd_layer_level_record recordCopy = record;
    QCOMPARE(recordCopy.input_floor, 17);
    QCOMPARE(recordCopy.input_ceiling, 211);
    QCOMPARE(recordCopy.output_floor, 23);
    QCOMPARE(recordCopy.output_ceiling, 197);
    QCOMPARE(recordCopy.gamma, 2.25f);
    recordCopy.input_floor = 31;
    recordCopy.input_ceiling = 191;
    recordCopy.output_floor = 37;
    recordCopy.output_ceiling = 181;
    recordCopy.gamma = 0.75f;
    QCOMPARE(record.input_floor, 17);
    QCOMPARE(record.input_ceiling, 211);
    QCOMPARE(record.output_floor, 23);
    QCOMPARE(record.output_ceiling, 197);
    QCOMPARE(record.gamma, 2.25f);

    const psd_layer_levels emptyLevels{};
    QCOMPARE(emptyLevels.record[0].input_ceiling, 2);
    QCOMPARE(emptyLevels.record[28].input_ceiling, 2);
    QCOMPARE(emptyLevels.extra_level_count, 0);
    QCOMPARE(emptyLevels.extra_record, nullptr);
    QCOMPARE(emptyLevels.lookup_table[0][0], 0);
    QCOMPARE(emptyLevels.lookup_table[2][255], 0);

    psd_layer_level_record extraRecords[2]{};
    extraRecords[0].gamma = 1.5f;
    psd_layer_levels levels{};
    levels.record[28].input_floor = 43;
    levels.extra_level_count = 2;
    levels.extra_record = extraRecords;
    levels.lookup_table[2][255] = 239;
    psd_layer_levels levelsCopy = levels;
    QCOMPARE(levelsCopy.record[28].input_floor, 43);
    QCOMPARE(levelsCopy.extra_level_count, 2);
    QCOMPARE(levelsCopy.extra_record, extraRecords);
    QCOMPARE(levelsCopy.extra_record[0].gamma, 1.5f);
    QCOMPARE(levelsCopy.lookup_table[2][255], 239);
    levelsCopy.record[28].input_floor = 47;
    levelsCopy.lookup_table[2][255] = 241;
    QCOMPARE(levels.record[28].input_floor, 43);
    QCOMPARE(levels.lookup_table[2][255], 239);
}

void PsdFormatValuesContractTest::curveRecordsValueInitializeAndCopyTableState()
{
    const psd_layer_curves_data emptyData{};
    QCOMPARE(emptyData.channel_index, 0);
    QCOMPARE(emptyData.point_count, 0);
    QCOMPARE(emptyData.output_value[0], 0);
    QCOMPARE(emptyData.output_value[18], 0);
    QCOMPARE(emptyData.input_value[0], 0);
    QCOMPARE(emptyData.input_value[18], 0);

    psd_layer_curves_data data{};
    data.channel_index = 7;
    data.point_count = 19;
    data.output_value[18] = 251;
    data.input_value[18] = 239;
    psd_layer_curves_data dataCopy = data;
    QCOMPARE(dataCopy.channel_index, 7);
    QCOMPARE(dataCopy.point_count, 19);
    QCOMPARE(dataCopy.output_value[18], 251);
    QCOMPARE(dataCopy.input_value[18], 239);
    dataCopy.output_value[18] = 227;
    dataCopy.input_value[18] = 223;
    QCOMPARE(data.channel_index, 7);
    QCOMPARE(data.point_count, 19);
    QCOMPARE(data.output_value[18], 251);
    QCOMPARE(data.input_value[18], 239);

    const psd_layer_curves emptyCurves{};
    QCOMPARE(emptyCurves.curve_count, 0);
    QCOMPARE(emptyCurves.curve, nullptr);
    QCOMPARE(emptyCurves.lookup_table[0][0], 0);
    QCOMPARE(emptyCurves.lookup_table[2][255], 0);

    psd_layer_curves_data curveRecords[2]{};
    curveRecords[1].point_count = 5;
    psd_layer_curves curves{};
    curves.curve_count = 2;
    curves.curve = curveRecords;
    curves.lookup_table[1][127] = 131;
    psd_layer_curves curvesCopy = curves;
    QCOMPARE(curvesCopy.curve_count, 2);
    QCOMPARE(curvesCopy.curve, curveRecords);
    QCOMPARE(curvesCopy.curve[1].point_count, 5);
    QCOMPARE(curvesCopy.lookup_table[1][127], 131);
    curvesCopy.lookup_table[1][127] = 137;
    QCOMPARE(curves.curve_count, 2);
    QCOMPARE(curves.lookup_table[1][127], 131);
}

void PsdFormatValuesContractTest::tonalAdjustmentRecordsPreserveDefaultsAndCopyState()
{
    const psd_layer_brightness_contrast emptyBrightness{};
    QCOMPARE(int(emptyBrightness.brightness), 0);
    QCOMPARE(int(emptyBrightness.contrast), 0);
    QCOMPARE(int(emptyBrightness.mean_value), 0);
    QCOMPARE(int(emptyBrightness.Lab_color), 0);
    QCOMPARE(emptyBrightness.lookup_table[255], 0);

    psd_layer_brightness_contrast brightness{};
    brightness.brightness = -63;
    brightness.contrast = 71;
    brightness.mean_value = -17;
    brightness.Lab_color = 1;
    brightness.lookup_table[255] = 253;
    psd_layer_brightness_contrast brightnessCopy = brightness;
    QCOMPARE(int(brightnessCopy.brightness), -63);
    QCOMPARE(int(brightnessCopy.contrast), 71);
    QCOMPARE(int(brightnessCopy.mean_value), -17);
    QCOMPARE(int(brightnessCopy.Lab_color), 1);
    QCOMPARE(brightnessCopy.lookup_table[255], 253);
    brightnessCopy.brightness = 19;
    brightnessCopy.lookup_table[255] = 251;
    QCOMPARE(int(brightness.brightness), -63);
    QCOMPARE(int(brightness.contrast), 71);
    QCOMPARE(int(brightness.mean_value), -17);
    QCOMPARE(int(brightness.Lab_color), 1);
    QCOMPARE(brightness.lookup_table[255], 253);

    const psd_layer_hue_saturation emptyHue{};
    QCOMPARE(emptyHue.hue_or_colorization, 0);
    QCOMPARE(int(emptyHue.colorization_hue), 0);
    QCOMPARE(int(emptyHue.colorization_saturation), 0);
    QCOMPARE(int(emptyHue.colorization_lightness), 0);
    QCOMPARE(int(emptyHue.master_hue), 0);
    QCOMPARE(int(emptyHue.master_saturation), 0);
    QCOMPARE(int(emptyHue.master_lightness), 0);
    QCOMPARE(int(emptyHue.range_values[5][3]), 0);
    QCOMPARE(int(emptyHue.setting_values[5][2]), 0);
    QCOMPARE(emptyHue.lookup_table[5][359], 0);

    psd_layer_hue_saturation hue{};
    hue.hue_or_colorization = 1;
    hue.colorization_hue = -91;
    hue.colorization_saturation = 77;
    hue.colorization_lightness = -43;
    hue.master_hue = 29;
    hue.master_saturation = -31;
    hue.master_lightness = 37;
    hue.range_values[5][3] = -59;
    hue.setting_values[5][2] = 61;
    hue.lookup_table[5][359] = 251;
    psd_layer_hue_saturation hueCopy = hue;
    QCOMPARE(hueCopy.hue_or_colorization, 1);
    QCOMPARE(int(hueCopy.colorization_hue), -91);
    QCOMPARE(int(hueCopy.colorization_saturation), 77);
    QCOMPARE(int(hueCopy.colorization_lightness), -43);
    QCOMPARE(int(hueCopy.master_hue), 29);
    QCOMPARE(int(hueCopy.master_saturation), -31);
    QCOMPARE(int(hueCopy.master_lightness), 37);
    QCOMPARE(int(hueCopy.range_values[5][3]), -59);
    QCOMPARE(int(hueCopy.setting_values[5][2]), 61);
    QCOMPARE(hueCopy.lookup_table[5][359], 251);
    hueCopy.range_values[5][3] = 67;
    hueCopy.setting_values[5][2] = -71;
    hueCopy.lookup_table[5][359] = 241;
    QCOMPARE(hue.hue_or_colorization, 1);
    QCOMPARE(int(hue.colorization_hue), -91);
    QCOMPARE(int(hue.colorization_saturation), 77);
    QCOMPARE(int(hue.colorization_lightness), -43);
    QCOMPARE(int(hue.master_hue), 29);
    QCOMPARE(int(hue.master_saturation), -31);
    QCOMPARE(int(hue.master_lightness), 37);
    QCOMPARE(int(hue.range_values[5][3]), -59);
    QCOMPARE(int(hue.setting_values[5][2]), 61);
    QCOMPARE(hue.lookup_table[5][359], 251);

    const psd_layer_threshold emptyThreshold{};
    QCOMPARE(emptyThreshold.level, 1);
    psd_layer_threshold threshold{};
    threshold.level = 127;
    psd_layer_threshold thresholdCopy = threshold;
    QCOMPARE(thresholdCopy.level, 127);
    thresholdCopy.level = 191;
    QCOMPARE(threshold.level, 127);
    const psd_layer_posterize emptyPosterize{};
    QCOMPARE(emptyPosterize.levels, 2);
    QCOMPARE(emptyPosterize.lookup_table[255], 0);

    psd_layer_posterize posterize{};
    posterize.levels = 17;
    posterize.lookup_table[255] = 199;
    psd_layer_posterize posterizeCopy = posterize;
    QCOMPARE(posterizeCopy.levels, 17);
    QCOMPARE(posterizeCopy.lookup_table[255], 199);
    posterizeCopy.levels = 23;
    posterizeCopy.lookup_table[255] = 197;
    QCOMPARE(posterize.levels, 17);
    QCOMPARE(posterize.lookup_table[255], 199);
}

void PsdFormatValuesContractTest::colorAdjustmentRecordsPreserveDefaultsAndCopyState()
{
    const psd_layer_color_balance emptyBalance{};
    QCOMPARE(int(emptyBalance.cyan_red[2]), 0);
    QCOMPARE(int(emptyBalance.magenta_green[2]), 0);
    QCOMPARE(int(emptyBalance.yellow_blue[2]), 0);
    QVERIFY(!emptyBalance.preserve_luminosity);
    QCOMPARE(emptyBalance.lookup_table[2][255], 0);

    psd_layer_color_balance balance{};
    balance.cyan_red[2] = -79;
    balance.magenta_green[2] = 83;
    balance.yellow_blue[2] = -89;
    balance.preserve_luminosity = true;
    balance.lookup_table[2][255] = 251;
    psd_layer_color_balance balanceCopy = balance;
    QCOMPARE(int(balanceCopy.cyan_red[2]), -79);
    QCOMPARE(int(balanceCopy.magenta_green[2]), 83);
    QCOMPARE(int(balanceCopy.yellow_blue[2]), -89);
    QVERIFY(balanceCopy.preserve_luminosity);
    QCOMPARE(balanceCopy.lookup_table[2][255], 251);
    balanceCopy.cyan_red[2] = 73;
    balanceCopy.magenta_green[2] = -71;
    balanceCopy.yellow_blue[2] = 67;
    balanceCopy.preserve_luminosity = false;
    balanceCopy.lookup_table[2][255] = 241;
    QCOMPARE(int(balance.cyan_red[2]), -79);
    QCOMPARE(int(balance.magenta_green[2]), 83);
    QCOMPARE(int(balance.yellow_blue[2]), -89);
    QVERIFY(balance.preserve_luminosity);
    QCOMPARE(balance.lookup_table[2][255], 251);

    const psd_layer_selective_color emptySelective{};
    QCOMPARE(emptySelective.correction_method, 0);
    QCOMPARE(int(emptySelective.cyan_correction[9]), 0);
    QCOMPARE(int(emptySelective.magenta_correction[9]), 0);
    QCOMPARE(int(emptySelective.yellow_correction[9]), 0);
    QCOMPARE(int(emptySelective.black_correction[9]), 0);

    psd_layer_selective_color selective{};
    selective.correction_method = 1;
    selective.cyan_correction[9] = -97;
    selective.magenta_correction[9] = 89;
    selective.yellow_correction[9] = -83;
    selective.black_correction[9] = 79;
    psd_layer_selective_color selectiveCopy = selective;
    QCOMPARE(selectiveCopy.correction_method, 1);
    QCOMPARE(int(selectiveCopy.cyan_correction[9]), -97);
    QCOMPARE(int(selectiveCopy.magenta_correction[9]), 89);
    QCOMPARE(int(selectiveCopy.yellow_correction[9]), -83);
    QCOMPARE(int(selectiveCopy.black_correction[9]), 79);
    selectiveCopy.cyan_correction[9] = 73;
    selectiveCopy.magenta_correction[9] = -71;
    selectiveCopy.yellow_correction[9] = 67;
    selectiveCopy.black_correction[9] = -61;
    QCOMPARE(selective.correction_method, 1);
    QCOMPARE(int(selective.cyan_correction[9]), -97);
    QCOMPARE(int(selective.magenta_correction[9]), 89);
    QCOMPARE(int(selective.yellow_correction[9]), -83);
    QCOMPARE(int(selective.black_correction[9]), 79);

    const psd_layer_photo_filter emptyPhoto{};
    QCOMPARE(emptyPhoto.x_color, 0);
    QCOMPARE(emptyPhoto.y_color, 0);
    QCOMPARE(emptyPhoto.z_color, 0);
    QCOMPARE(emptyPhoto.density, 0);
    QVERIFY(emptyPhoto.preserve_luminosity);

    psd_layer_photo_filter photo{};
    photo.x_color = -100000;
    photo.y_color = 200000;
    photo.z_color = -300000;
    photo.density = 85;
    photo.preserve_luminosity = false;
    psd_layer_photo_filter photoCopy = photo;
    QCOMPARE(photoCopy.x_color, -100000);
    QCOMPARE(photoCopy.y_color, 200000);
    QCOMPARE(photoCopy.z_color, -300000);
    QCOMPARE(photoCopy.density, 85);
    QVERIFY(!photoCopy.preserve_luminosity);
    photoCopy.x_color = 400000;
    photoCopy.y_color = -500000;
    photoCopy.z_color = 600000;
    photoCopy.density = 25;
    photoCopy.preserve_luminosity = true;
    QCOMPARE(photo.x_color, -100000);
    QCOMPARE(photo.y_color, 200000);
    QCOMPARE(photo.z_color, -300000);
    QCOMPARE(photo.density, 85);
    QVERIFY(!photo.preserve_luminosity);
}

void PsdFormatValuesContractTest::channelMixerValueInitializesAndCopiesChannels()
{
    const psd_layer_channel_mixer empty{};
    QVERIFY(!empty.monochrome);
    QCOMPARE(int(empty.red_cyan[3]), 0);
    QCOMPARE(int(empty.green_magenta[3]), 0);
    QCOMPARE(int(empty.blue_yellow[3]), 0);
    QCOMPARE(int(empty.black[3]), 0);
    QCOMPARE(int(empty.constant[3]), 0);

    psd_layer_channel_mixer mixer{};
    mixer.monochrome = true;
    mixer.red_cyan[3] = -101;
    mixer.green_magenta[3] = 103;
    mixer.blue_yellow[3] = -107;
    mixer.black[3] = 109;
    mixer.constant[3] = -113;
    psd_layer_channel_mixer copy = mixer;
    QVERIFY(copy.monochrome);
    QCOMPARE(int(copy.red_cyan[3]), -101);
    QCOMPARE(int(copy.green_magenta[3]), 103);
    QCOMPARE(int(copy.blue_yellow[3]), -107);
    QCOMPARE(int(copy.black[3]), 109);
    QCOMPARE(int(copy.constant[3]), -113);
    copy.monochrome = false;
    copy.red_cyan[3] = 97;
    copy.green_magenta[3] = -89;
    copy.blue_yellow[3] = 83;
    copy.black[3] = -79;
    copy.constant[3] = 73;
    QVERIFY(mixer.monochrome);
    QCOMPARE(int(mixer.red_cyan[3]), -101);
    QCOMPARE(int(mixer.green_magenta[3]), 103);
    QCOMPARE(int(mixer.blue_yellow[3]), -107);
    QCOMPARE(int(mixer.black[3]), 109);
    QCOMPARE(int(mixer.constant[3]), -113);
}

void PsdFormatValuesContractTest::typeFaceFixedStringsPreserveSignedMetadataAndCopyState()
{
    const psd_layer_type_face empty{};
    QCOMPARE(int(empty.mark), 0);
    QCOMPARE(empty.font_type, 0);
    QCOMPARE(int(empty.font_name[0]), 0);
    QCOMPARE(int(empty.font_name[255]), 0);
    QCOMPARE(int(empty.font_family_name[0]), 0);
    QCOMPARE(int(empty.font_family_name[255]), 0);
    QCOMPARE(int(empty.font_style_name[0]), 0);
    QCOMPARE(int(empty.font_style_name[255]), 0);
    QCOMPARE(int(empty.script), 0);
    QCOMPARE(empty.number_axes_vector, 0);

    psd_layer_type_face face{};
    face.mark = -7;
    face.font_type = -123456;
    face.font_name[0] = 17;
    face.font_name[255] = -19;
    face.font_family_name[0] = 23;
    face.font_family_name[255] = -29;
    face.font_style_name[0] = 31;
    face.font_style_name[255] = -37;
    face.script = -41;
    face.number_axes_vector = 43;

    psd_layer_type_face copy = face;
    QCOMPARE(int(copy.mark), -7);
    QCOMPARE(copy.font_type, -123456);
    QCOMPARE(int(copy.font_name[0]), 17);
    QCOMPARE(int(copy.font_name[255]), -19);
    QCOMPARE(int(copy.font_family_name[0]), 23);
    QCOMPARE(int(copy.font_family_name[255]), -29);
    QCOMPARE(int(copy.font_style_name[0]), 31);
    QCOMPARE(int(copy.font_style_name[255]), -37);
    QCOMPARE(int(copy.script), -41);
    QCOMPARE(copy.number_axes_vector, 43);

    copy.mark = 47;
    copy.font_type = 654321;
    copy.font_name[0] = -53;
    copy.font_name[255] = 59;
    copy.font_family_name[0] = -61;
    copy.font_family_name[255] = 67;
    copy.font_style_name[0] = -71;
    copy.font_style_name[255] = 73;
    copy.script = 79;
    copy.number_axes_vector = 83;

    QCOMPARE(int(face.mark), -7);
    QCOMPARE(face.font_type, -123456);
    QCOMPARE(int(face.font_name[0]), 17);
    QCOMPARE(int(face.font_name[255]), -19);
    QCOMPARE(int(face.font_family_name[0]), 23);
    QCOMPARE(int(face.font_family_name[255]), -29);
    QCOMPARE(int(face.font_style_name[0]), 31);
    QCOMPARE(int(face.font_style_name[255]), -37);
    QCOMPARE(int(face.script), -41);
    QCOMPARE(face.number_axes_vector, 43);
}

void PsdFormatValuesContractTest::typeStyleDefaultsAndSignedMetricsCopyIndependently()
{
    const psd_layer_type_style empty{};
    QCOMPARE(int(empty.mark), 0);
    QCOMPARE(int(empty.face_mark), 0);
    QCOMPARE(empty.size, 0);
    QCOMPARE(empty.tracking, 0);
    QCOMPARE(empty.kerning, 0);
    QCOMPARE(empty.leading, 0);
    QCOMPARE(empty.base_shift, 0);
    QVERIFY(!empty.auto_kern);
    QVERIFY(!empty.rotate);

    psd_layer_type_style style{};
    style.mark = -5;
    style.face_mark = 7;
    style.size = 72000;
    style.tracking = -125;
    style.kerning = -250;
    style.leading = 144000;
    style.base_shift = -36000;
    style.auto_kern = true;
    style.rotate = true;

    psd_layer_type_style copy = style;
    QCOMPARE(int(copy.mark), -5);
    QCOMPARE(int(copy.face_mark), 7);
    QCOMPARE(copy.size, 72000);
    QCOMPARE(copy.tracking, -125);
    QCOMPARE(copy.kerning, -250);
    QCOMPARE(copy.leading, 144000);
    QCOMPARE(copy.base_shift, -36000);
    QVERIFY(copy.auto_kern);
    QVERIFY(copy.rotate);

    copy.mark = 11;
    copy.face_mark = -13;
    copy.size = 18000;
    copy.tracking = 375;
    copy.kerning = 500;
    copy.leading = 90000;
    copy.base_shift = 12000;
    copy.auto_kern = false;
    copy.rotate = false;

    QCOMPARE(int(style.mark), -5);
    QCOMPARE(int(style.face_mark), 7);
    QCOMPARE(style.size, 72000);
    QCOMPARE(style.tracking, -125);
    QCOMPARE(style.kerning, -250);
    QCOMPARE(style.leading, 144000);
    QCOMPARE(style.base_shift, -36000);
    QVERIFY(style.auto_kern);
    QVERIFY(style.rotate);
}

void PsdFormatValuesContractTest::typeLineDefaultsAndSignedCodesCopyIndependently()
{
    const psd_layer_type_line empty{};
    QCOMPARE(empty.char_count, 0);
    QCOMPARE(int(empty.orientation), 0);
    QCOMPARE(int(empty.alignment), 0);
    QCOMPARE(int(empty.actual_char), 0);
    QCOMPARE(int(empty.style), 0);

    psd_layer_type_line line{};
    line.char_count = 257;
    line.orientation = -3;
    line.alignment = 5;
    line.actual_char = -7;
    line.style = 11;

    psd_layer_type_line copy = line;
    QCOMPARE(copy.char_count, 257);
    QCOMPARE(int(copy.orientation), -3);
    QCOMPARE(int(copy.alignment), 5);
    QCOMPARE(int(copy.actual_char), -7);
    QCOMPARE(int(copy.style), 11);

    copy.char_count = 509;
    copy.orientation = 13;
    copy.alignment = -17;
    copy.actual_char = 19;
    copy.style = -23;

    QCOMPARE(line.char_count, 257);
    QCOMPARE(int(line.orientation), -3);
    QCOMPARE(int(line.alignment), 5);
    QCOMPARE(int(line.actual_char), -7);
    QCOMPARE(int(line.style), 11);
}

void PsdFormatValuesContractTest::typeToolTransformAndPlacementValuesCopyIndependently()
{
    const psd_layer_type_tool empty{};
    QCOMPARE(empty.transform_info[0], 0.0);
    QCOMPARE(empty.transform_info[5], 0.0);
    QCOMPARE(int(empty.faces_count), 0);
    QCOMPARE(int(empty.styles_count), 0);
    QCOMPARE(int(empty.type), 0);
    QCOMPARE(empty.scaling_factor, 0);
    QCOMPARE(empty.character_count, 0);
    QCOMPARE(empty.horz_place, 0);
    QCOMPARE(empty.vert_place, 0);
    QCOMPARE(empty.select_start, 0);
    QCOMPARE(empty.select_end, 0);
    QCOMPARE(int(empty.lines_count), 0);
    QVERIFY(!empty.color.isValid());
    QVERIFY(!empty.anti_alias);

    psd_layer_type_tool tool{};
    tool.transform_info[0] = -1.25;
    tool.transform_info[5] = 3.75;
    tool.faces_count = 3;
    tool.styles_count = 5;
    tool.type = -7;
    tool.scaling_factor = -65536;
    tool.character_count = 1024;
    tool.horz_place = -2048;
    tool.vert_place = 4096;
    tool.select_start = -11;
    tool.select_end = 997;
    tool.lines_count = 13;
    tool.color = QColor(17, 23, 31, 47);
    tool.anti_alias = true;

    psd_layer_type_tool copy = tool;
    QCOMPARE(copy.transform_info[0], -1.25);
    QCOMPARE(copy.transform_info[5], 3.75);
    QCOMPARE(int(copy.faces_count), 3);
    QCOMPARE(int(copy.styles_count), 5);
    QCOMPARE(int(copy.type), -7);
    QCOMPARE(copy.scaling_factor, -65536);
    QCOMPARE(copy.character_count, 1024);
    QCOMPARE(copy.horz_place, -2048);
    QCOMPARE(copy.vert_place, 4096);
    QCOMPARE(copy.select_start, -11);
    QCOMPARE(copy.select_end, 997);
    QCOMPARE(int(copy.lines_count), 13);
    QCOMPARE(copy.color, QColor(17, 23, 31, 47));
    QVERIFY(copy.anti_alias);

    copy.transform_info[0] = 5.5;
    copy.transform_info[5] = -7.25;
    copy.faces_count = 17;
    copy.styles_count = 19;
    copy.type = 23;
    copy.scaling_factor = 32768;
    copy.character_count = 2048;
    copy.horz_place = 8192;
    copy.vert_place = -16384;
    copy.select_start = 29;
    copy.select_end = 31;
    copy.lines_count = 37;
    copy.color = QColor(53, 59, 61, 67);
    copy.anti_alias = false;

    QCOMPARE(tool.transform_info[0], -1.25);
    QCOMPARE(tool.transform_info[5], 3.75);
    QCOMPARE(int(tool.faces_count), 3);
    QCOMPARE(int(tool.styles_count), 5);
    QCOMPARE(int(tool.type), -7);
    QCOMPARE(tool.scaling_factor, -65536);
    QCOMPARE(tool.character_count, 1024);
    QCOMPARE(tool.horz_place, -2048);
    QCOMPARE(tool.vert_place, 4096);
    QCOMPARE(tool.select_start, -11);
    QCOMPARE(tool.select_end, 997);
    QCOMPARE(int(tool.lines_count), 13);
    QCOMPARE(tool.color, QColor(17, 23, 31, 47));
    QVERIFY(tool.anti_alias);
}

void PsdFormatValuesContractTest::typeShapeSettersAndValueCopiesRemainIndependent()
{
    const psd_layer_type_shape empty{};
    QCOMPARE(empty.transform, QTransform());
    QVERIFY(empty.engineData.isEmpty());
    QCOMPARE(empty.bounds, QRectF());
    QCOMPARE(empty.boundingBox, QRectF());
    QCOMPARE(empty.textIndex, 0);
    QVERIFY(empty.text.isEmpty());
    QVERIFY(empty.isHorizontal);

    psd_layer_type_shape shape{};
    shape.transform.translate(-13.5, 17.25);
    shape.engineData.insert(QStringLiteral("language"), QStringLiteral("ja"));
    shape.boundingBox = QRectF(-5.0, -7.0, 19.0, 23.0);
    shape.text = QStringLiteral("LibrePaint text");
    shape.setIndex(-29);
    shape.setLeft(-31.5f);
    shape.setTop(-37.25f);
    shape.setRight(41.75f);
    shape.setBottom(43.5f);
    shape.setWritingMode(QStringLiteral("Vrtc"));
    QVERIFY(!shape.isHorizontal);
    shape.setWritingMode(QStringLiteral("Hrzn"));
    QVERIFY(shape.isHorizontal);
    shape.setWritingMode(QStringLiteral("Vrtc"));

    psd_layer_type_shape copy = shape;
    QCOMPARE(copy.transform, shape.transform);
    QCOMPARE(copy.engineData.value(QStringLiteral("language")).toString(), QStringLiteral("ja"));
    QCOMPARE(copy.bounds, QRectF(-31.5, -37.25, 73.25, 80.75));
    QCOMPARE(copy.boundingBox, QRectF(-5.0, -7.0, 19.0, 23.0));
    QCOMPARE(copy.textIndex, -29);
    QCOMPARE(copy.text, QStringLiteral("LibrePaint text"));
    QVERIFY(!copy.isHorizontal);

    copy.transform.scale(2.0, 3.0);
    copy.engineData.insert(QStringLiteral("language"), QStringLiteral("fr"));
    copy.bounds = QRectF(1.0, 2.0, 3.0, 4.0);
    copy.boundingBox = QRectF(5.0, 6.0, 7.0, 8.0);
    copy.textIndex = 47;
    copy.text = QStringLiteral("Changed copy");
    copy.setWritingMode(QStringLiteral("Hrzn"));

    QCOMPARE(shape.engineData.value(QStringLiteral("language")).toString(), QStringLiteral("ja"));
    QCOMPARE(shape.bounds, QRectF(-31.5, -37.25, 73.25, 80.75));
    QCOMPARE(shape.boundingBox, QRectF(-5.0, -7.0, 19.0, 23.0));
    QCOMPARE(shape.textIndex, -29);
    QCOMPARE(shape.text, QStringLiteral("LibrePaint text"));
    QVERIFY(!shape.isHorizontal);
    QVERIFY(copy.transform != shape.transform);
}

QTEST_GUILESS_MAIN(PsdFormatValuesContractTest)

#include "PsdFormatValuesContractTest.moc"
