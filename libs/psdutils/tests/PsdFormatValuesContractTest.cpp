/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "psd.h"
#include "psd_additional_layer_info_block.h"
#include "psd_layer_record.h"
#include "psd_resource_block.h"

#include <QBuffer>
#include <QDataStream>
#include <QTest>

#include <type_traits>

namespace
{

template<typename Resource>
void verifyLegacyResourceAcceptsAndIgnoresPayloads()
{
    static_assert(std::is_base_of_v<PSDInterpretedResource, Resource>);
    static_assert(std::is_default_constructible_v<Resource>);

    Resource resource;
    QVERIFY(resource.error.isEmpty());
    QVERIFY(resource.interpretBlock(QByteArray()));
    QVERIFY(resource.error.isEmpty());
    QVERIFY(resource.valid());
    QVERIFY(resource.displayText().isEmpty());

    QByteArray payload = QByteArray::fromHex("00017f80ff00");
    const QByteArray originalPayload = payload;
    QVERIFY(resource.interpretBlock(payload));
    QCOMPARE(payload, originalPayload);
    QVERIFY(resource.error.isEmpty());
    QVERIFY(resource.valid());
    QVERIFY(resource.displayText().isEmpty());
}

} // namespace

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
    void pathNodesAndSubpathsCopyGeometryIndependently();
    void pathsAndVectorMasksPreserveNestedValueState();
    void vectorOriginationDefaultsAndMembersCopyIndependently();
    void vectorOriginationSettersStoreAndAppendValues();
    void vectorOriginationQueriesClassifyAndMeasureShapes();
    void vectorStrokeDefaultsAndCopiesRemainIndependent();
    void vectorStrokeScalarSettersPreserveSignedValues();
    void vectorStrokePenMetricsTogglePixelUnits();
    void vectorStrokeCapAndJoinNamesMapKnownCodes();
    void vectorStrokeDashValuesClampAndDetachOnCopy();
    void gridGuideDefaultsAndCopiesPreserveSpatialValues();
    void gridGuideBlocksRoundTripAndDescribeOrientation();
    void globalAngleDefaultsCopyAndDescribeSignedValues();
    void globalAltitudeDefaultsCopyAndDescribeSignedValues();
    void globalLightingBlocksRoundTripSignedValues();
    void gradientColorRecordPreservesAggregateSchemaAndCopyState();
    void gradientMapDescriptorPreservesSignedControlValues();
    void gradientMapStopSequencesPreserveCountsAndPointerIdentity();
    void gradientMapProceduralControlsPreserveSignedValues();
    void gradientMapColorTableCopiesInlineValuesIndependently();
    void gradientFillDefaultsAndCopiesValueState();
    void gradientFillScalarSettersPreserveSignedValues();
    void gradientFillTypeCodesAndSvgCompatibilityRemainStable();
    void patternFillDefaultsAndCopiesValueState();
    void patternFillSettersPreserveValues();
    void interpretedResourceDefaultsRemainStable();
    void resolutionInfoDefaultsAndCopiesRemainIndependent();
    void resolutionInfoBlocksRoundTripInMemory();
    void iccProfileDefaultsAndCopiesRemainIndependent();
    void iccProfileBlocksValidateAndRoundTripInMemory();
    void legacyResourceRecords1001To1014AcceptAndIgnorePayloads();
    void legacyResourceRecords1015To1030AcceptAndIgnorePayloads();
    void legacyResourceRecords1033To1045AcceptAndIgnorePayloads();
    void legacyResourceRecords1046To1060AcceptAndIgnorePayloads();
    void legacyPathAndPrintResourceRecordsAcceptAndIgnorePayloads();
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

void PsdFormatValuesContractTest::pathNodesAndSubpathsCopyGeometryIndependently()
{
    const psd_path_node emptyNode{};
    QCOMPARE(emptyNode.control1, QPointF());
    QCOMPARE(emptyNode.node, QPointF());
    QCOMPARE(emptyNode.control2, QPointF());
    QVERIFY(!emptyNode.isSmooth);

    psd_path_node node{};
    node.control1 = QPointF(-13.5, 17.25);
    node.node = QPointF(19.75, -23.5);
    node.control2 = QPointF(29.25, 31.5);
    node.isSmooth = true;
    psd_path_node nodeCopy = node;
    QCOMPARE(nodeCopy.control1, QPointF(-13.5, 17.25));
    QCOMPARE(nodeCopy.node, QPointF(19.75, -23.5));
    QCOMPARE(nodeCopy.control2, QPointF(29.25, 31.5));
    QVERIFY(nodeCopy.isSmooth);
    nodeCopy.control1 = QPointF(37.0, 41.0);
    nodeCopy.node = QPointF(43.0, 47.0);
    nodeCopy.control2 = QPointF(53.0, 59.0);
    nodeCopy.isSmooth = false;
    QCOMPARE(node.control1, QPointF(-13.5, 17.25));
    QCOMPARE(node.node, QPointF(19.75, -23.5));
    QCOMPARE(node.control2, QPointF(29.25, 31.5));
    QVERIFY(node.isSmooth);

    const psd_path_sub_path emptySubPath{};
    QVERIFY(emptySubPath.nodes.isEmpty());
    QVERIFY(!emptySubPath.isClosed);

    psd_path_sub_path subPath{};
    subPath.nodes.append(node);
    subPath.isClosed = true;
    psd_path_sub_path subPathCopy = subPath;
    QCOMPARE(subPathCopy.nodes.size(), 1);
    QCOMPARE(subPathCopy.nodes[0].node, QPointF(19.75, -23.5));
    QVERIFY(subPathCopy.isClosed);
    subPathCopy.nodes[0].node = QPointF(-61.0, 67.0);
    subPathCopy.nodes.append(nodeCopy);
    subPathCopy.isClosed = false;
    QCOMPARE(subPath.nodes.size(), 1);
    QCOMPARE(subPath.nodes[0].node, QPointF(19.75, -23.5));
    QVERIFY(subPath.isClosed);
}

void PsdFormatValuesContractTest::pathsAndVectorMasksPreserveNestedValueState()
{
    const psd_path emptyPath{};
    QVERIFY(!emptyPath.initialFillRecord);
    QCOMPARE(emptyPath.clipBoardBounds, QRectF());
    QCOMPARE(emptyPath.clipBoardResolution, 0.0);
    QVERIFY(emptyPath.subPaths.isEmpty());

    psd_path_node node{};
    node.control1 = QPointF(-3.0, 5.0);
    node.node = QPointF(7.0, -11.0);
    node.control2 = QPointF(13.0, 17.0);
    psd_path_sub_path subPath{};
    subPath.nodes.append(node);
    subPath.isClosed = true;

    psd_path path{};
    path.initialFillRecord = true;
    path.clipBoardBounds = QRectF(-19.0, -23.0, 29.0, 31.0);
    path.clipBoardResolution = 144.5;
    path.subPaths.append(subPath);
    psd_path pathCopy = path;
    QVERIFY(pathCopy.initialFillRecord);
    QCOMPARE(pathCopy.clipBoardBounds, QRectF(-19.0, -23.0, 29.0, 31.0));
    QCOMPARE(pathCopy.clipBoardResolution, 144.5);
    QCOMPARE(pathCopy.subPaths.size(), 1);
    pathCopy.initialFillRecord = false;
    pathCopy.clipBoardBounds = QRectF(37.0, 41.0, 43.0, 47.0);
    pathCopy.clipBoardResolution = 300.0;
    pathCopy.subPaths[0].nodes[0].node = QPointF(53.0, 59.0);
    QVERIFY(path.initialFillRecord);
    QCOMPARE(path.clipBoardBounds, QRectF(-19.0, -23.0, 29.0, 31.0));
    QCOMPARE(path.clipBoardResolution, 144.5);
    QCOMPARE(path.subPaths[0].nodes[0].node, QPointF(7.0, -11.0));

    const psd_vector_mask emptyMask{};
    QVERIFY(!emptyMask.invert);
    QVERIFY(!emptyMask.notLink);
    QVERIFY(!emptyMask.disable);
    QVERIFY(emptyMask.path.subPaths.isEmpty());

    psd_vector_mask mask{};
    mask.invert = true;
    mask.notLink = true;
    mask.disable = true;
    mask.path = path;
    psd_vector_mask maskCopy = mask;
    QVERIFY(maskCopy.invert);
    QVERIFY(maskCopy.notLink);
    QVERIFY(maskCopy.disable);
    QCOMPARE(maskCopy.path.clipBoardResolution, 144.5);
    maskCopy.invert = false;
    maskCopy.notLink = false;
    maskCopy.disable = false;
    maskCopy.path.subPaths[0].nodes[0].control1 = QPointF(-61.0, -67.0);
    QVERIFY(mask.invert);
    QVERIFY(mask.notLink);
    QVERIFY(mask.disable);
    QCOMPARE(mask.path.subPaths[0].nodes[0].control1, QPointF(-3.0, 5.0));
}

void PsdFormatValuesContractTest::vectorOriginationDefaultsAndMembersCopyIndependently()
{
    const psd_vector_origination_data empty{};
    QCOMPARE(empty.originType, -1);
    QCOMPARE(empty.originShapeBBox, QRectF());
    QCOMPARE(empty.originResolution, 72.0);
    QCOMPARE(empty.transform, QTransform());
    QCOMPARE(empty.originIndex, 0);
    QVERIFY(empty.originBoxCorners.isEmpty());
    QCOMPARE(empty.originPolySides, 0);
    QCOMPARE(empty.originPolyStarRatio, 100.0);
    QVERIFY(!empty.isStar);
    QCOMPARE(empty.originPolyPixelHSF, 1.0);
    QVERIFY(empty.originPolyPreviousTightBoxCorners.isEmpty());
    QVERIFY(empty.originPolyTrueRectCorners.isEmpty());
    QCOMPARE(empty.typeToName.value(1), QStringLiteral("RectangleShape"));
    QCOMPARE(empty.typeToName.value(5), QStringLiteral("EllipseShape"));
    QCOMPARE(empty.typeToName.value(7), QStringLiteral("StarShape"));
    QCOMPARE(empty.typeToName.value(8), QStringLiteral("StarShape"));

    psd_vector_origination_data data{};
    data.originType = 8;
    data.originShapeBBox = QRectF(-5.0, -7.0, 11.0, 13.0);
    data.originResolution = 300.0;
    data.transform.translate(-17.0, 19.0);
    data.originIndex = -23;
    data.originBoxCorners = QPolygonF({QPointF(-29.0, 31.0), QPointF(37.0, -41.0)});
    data.originPolySides = 7;
    data.originPolyStarRatio = 43.5;
    data.isStar = true;
    data.originPolyPixelHSF = -1.25;
    data.originPolyPreviousTightBoxCorners = QPolygonF({QPointF(47.0, 53.0)});
    data.originPolyTrueRectCorners = QPolygonF({QPointF(-59.0, 61.0)});

    psd_vector_origination_data copy = data;
    QCOMPARE(copy.originType, 8);
    QCOMPARE(copy.originShapeBBox, QRectF(-5.0, -7.0, 11.0, 13.0));
    QCOMPARE(copy.originResolution, 300.0);
    QCOMPARE(copy.transform, data.transform);
    QCOMPARE(copy.originIndex, -23);
    QCOMPARE(copy.originBoxCorners, data.originBoxCorners);
    QCOMPARE(copy.originPolySides, 7);
    QCOMPARE(copy.originPolyStarRatio, 43.5);
    QVERIFY(copy.isStar);
    QCOMPARE(copy.originPolyPixelHSF, -1.25);
    QCOMPARE(copy.originPolyPreviousTightBoxCorners, data.originPolyPreviousTightBoxCorners);
    QCOMPARE(copy.originPolyTrueRectCorners, data.originPolyTrueRectCorners);
    QCOMPARE(copy.typeToName, data.typeToName);

    copy.originType = 5;
    copy.originShapeBBox = QRectF(67.0, 71.0, 73.0, 79.0);
    copy.originResolution = 600.0;
    copy.transform.scale(2.0, 3.0);
    copy.originIndex = 83;
    copy.originBoxCorners[0] = QPointF(89.0, 97.0);
    copy.originPolySides = 9;
    copy.originPolyStarRatio = 101.0;
    copy.isStar = false;
    copy.originPolyPixelHSF = 2.5;
    copy.originPolyPreviousTightBoxCorners[0] = QPointF(103.0, 107.0);
    copy.originPolyTrueRectCorners[0] = QPointF(109.0, 113.0);

    QCOMPARE(data.originType, 8);
    QCOMPARE(data.originShapeBBox, QRectF(-5.0, -7.0, 11.0, 13.0));
    QCOMPARE(data.originResolution, 300.0);
    QCOMPARE(data.transform, QTransform::fromTranslate(-17.0, 19.0));
    QCOMPARE(data.originIndex, -23);
    QCOMPARE(data.originBoxCorners[0], QPointF(-29.0, 31.0));
    QCOMPARE(data.originPolySides, 7);
    QCOMPARE(data.originPolyStarRatio, 43.5);
    QVERIFY(data.isStar);
    QCOMPARE(data.originPolyPixelHSF, -1.25);
    QCOMPARE(data.originPolyPreviousTightBoxCorners[0], QPointF(47.0, 53.0));
    QCOMPARE(data.originPolyTrueRectCorners[0], QPointF(-59.0, 61.0));
}

void PsdFormatValuesContractTest::vectorOriginationSettersStoreAndAppendValues()
{
    psd_vector_origination_data data{};
    data.setOriginType(7);
    data.setOriginResolution(288.0);
    QTransform transform;
    transform.translate(-3.0, 5.0);
    transform.rotate(30.0);
    data.setTransform(transform);
    data.setOriginShapeBBox(QRectF(-7.0, -11.0, 13.0, 17.0));
    data.setOriginBoxCorners(QPointF(-19.0, 23.0));
    data.setOriginBoxCorners(QPointF(29.0, -31.0));
    data.setOriginPolyTightBoxCorners(QPointF(37.0, 41.0));
    data.setOriginPolyTightBoxCorners(QPointF(-43.0, 47.0));
    data.setOriginPolyTrueRectCorners(QPointF(53.0, -59.0));
    data.setOriginPolyTrueRectCorners(QPointF(61.0, 67.0));
    data.setOriginPolySides(9);
    data.setOriginPolyStarRatio(37.5);

    QCOMPARE(data.originType, 7);
    QCOMPARE(data.originResolution, 288.0);
    QCOMPARE(data.transform, transform);
    QCOMPARE(data.originShapeBBox, QRectF(-7.0, -11.0, 13.0, 17.0));
    QCOMPARE(data.originBoxCorners,
             QPolygonF({QPointF(-19.0, 23.0), QPointF(29.0, -31.0)}));
    QCOMPARE(data.originPolyPreviousTightBoxCorners,
             QPolygonF({QPointF(37.0, 41.0), QPointF(-43.0, 47.0)}));
    QCOMPARE(data.originPolyTrueRectCorners,
             QPolygonF({QPointF(53.0, -59.0), QPointF(61.0, 67.0)}));
    QCOMPARE(data.originPolySides, 9);
    QCOMPARE(data.originPolyStarRatio, 37.5);
    QVERIFY(data.isStar);
}

void PsdFormatValuesContractTest::vectorOriginationQueriesClassifyAndMeasureShapes()
{
    psd_vector_origination_data data{};
    data.originType = 1;
    QCOMPARE(data.shapeName(), QStringLiteral("RectangleShape"));
    QVERIFY(data.canMakeParametricShape());
    data.originType = 5;
    QCOMPARE(data.shapeName(), QStringLiteral("EllipseShape"));
    QVERIFY(data.canMakeParametricShape());
    data.originType = 7;
    QCOMPARE(data.shapeName(), QStringLiteral("StarShape"));
    QVERIFY(data.canMakeParametricShape());
    data.originType = 8;
    QCOMPARE(data.shapeName(), QStringLiteral("StarShape"));
    QVERIFY(data.canMakeParametricShape());
    data.originType = 99;
    QVERIFY(data.shapeName().isEmpty());
    QVERIFY(!data.canMakeParametricShape());

    data.originShapeBBox = QRectF(-10.0, -20.0, 30.0, 40.0);
    QSizeF size;
    double angle = -1.0;
    data.OriginalSizeAndAngle(size, angle);
    QCOMPARE(size, QSizeF(30.0, 40.0));
    QCOMPARE(angle, 0.0);

    const QPolygonF originalCorners({QPointF(0.0, 0.0),
                                     QPointF(30.0, 0.0),
                                     QPointF(30.0, 40.0),
                                     QPointF(0.0, 40.0)});
    data.transform = QTransform::fromTranslate(17.0, -23.0);
    data.originBoxCorners = data.transform.map(originalCorners);
    size = QSizeF();
    angle = -1.0;
    data.OriginalSizeAndAngle(size, angle);
    QCOMPARE(size, QSizeF(30.0, 40.0));
    QCOMPARE(angle, 0.0);
}

void PsdFormatValuesContractTest::vectorStrokeDefaultsAndCopiesRemainIndependent()
{
    const psd_vector_stroke_data empty{};
    QCOMPARE(empty.strokeVersion, 2);
    QVERIFY(!empty.strokeEnabled);
    QVERIFY(empty.fillEnabled);
    QCOMPARE(empty.pen, QPen());
    QVERIFY(!empty.gradient);
    QVERIFY(!empty.scaleLock);
    QVERIFY(!empty.strokeAdjust);
    QVERIFY(empty.dashPattern.isEmpty());
    QCOMPARE(empty.opacity, 1.0);
    QCOMPARE(empty.resolution, 72.0);
    QVERIFY(!empty.pixelWidth);

    psd_vector_stroke_data data{};
    data.strokeVersion = -3;
    data.strokeEnabled = true;
    data.fillEnabled = false;
    data.pen.setWidthF(5.25);
    data.gradient = true;
    data.scaleLock = true;
    data.strokeAdjust = true;
    data.dashPattern = {-7.5, 11.25};
    data.opacity = -0.5;
    data.resolution = -144.5;
    data.pixelWidth = true;

    psd_vector_stroke_data copy = data;
    QCOMPARE(copy.strokeVersion, -3);
    QVERIFY(copy.strokeEnabled);
    QVERIFY(!copy.fillEnabled);
    QCOMPARE(copy.pen.widthF(), 5.25);
    QVERIFY(copy.gradient);
    QVERIFY(copy.scaleLock);
    QVERIFY(copy.strokeAdjust);
    QCOMPARE(copy.dashPattern, QVector<double>({-7.5, 11.25}));
    QCOMPARE(copy.opacity, -0.5);
    QCOMPARE(copy.resolution, -144.5);
    QVERIFY(copy.pixelWidth);

    copy.strokeVersion = 7;
    copy.strokeEnabled = false;
    copy.fillEnabled = true;
    copy.pen.setWidthF(13.75);
    copy.gradient = false;
    copy.scaleLock = false;
    copy.strokeAdjust = false;
    copy.dashPattern[0] = 17.5;
    copy.opacity = 0.25;
    copy.resolution = 288.0;
    copy.pixelWidth = false;

    QCOMPARE(data.strokeVersion, -3);
    QVERIFY(data.strokeEnabled);
    QVERIFY(!data.fillEnabled);
    QCOMPARE(data.pen.widthF(), 5.25);
    QVERIFY(data.gradient);
    QVERIFY(data.scaleLock);
    QVERIFY(data.strokeAdjust);
    QCOMPARE(data.dashPattern, QVector<double>({-7.5, 11.25}));
    QCOMPARE(data.opacity, -0.5);
    QCOMPARE(data.resolution, -144.5);
    QVERIFY(data.pixelWidth);
}

void PsdFormatValuesContractTest::vectorStrokeScalarSettersPreserveSignedValues()
{
    psd_vector_stroke_data data{};
    data.setVersion(-17);
    data.setStrokeEnabled(true);
    data.setFillEnabled(false);
    data.setScaleLock(true);
    data.setStrokeAdjust(true);
    data.setOpacityFromPercentage(-37.5);
    data.setResolution(-288.5);

    QCOMPARE(data.strokeVersion, -17);
    QVERIFY(data.strokeEnabled);
    QVERIFY(!data.fillEnabled);
    QVERIFY(data.scaleLock);
    QVERIFY(data.strokeAdjust);
    QCOMPARE(data.opacity, -37.5 * 0.01);
    QCOMPARE(data.resolution, -288.5);

    data.setStrokeEnabled(false);
    data.setFillEnabled(true);
    data.setScaleLock(false);
    data.setStrokeAdjust(false);
    QVERIFY(!data.strokeEnabled);
    QVERIFY(data.fillEnabled);
    QVERIFY(!data.scaleLock);
    QVERIFY(!data.strokeAdjust);
}

void PsdFormatValuesContractTest::vectorStrokePenMetricsTogglePixelUnits()
{
    psd_vector_stroke_data data{};
    data.setStrokeWidth(7.25);
    QCOMPARE(data.pen.widthF(), 7.25);
    QVERIFY(!data.pixelWidth);

    data.setStrokePixel(3.5);
    QCOMPARE(data.pen.widthF(), 3.5);
    QVERIFY(data.pixelWidth);

    data.setStrokeDashOffset(-11.75);
    data.setStrokeMiterLimit(17.5);
    QCOMPARE(data.pen.dashOffset(), -11.75);
    QCOMPARE(data.pen.miterLimit(), 17.5);

    psd_vector_stroke_data copy = data;
    copy.setStrokeWidth(19.25);
    copy.setStrokeDashOffset(23.5);
    copy.setStrokeMiterLimit(29.75);
    QCOMPARE(data.pen.widthF(), 3.5);
    QCOMPARE(data.pen.dashOffset(), -11.75);
    QCOMPARE(data.pen.miterLimit(), 17.5);
    QVERIFY(data.pixelWidth);
    QVERIFY(!copy.pixelWidth);
}

void PsdFormatValuesContractTest::vectorStrokeCapAndJoinNamesMapKnownCodes()
{
    psd_vector_stroke_data data{};

    data.setLineCapType(QStringLiteral("strokeStyleButtCap"));
    QCOMPARE(data.pen.capStyle(), Qt::FlatCap);
    data.setLineCapType(QStringLiteral("strokeStyleSquareCap"));
    QCOMPARE(data.pen.capStyle(), Qt::SquareCap);
    data.setLineCapType(QStringLiteral("strokeStyleRoundCap"));
    QCOMPARE(data.pen.capStyle(), Qt::RoundCap);
    data.setLineCapType(QStringLiteral("unknownCap"));
    QCOMPARE(data.pen.capStyle(), Qt::RoundCap);

    data.setLineJoinType(QStringLiteral("strokeStyleMiterJoin"));
    QCOMPARE(data.pen.joinStyle(), Qt::MiterJoin);
    data.setLineJoinType(QStringLiteral("strokeStyleBevelJoin"));
    QCOMPARE(data.pen.joinStyle(), Qt::BevelJoin);
    data.setLineJoinType(QStringLiteral("strokeStyleRoundJoin"));
    QCOMPARE(data.pen.joinStyle(), Qt::RoundJoin);
    data.setLineJoinType(QStringLiteral("unknownJoin"));
    QCOMPARE(data.pen.joinStyle(), Qt::RoundJoin);
}

void PsdFormatValuesContractTest::vectorStrokeDashValuesClampAndDetachOnCopy()
{
    psd_vector_stroke_data data{};
    data.appendToDashPattern(-3.0);
    data.appendToDashPattern(0.0);
    data.appendToDashPattern(1e-6);
    data.appendToDashPattern(2.75);
    QCOMPARE(data.dashPattern, QVector<double>({1e-6, 1e-6, 1e-6, 2.75}));

    psd_vector_stroke_data copy = data;
    copy.dashPattern[0] = 5.5;
    copy.appendToDashPattern(7.25);
    QCOMPARE(data.dashPattern, QVector<double>({1e-6, 1e-6, 1e-6, 2.75}));
    QCOMPARE(copy.dashPattern, QVector<double>({5.5, 1e-6, 1e-6, 2.75, 7.25}));
}

void PsdFormatValuesContractTest::gridGuideDefaultsAndCopiesPreserveSpatialValues()
{
    const GRID_GUIDE_1032 empty{};
    QCOMPARE(empty.documentMultiplier, 32);
    QCOMPARE(empty.version, quint32(1));
    QCOMPARE(empty.gridHorizontal, qint32(0));
    QCOMPARE(empty.gridVertical, qint32(0));
    QVERIFY(empty.horizontalGuides.isEmpty());
    QVERIFY(empty.verticalGuides.isEmpty());

    GRID_GUIDE_1032 guides;
    guides.version = 3;
    guides.gridHorizontal = -64;
    guides.gridVertical = -96;
    guides.horizontalGuides = {2, 5};
    guides.verticalGuides = {7, 11};

    GRID_GUIDE_1032 copy = guides;
    QCOMPARE(copy.documentMultiplier, 32);
    QCOMPARE(copy.version, quint32(3));
    QCOMPARE(copy.gridHorizontal, qint32(-64));
    QCOMPARE(copy.gridVertical, qint32(-96));
    QCOMPARE(copy.horizontalGuides, QList<quint32>({2, 5}));
    QCOMPARE(copy.verticalGuides, QList<quint32>({7, 11}));

    copy.version = 4;
    copy.gridHorizontal = 13;
    copy.gridVertical = 17;
    copy.horizontalGuides[0] = 19;
    copy.verticalGuides[0] = 23;
    QCOMPARE(guides.version, quint32(3));
    QCOMPARE(guides.gridHorizontal, qint32(-64));
    QCOMPARE(guides.gridVertical, qint32(-96));
    QCOMPARE(guides.horizontalGuides, QList<quint32>({2, 5}));
    QCOMPARE(guides.verticalGuides, QList<quint32>({7, 11}));
}

void PsdFormatValuesContractTest::gridGuideBlocksRoundTripAndDescribeOrientation()
{
    GRID_GUIDE_1032 guides;
    guides.version = 4;
    guides.gridHorizontal = -64;
    guides.gridVertical = -96;
    guides.horizontalGuides = {5, 11};
    guides.verticalGuides = {3, 7};

    QByteArray block;
    QVERIFY(guides.createBlock(block));
    QCOMPARE(block.size(), 48);
    QCOMPARE(block.left(4), QByteArray("8BIM", 4));

    GRID_GUIDE_1032 parsed;
    QVERIFY(parsed.interpretBlock(block.mid(12)));
    QCOMPARE(parsed.version, quint32(4));
    QCOMPARE(parsed.gridHorizontal, qint32(-64));
    QCOMPARE(parsed.gridVertical, qint32(-96));
    QCOMPARE(parsed.horizontalGuides, QList<quint32>({5, 11}));
    QCOMPARE(parsed.verticalGuides, QList<quint32>({3, 7}));
    QVERIFY(parsed.valid());
    QCOMPARE(parsed.displayText(),
             QStringLiteral("Grids and Guides version: 4\n"
                            "Grids vertical: -96, horizontal: -64\n"
                            "Vertical guides: 3, 7\n"
                            "Horizontal guides: 5, 11"));
}

void PsdFormatValuesContractTest::globalAngleDefaultsCopyAndDescribeSignedValues()
{
    const GLOBAL_ANGLE_1037 empty{};
    QCOMPARE(empty.angle, qint32(30));

    GLOBAL_ANGLE_1037 angle;
    angle.angle = -135;
    GLOBAL_ANGLE_1037 copy = angle;
    QCOMPARE(copy.angle, qint32(-135));
    QVERIFY(copy.valid());
    QCOMPARE(copy.displayText(), QStringLiteral("Global Angle: -135"));

    copy.angle = 225;
    QCOMPARE(angle.angle, qint32(-135));
}

void PsdFormatValuesContractTest::globalAltitudeDefaultsCopyAndDescribeSignedValues()
{
    const GLOBAL_ALT_1049 empty{};
    QCOMPARE(empty.altitude, qint32(30));

    GLOBAL_ALT_1049 altitude;
    altitude.altitude = -45;
    GLOBAL_ALT_1049 copy = altitude;
    QCOMPARE(copy.altitude, qint32(-45));
    QVERIFY(copy.valid());
    QCOMPARE(copy.displayText(), QStringLiteral("Global Altitude: -45"));

    copy.altitude = 75;
    QCOMPARE(altitude.altitude, qint32(-45));
}

void PsdFormatValuesContractTest::globalLightingBlocksRoundTripSignedValues()
{
    GLOBAL_ANGLE_1037 angle;
    angle.angle = -135;
    QByteArray angleBlock;
    QVERIFY(angle.createBlock(angleBlock));
    QCOMPARE(angleBlock.size(), 16);

    QBuffer angleBuffer(&angleBlock);
    QVERIFY(angleBuffer.open(QIODevice::ReadOnly));
    QCOMPARE(angleBuffer.read(4), QByteArray("8BIM", 4));
    QDataStream angleHeader(&angleBuffer);
    angleHeader.setByteOrder(QDataStream::BigEndian);
    quint16 angleId = 0;
    quint16 angleNameLength = 1;
    quint32 angleSize = 0;
    angleHeader >> angleId >> angleNameLength >> angleSize;
    QCOMPARE(angleId, quint16(PSDImageResourceSection::GLOBAL_ANGLE));
    QCOMPARE(angleNameLength, quint16(0));
    QCOMPARE(angleSize, quint32(4));
    GLOBAL_ANGLE_1037 parsedAngle;
    QVERIFY(parsedAngle.interpretBlock(angleBuffer.readAll()));
    QCOMPARE(parsedAngle.angle, qint32(-135));

    GLOBAL_ALT_1049 altitude;
    altitude.altitude = -45;
    QByteArray altitudeBlock;
    QVERIFY(altitude.createBlock(altitudeBlock));
    QCOMPARE(altitudeBlock.size(), 16);

    QBuffer altitudeBuffer(&altitudeBlock);
    QVERIFY(altitudeBuffer.open(QIODevice::ReadOnly));
    QCOMPARE(altitudeBuffer.read(4), QByteArray("8BIM", 4));
    QDataStream altitudeHeader(&altitudeBuffer);
    altitudeHeader.setByteOrder(QDataStream::BigEndian);
    quint16 altitudeId = 0;
    quint16 altitudeNameLength = 1;
    quint32 altitudeSize = 0;
    altitudeHeader >> altitudeId >> altitudeNameLength >> altitudeSize;
    QCOMPARE(altitudeId, quint16(PSDImageResourceSection::GLOBAL_ALT));
    QCOMPARE(altitudeNameLength, quint16(0));
    QCOMPARE(altitudeSize, quint32(4));
    GLOBAL_ALT_1049 parsedAltitude;
    QVERIFY(parsedAltitude.interpretBlock(altitudeBuffer.readAll()));
    QCOMPARE(parsedAltitude.altitude, qint32(-45));
}

void PsdFormatValuesContractTest::gradientColorRecordPreservesAggregateSchemaAndCopyState()
{
    static_assert(std::is_aggregate_v<psd_gradient_color>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::smoothness), qint32>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::name_length), qint32>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::name), quint16 *>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::number_color_stops), qint8>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::color_stop), psd_gradient_color_stop *>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::number_transparency_stops), qint8>);
    static_assert(std::is_same_v<decltype(psd_gradient_color::transparency_stop), psd_gradient_transparency_stop *>);

    psd_gradient_color gradient{};
    QCOMPARE(gradient.smoothness, qint32(0));
    QCOMPARE(gradient.name_length, qint32(0));
    QVERIFY(gradient.name == nullptr);
    QCOMPARE(gradient.number_color_stops, qint8(0));
    QVERIFY(gradient.color_stop == nullptr);
    QCOMPARE(gradient.number_transparency_stops, qint8(0));
    QVERIFY(gradient.transparency_stop == nullptr);

    quint16 nameCodeUnit = 0x1234;
    auto *colorStops = reinterpret_cast<psd_gradient_color_stop *>(quintptr(0x1010));
    auto *transparencyStops = reinterpret_cast<psd_gradient_transparency_stop *>(quintptr(0x2020));
    gradient.smoothness = -32768;
    gradient.name_length = -17;
    gradient.name = &nameCodeUnit;
    gradient.number_color_stops = qint8(-5);
    gradient.color_stop = colorStops;
    gradient.number_transparency_stops = qint8(-7);
    gradient.transparency_stop = transparencyStops;

    psd_gradient_color copy = gradient;
    QCOMPARE(copy.smoothness, qint32(-32768));
    QCOMPARE(copy.name_length, qint32(-17));
    QCOMPARE(copy.name, &nameCodeUnit);
    QCOMPARE(copy.number_color_stops, qint8(-5));
    QCOMPARE(copy.color_stop, colorStops);
    QCOMPARE(copy.number_transparency_stops, qint8(-7));
    QCOMPARE(copy.transparency_stop, transparencyStops);

    copy.smoothness = 4096;
    copy.name_length = 1;
    copy.number_color_stops = 2;
    copy.number_transparency_stops = 3;
    QCOMPARE(gradient.smoothness, qint32(-32768));
    QCOMPARE(gradient.name_length, qint32(-17));
    QCOMPARE(gradient.number_color_stops, qint8(-5));
    QCOMPARE(gradient.number_transparency_stops, qint8(-7));
}

void PsdFormatValuesContractTest::gradientMapDescriptorPreservesSignedControlValues()
{
    static_assert(std::is_aggregate_v<psd_layer_gradient_map>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::reverse), bool>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::dithered), bool>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::name_length), qint32>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::name), quint16 *>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::expansion_count), qint8>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::interpolation), qint8>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::length), qint8>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::mode), qint8>);

    psd_layer_gradient_map gradient{};
    QVERIFY(!gradient.reverse);
    QVERIFY(!gradient.dithered);
    QCOMPARE(gradient.name_length, qint32(0));
    QVERIFY(gradient.name == nullptr);
    QCOMPARE(gradient.expansion_count, qint8(0));
    QCOMPARE(gradient.interpolation, qint8(0));
    QCOMPARE(gradient.length, qint8(0));
    QCOMPARE(gradient.mode, qint8(0));

    quint16 nameCodeUnit = 0xabcd;
    gradient.reverse = true;
    gradient.dithered = true;
    gradient.name_length = -23;
    gradient.name = &nameCodeUnit;
    gradient.expansion_count = qint8(-2);
    gradient.interpolation = qint8(-3);
    gradient.length = qint8(-4);
    gradient.mode = qint8(-5);

    psd_layer_gradient_map copy = gradient;
    QVERIFY(copy.reverse);
    QVERIFY(copy.dithered);
    QCOMPARE(copy.name_length, qint32(-23));
    QCOMPARE(copy.name, &nameCodeUnit);
    QCOMPARE(copy.expansion_count, qint8(-2));
    QCOMPARE(copy.interpolation, qint8(-3));
    QCOMPARE(copy.length, qint8(-4));
    QCOMPARE(copy.mode, qint8(-5));

    copy.reverse = false;
    copy.dithered = false;
    copy.name_length = 6;
    copy.expansion_count = 7;
    copy.interpolation = 8;
    copy.length = 9;
    copy.mode = 10;
    QVERIFY(gradient.reverse);
    QVERIFY(gradient.dithered);
    QCOMPARE(gradient.name_length, qint32(-23));
    QCOMPARE(gradient.expansion_count, qint8(-2));
    QCOMPARE(gradient.interpolation, qint8(-3));
    QCOMPARE(gradient.length, qint8(-4));
    QCOMPARE(gradient.mode, qint8(-5));
}

void PsdFormatValuesContractTest::gradientMapStopSequencesPreserveCountsAndPointerIdentity()
{
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::number_color_stops), qint8>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::color_stop), psd_gradient_color_stop *>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::number_transparency_stops), qint8>);
    static_assert(
        std::is_same_v<decltype(psd_layer_gradient_map::transparency_stop), psd_gradient_transparency_stop *>);

    psd_layer_gradient_map gradient{};
    QCOMPARE(gradient.number_color_stops, qint8(0));
    QVERIFY(gradient.color_stop == nullptr);
    QCOMPARE(gradient.number_transparency_stops, qint8(0));
    QVERIFY(gradient.transparency_stop == nullptr);

    auto *colorStops = reinterpret_cast<psd_gradient_color_stop *>(quintptr(0x3030));
    auto *transparencyStops = reinterpret_cast<psd_gradient_transparency_stop *>(quintptr(0x4040));
    gradient.number_color_stops = qint8(-11);
    gradient.color_stop = colorStops;
    gradient.number_transparency_stops = qint8(-13);
    gradient.transparency_stop = transparencyStops;

    psd_layer_gradient_map copy = gradient;
    QCOMPARE(copy.number_color_stops, qint8(-11));
    QCOMPARE(copy.color_stop, colorStops);
    QCOMPARE(copy.number_transparency_stops, qint8(-13));
    QCOMPARE(copy.transparency_stop, transparencyStops);

    copy.number_color_stops = 1;
    copy.color_stop = nullptr;
    copy.number_transparency_stops = 2;
    copy.transparency_stop = nullptr;
    QCOMPARE(gradient.number_color_stops, qint8(-11));
    QCOMPARE(gradient.color_stop, colorStops);
    QCOMPARE(gradient.number_transparency_stops, qint8(-13));
    QCOMPARE(gradient.transparency_stop, transparencyStops);
}

void PsdFormatValuesContractTest::gradientMapProceduralControlsPreserveSignedValues()
{
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::random_number_seed), qint32>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::showing_transparency_flag), qint8>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::using_vector_color_flag), qint8>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::roughness_factor), qint32>);

    psd_layer_gradient_map gradient{};
    QCOMPARE(gradient.random_number_seed, qint32(0));
    QCOMPARE(gradient.showing_transparency_flag, qint8(0));
    QCOMPARE(gradient.using_vector_color_flag, qint8(0));
    QCOMPARE(gradient.roughness_factor, qint32(0));

    gradient.random_number_seed = -2147483647;
    gradient.showing_transparency_flag = qint8(-17);
    gradient.using_vector_color_flag = qint8(-19);
    gradient.roughness_factor = -65536;

    psd_layer_gradient_map copy = gradient;
    QCOMPARE(copy.random_number_seed, qint32(-2147483647));
    QCOMPARE(copy.showing_transparency_flag, qint8(-17));
    QCOMPARE(copy.using_vector_color_flag, qint8(-19));
    QCOMPARE(copy.roughness_factor, qint32(-65536));

    copy.random_number_seed = 1;
    copy.showing_transparency_flag = 2;
    copy.using_vector_color_flag = 3;
    copy.roughness_factor = 4;
    QCOMPARE(gradient.random_number_seed, qint32(-2147483647));
    QCOMPARE(gradient.showing_transparency_flag, qint8(-17));
    QCOMPARE(gradient.using_vector_color_flag, qint8(-19));
    QCOMPARE(gradient.roughness_factor, qint32(-65536));
}

void PsdFormatValuesContractTest::gradientMapColorTableCopiesInlineValuesIndependently()
{
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::min_color), QColor>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::max_color), QColor>);
    static_assert(std::is_same_v<decltype(psd_layer_gradient_map::lookup_table), QColor[256]>);
    static_assert(std::extent_v<decltype(psd_layer_gradient_map::lookup_table)> == 256);

    psd_layer_gradient_map gradient{};
    gradient.min_color = QColor(1, 2, 3, 4);
    gradient.max_color = QColor(251, 252, 253, 254);
    gradient.lookup_table[0] = QColor(5, 6, 7, 8);
    gradient.lookup_table[127] = QColor(101, 102, 103, 104);
    gradient.lookup_table[255] = QColor(201, 202, 203, 204);

    psd_layer_gradient_map copy = gradient;
    QCOMPARE(copy.min_color, QColor(1, 2, 3, 4));
    QCOMPARE(copy.max_color, QColor(251, 252, 253, 254));
    QCOMPARE(copy.lookup_table[0], QColor(5, 6, 7, 8));
    QCOMPARE(copy.lookup_table[127], QColor(101, 102, 103, 104));
    QCOMPARE(copy.lookup_table[255], QColor(201, 202, 203, 204));

    copy.min_color = QColor(11, 12, 13, 14);
    copy.max_color = QColor(241, 242, 243, 244);
    copy.lookup_table[0] = QColor(15, 16, 17, 18);
    copy.lookup_table[127] = QColor(111, 112, 113, 114);
    copy.lookup_table[255] = QColor(211, 212, 213, 214);
    QCOMPARE(gradient.min_color, QColor(1, 2, 3, 4));
    QCOMPARE(gradient.max_color, QColor(251, 252, 253, 254));
    QCOMPARE(gradient.lookup_table[0], QColor(5, 6, 7, 8));
    QCOMPARE(gradient.lookup_table[127], QColor(101, 102, 103, 104));
    QCOMPARE(gradient.lookup_table[255], QColor(201, 202, 203, 204));
}

void PsdFormatValuesContractTest::gradientFillDefaultsAndCopiesValueState()
{
    const psd_layer_gradient_fill empty;
    QCOMPARE(empty.angle, 0.0);
    QCOMPARE(empty.style, QStringLiteral("linear"));
    QCOMPARE(empty.repeat, QStringLiteral("none"));
    QCOMPARE(empty.scale, 100.0);
    QVERIFY(!empty.reverse);
    QVERIFY(!empty.dithered);
    QVERIFY(!empty.align_with_layer);
    QCOMPARE(empty.offset, QPointF());
    QVERIFY(empty.gradient.isNull());
    QCOMPARE(empty.imageWidth, 1);
    QCOMPARE(empty.imageHeight, 1);

    psd_layer_gradient_fill fill;
    fill.angle = -135.25;
    fill.style = QStringLiteral("radial");
    fill.repeat = QStringLiteral("alternate");
    fill.scale = -42.5;
    fill.reverse = true;
    fill.dithered = true;
    fill.align_with_layer = true;
    fill.offset = QPointF(-12.5, 8.25);
    QDomElement gradientElement = fill.gradient.createElement(QStringLiteral("gradient"));
    fill.gradient.appendChild(gradientElement);
    fill.imageWidth = -640;
    fill.imageHeight = -480;

    psd_layer_gradient_fill copy = fill;
    QCOMPARE(copy.angle, -135.25);
    QCOMPARE(copy.style, QStringLiteral("radial"));
    QCOMPARE(copy.repeat, QStringLiteral("alternate"));
    QCOMPARE(copy.scale, -42.5);
    QVERIFY(copy.reverse);
    QVERIFY(copy.dithered);
    QVERIFY(copy.align_with_layer);
    QCOMPARE(copy.offset, QPointF(-12.5, 8.25));
    QCOMPARE(copy.gradient.documentElement().tagName(), QStringLiteral("gradient"));
    QCOMPARE(copy.imageWidth, -640);
    QCOMPARE(copy.imageHeight, -480);

    copy.angle = 22.5;
    copy.style = QStringLiteral("linear");
    copy.repeat = QStringLiteral("none");
    copy.scale = 75.0;
    copy.reverse = false;
    copy.dithered = false;
    copy.align_with_layer = false;
    copy.offset = QPointF(4.0, -6.0);
    copy.gradient = QDomDocument();
    copy.imageWidth = 320;
    copy.imageHeight = 240;
    QCOMPARE(fill.angle, -135.25);
    QCOMPARE(fill.style, QStringLiteral("radial"));
    QCOMPARE(fill.repeat, QStringLiteral("alternate"));
    QCOMPARE(fill.scale, -42.5);
    QVERIFY(fill.reverse);
    QVERIFY(fill.dithered);
    QVERIFY(fill.align_with_layer);
    QCOMPARE(fill.offset, QPointF(-12.5, 8.25));
    QCOMPARE(fill.gradient.documentElement().tagName(), QStringLiteral("gradient"));
    QCOMPARE(fill.imageWidth, -640);
    QCOMPARE(fill.imageHeight, -480);
}

void PsdFormatValuesContractTest::gradientFillScalarSettersPreserveSignedValues()
{
    psd_layer_gradient_fill fill;
    fill.setAlignWithLayer(true);
    fill.setAngle(-135.25f);
    fill.setDither(true);
    fill.setOffset(QPointF(-12.5, 8.25));
    fill.setReverse(true);
    fill.setScale(-42.5f);

    QVERIFY(fill.align_with_layer);
    QCOMPARE(fill.angle, -135.25);
    QVERIFY(fill.dithered);
    QCOMPARE(fill.offset, QPointF(-12.5, 8.25));
    QVERIFY(fill.reverse);
    QCOMPARE(fill.scale, -42.5);

    fill.setAlignWithLayer(false);
    fill.setDither(false);
    fill.setReverse(false);
    QVERIFY(!fill.align_with_layer);
    QVERIFY(!fill.dithered);
    QVERIFY(!fill.reverse);
}

void PsdFormatValuesContractTest::gradientFillTypeCodesAndSvgCompatibilityRemainStable()
{
    psd_layer_gradient_fill fill;

    fill.repeat = QStringLiteral("stale");
    fill.setType(QStringLiteral("Lnr "));
    QCOMPARE(fill.style, QStringLiteral("linear"));
    QCOMPARE(fill.repeat, QStringLiteral("none"));
    QVERIFY(fill.svgCompatible());

    fill.setType(QStringLiteral("Rdl "));
    QCOMPARE(fill.style, QStringLiteral("radial"));
    QCOMPARE(fill.repeat, QStringLiteral("none"));
    QVERIFY(fill.svgCompatible());

    fill.setType(QStringLiteral("Angl"));
    QCOMPARE(fill.style, QStringLiteral("conical"));
    QCOMPARE(fill.repeat, QStringLiteral("none"));
    QVERIFY(!fill.svgCompatible());

    fill.setType(QStringLiteral("Rflc"));
    QCOMPARE(fill.style, QStringLiteral("bilinear"));
    QCOMPARE(fill.repeat, QStringLiteral("alternate"));
    QVERIFY(!fill.svgCompatible());

    fill.setType(QStringLiteral("Dmnd"));
    QCOMPARE(fill.style, QStringLiteral("square"));
    QCOMPARE(fill.repeat, QStringLiteral("none"));
    QVERIFY(!fill.svgCompatible());
}

void PsdFormatValuesContractTest::patternFillDefaultsAndCopiesValueState()
{
    const psd_layer_pattern_fill empty;
    QCOMPARE(empty.angle, 0.0);
    QCOMPARE(empty.scale, 100.0);
    QCOMPARE(empty.offset, QPointF());
    QVERIFY(empty.patternName.isEmpty());
    QVERIFY(empty.patternID.isEmpty());
    QVERIFY(empty.pattern.isNull());
    QVERIFY(!empty.align_with_layer);

    psd_layer_pattern_fill fill;
    fill.angle = -90.5;
    fill.scale = -25.0;
    fill.offset = QPointF(-7.5, 11.25);
    fill.patternName = QStringLiteral("Grid");
    fill.patternID = QStringLiteral("pattern-id");
    fill.align_with_layer = true;

    psd_layer_pattern_fill copy = fill;
    QCOMPARE(copy.angle, -90.5);
    QCOMPARE(copy.scale, -25.0);
    QCOMPARE(copy.offset, QPointF(-7.5, 11.25));
    QCOMPARE(copy.patternName, QStringLiteral("Grid"));
    QCOMPARE(copy.patternID, QStringLiteral("pattern-id"));
    QVERIFY(copy.pattern.isNull());
    QVERIFY(copy.align_with_layer);

    copy.angle = 45.0;
    copy.scale = 125.0;
    copy.offset = QPointF(3.0, -4.0);
    copy.patternName = QStringLiteral("Dots");
    copy.patternID = QStringLiteral("copy-id");
    copy.align_with_layer = false;
    QCOMPARE(fill.angle, -90.5);
    QCOMPARE(fill.scale, -25.0);
    QCOMPARE(fill.offset, QPointF(-7.5, 11.25));
    QCOMPARE(fill.patternName, QStringLiteral("Grid"));
    QCOMPARE(fill.patternID, QStringLiteral("pattern-id"));
    QVERIFY(fill.pattern.isNull());
    QVERIFY(fill.align_with_layer);
}

void PsdFormatValuesContractTest::patternFillSettersPreserveValues()
{
    psd_layer_pattern_fill fill;
    fill.setAlignWithLayer(true);
    fill.setAngle(-90.5f);
    fill.setOffset(QPointF(-7.5, 11.25));
    fill.setPatternRef(QStringLiteral("pattern-id"), QStringLiteral("Grid"));
    fill.setScale(-25.0f);

    QVERIFY(fill.align_with_layer);
    QCOMPARE(fill.angle, -90.5);
    QCOMPARE(fill.offset, QPointF(-7.5, 11.25));
    QCOMPARE(fill.patternID, QStringLiteral("pattern-id"));
    QCOMPARE(fill.patternName, QStringLiteral("Grid"));
    QCOMPARE(fill.scale, -25.0);

    fill.setAlignWithLayer(false);
    QVERIFY(!fill.align_with_layer);
}

void PsdFormatValuesContractTest::interpretedResourceDefaultsRemainStable()
{
    static_assert(std::is_destructible_v<PSDInterpretedResource>);

    PSDInterpretedResource resource;
    QVERIFY(resource.error.isEmpty());

    QByteArray block("unchanged");
    QVERIFY(resource.createBlock(block));
    QCOMPARE(block, QByteArray("unchanged"));
    QVERIFY(resource.interpretBlock(QByteArray("ignored")));
    QVERIFY(resource.valid());
    QVERIFY(resource.displayText().isEmpty());
}

void PsdFormatValuesContractTest::resolutionInfoDefaultsAndCopiesRemainIndependent()
{
    static_assert(std::is_base_of_v<PSDInterpretedResource, RESN_INFO_1005>);

    QCOMPARE(int(RESN_INFO_1005::PSD_UNIT_INCH), 1);
    QCOMPARE(int(RESN_INFO_1005::PSD_UNIT_CM), 2);
    QCOMPARE(int(RESN_INFO_1005::PSD_UNIT_POINT), 3);
    QCOMPARE(int(RESN_INFO_1005::PSD_UNIT_PICA), 4);
    QCOMPARE(int(RESN_INFO_1005::PSD_UNIT_COLUMN), 5);

    const RESN_INFO_1005 empty;
    QCOMPARE(empty.hRes, Fixed(300));
    QCOMPARE(empty.hResUnit, quint16(RESN_INFO_1005::PSD_UNIT_INCH));
    QCOMPARE(empty.widthUnit, quint16(RESN_INFO_1005::PSD_UNIT_INCH));
    QCOMPARE(empty.vRes, Fixed(300));
    QCOMPARE(empty.vResUnit, quint16(RESN_INFO_1005::PSD_UNIT_INCH));
    QCOMPARE(empty.heightUnit, quint16(RESN_INFO_1005::PSD_UNIT_INCH));

    RESN_INFO_1005 resolution;
    resolution.hRes = -144;
    resolution.hResUnit = RESN_INFO_1005::PSD_UNIT_CM;
    resolution.widthUnit = RESN_INFO_1005::PSD_UNIT_POINT;
    resolution.vRes = -72;
    resolution.vResUnit = RESN_INFO_1005::PSD_UNIT_PICA;
    resolution.heightUnit = RESN_INFO_1005::PSD_UNIT_COLUMN;

    RESN_INFO_1005 copy = resolution;
    QCOMPARE(copy.hRes, Fixed(-144));
    QCOMPARE(copy.hResUnit, quint16(RESN_INFO_1005::PSD_UNIT_CM));
    QCOMPARE(copy.widthUnit, quint16(RESN_INFO_1005::PSD_UNIT_POINT));
    QCOMPARE(copy.vRes, Fixed(-72));
    QCOMPARE(copy.vResUnit, quint16(RESN_INFO_1005::PSD_UNIT_PICA));
    QCOMPARE(copy.heightUnit, quint16(RESN_INFO_1005::PSD_UNIT_COLUMN));

    copy.hRes = 600;
    copy.hResUnit = RESN_INFO_1005::PSD_UNIT_INCH;
    copy.widthUnit = RESN_INFO_1005::PSD_UNIT_INCH;
    copy.vRes = 1200;
    copy.vResUnit = RESN_INFO_1005::PSD_UNIT_INCH;
    copy.heightUnit = RESN_INFO_1005::PSD_UNIT_INCH;
    QCOMPARE(resolution.hRes, Fixed(-144));
    QCOMPARE(resolution.hResUnit, quint16(RESN_INFO_1005::PSD_UNIT_CM));
    QCOMPARE(resolution.widthUnit, quint16(RESN_INFO_1005::PSD_UNIT_POINT));
    QCOMPARE(resolution.vRes, Fixed(-72));
    QCOMPARE(resolution.vResUnit, quint16(RESN_INFO_1005::PSD_UNIT_PICA));
    QCOMPARE(resolution.heightUnit, quint16(RESN_INFO_1005::PSD_UNIT_COLUMN));
}

void PsdFormatValuesContractTest::resolutionInfoBlocksRoundTripInMemory()
{
    RESN_INFO_1005 resolution;
    resolution.hRes = 144;
    resolution.hResUnit = RESN_INFO_1005::PSD_UNIT_CM;
    resolution.widthUnit = RESN_INFO_1005::PSD_UNIT_POINT;
    resolution.vRes = 72;
    resolution.vResUnit = RESN_INFO_1005::PSD_UNIT_PICA;
    resolution.heightUnit = RESN_INFO_1005::PSD_UNIT_COLUMN;

    QByteArray block;
    QVERIFY(resolution.createBlock(block));
    QCOMPARE(block.size(), 28);

    QBuffer buffer(&block);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QCOMPARE(buffer.read(4), QByteArray("8BIM", 4));
    QDataStream header(&buffer);
    header.setByteOrder(QDataStream::BigEndian);
    quint16 resourceId = 0;
    quint16 nameLength = 1;
    quint32 payloadSize = 0;
    header >> resourceId >> nameLength >> payloadSize;
    QCOMPARE(resourceId, quint16(PSDImageResourceSection::RESN_INFO));
    QCOMPARE(nameLength, quint16(0));
    QCOMPARE(payloadSize, quint32(16));

    RESN_INFO_1005 parsed;
    QVERIFY(parsed.interpretBlock(buffer.readAll()));
    QCOMPARE(parsed.hRes, Fixed(144));
    QCOMPARE(parsed.hResUnit, quint16(RESN_INFO_1005::PSD_UNIT_CM));
    QCOMPARE(parsed.widthUnit, quint16(RESN_INFO_1005::PSD_UNIT_POINT));
    QCOMPARE(parsed.vRes, Fixed(72));
    QCOMPARE(parsed.vResUnit, quint16(RESN_INFO_1005::PSD_UNIT_PICA));
    QCOMPARE(parsed.heightUnit, quint16(RESN_INFO_1005::PSD_UNIT_COLUMN));
}

void PsdFormatValuesContractTest::iccProfileDefaultsAndCopiesRemainIndependent()
{
    static_assert(std::is_base_of_v<PSDInterpretedResource, ICC_PROFILE_1039>);

    const ICC_PROFILE_1039 empty;
    QVERIFY(empty.icc.isEmpty());

    ICC_PROFILE_1039 profile;
    profile.icc = QByteArray::fromHex("0001027f80ff");
    ICC_PROFILE_1039 copy = profile;
    QCOMPARE(copy.icc, QByteArray::fromHex("0001027f80ff"));

    copy.icc[0] = char(0x7f);
    QCOMPARE(profile.icc, QByteArray::fromHex("0001027f80ff"));
    QCOMPARE(copy.icc, QByteArray::fromHex("7f01027f80ff"));
}

void PsdFormatValuesContractTest::iccProfileBlocksValidateAndRoundTripInMemory()
{
    ICC_PROFILE_1039 empty;
    QByteArray untouched("unchanged");
    QVERIFY(!empty.createBlock(untouched));
    QCOMPARE(untouched, QByteArray("unchanged"));
    QCOMPARE(empty.error, QStringLiteral("ICC_PROFILE_1039: Trying to save an empty profile"));

    const QByteArray payload = QByteArray::fromHex("0001027f80ff");
    ICC_PROFILE_1039 profile;
    QVERIFY(profile.interpretBlock(payload));
    QCOMPARE(profile.icc, payload);

    QByteArray block;
    QVERIFY(profile.createBlock(block));
    QCOMPARE(block.size(), 18);

    QBuffer buffer(&block);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QCOMPARE(buffer.read(4), QByteArray("8BIM", 4));
    QDataStream header(&buffer);
    header.setByteOrder(QDataStream::BigEndian);
    quint16 resourceId = 0;
    quint16 nameLength = 1;
    quint32 payloadSize = 0;
    header >> resourceId >> nameLength >> payloadSize;
    QCOMPARE(resourceId, quint16(PSDImageResourceSection::ICC_PROFILE));
    QCOMPARE(nameLength, quint16(0));
    QCOMPARE(payloadSize, quint32(payload.size()));
    QCOMPARE(buffer.readAll(), payload);
}

void PsdFormatValuesContractTest::legacyResourceRecords1001To1014AcceptAndIgnorePayloads()
{
    verifyLegacyResourceAcceptsAndIgnoresPayloads<MAC_PRINT_INFO_1001>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<ALPHA_NAMES_1006>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<DISPLAY_INFO_1007>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<CAPTION_1008>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<BORDER_INFO_1009>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<BACKGROUND_COL_1010>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<PRINT_FLAGS_1011>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<GREY_HALFTONE_1012>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<COLOR_HALFTONE_1013>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<DUOTONE_HALFTONE_1014>();
}

void PsdFormatValuesContractTest::legacyResourceRecords1015To1030AcceptAndIgnorePayloads()
{
    verifyLegacyResourceAcceptsAndIgnoresPayloads<GREY_XFER_1015>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<COLOR_XFER_1016>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<DUOTONE_XFER_1017>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<DUOTONE_INFO_1018>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<EFFECTIVE_BW_1019>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<EPS_OPT_1021>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<QUICK_MASK_1022>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<LAYER_STATE_1024>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<WORKING_PATH_1025>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<LAYER_GROUP_1026>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<IPTC_NAA_DATA_1028>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<IMAGE_MODE_RAW_1029>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<JPEG_QUAL_1030>();
}

void PsdFormatValuesContractTest::legacyResourceRecords1033To1045AcceptAndIgnorePayloads()
{
    verifyLegacyResourceAcceptsAndIgnoresPayloads<THUMB_RES_1033>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<COPYRIGHT_FLG_1034>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<URL_1035>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<THUMB_RES2_1036>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<COLOR_SAMPLER_1038>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<WATERMARK_1040>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<ICC_UNTAGGED_1041>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<EFFECTS_VISIBLE_1042>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<SPOT_HALFTONE_1043>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<DOC_IDS_1044>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<ALPHA_NAMES_UNI_1045>();
}

void PsdFormatValuesContractTest::legacyResourceRecords1046To1060AcceptAndIgnorePayloads()
{
    verifyLegacyResourceAcceptsAndIgnoresPayloads<IDX_COL_TAB_CNT_1046>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<IDX_TRANSPARENT_1047>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<SLICES_1050>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<WORKFLOW_URL_UNI_1051>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<JUMP_TO_XPEP_1052>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<ALPHA_ID_1053>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<URL_LIST_UNI_1054>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<VERSION_INFO_1057>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<EXIF_DATA_1058>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<XMP_DATA_1060>();
}

void PsdFormatValuesContractTest::legacyPathAndPrintResourceRecordsAcceptAndIgnorePayloads()
{
    verifyLegacyResourceAcceptsAndIgnoresPayloads<PATH_INFO_FIRST_2000>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<PATH_INFO_LAST_2998>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<CLIPPING_PATH_2999>();
    verifyLegacyResourceAcceptsAndIgnoresPayloads<PRINT_FLAGS_2_10000>();
}

QTEST_GUILESS_MAIN(PsdFormatValuesContractTest)

#include "PsdFormatValuesContractTest.moc"
