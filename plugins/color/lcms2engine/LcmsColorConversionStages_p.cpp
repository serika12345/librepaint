/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "LcmsColorConversionStages_p.h"
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>
#include <lcms2_plugin.h>
#include <cmath>
#include <limits>
#include <new>

namespace {
constexpr quint32 MaximumStageCount = 16;
constexpr quint64 MaximumClutEntries = 16 * 1024 * 1024;

bool encoding(const KoColorSpace *cs, KoColorConversionStages::Encoding &value)
{
    if (!cs || !cs->profile() || cs->colorModelId() != RGBAColorModelID) return false;
    if (cs->colorDepthId() == Integer8BitsColorDepthID)
        value.depth = KoColorConversionStages::Encoding::U8;
    else if (cs->colorDepthId() == Integer16BitsColorDepthID)
        value.depth = KoColorConversionStages::Encoding::U16;
    else return false;
    value.iccProfile = cs->profile()->rawData();
    return !value.iccProfile.isEmpty();
}
}

KoColorConversionStagesResult lcmsDisplayStages(cmsHTRANSFORM transform,
    const KoColorSpace *source, const KoColorSpace *destination)
{
    using Value = KoColorConversionStages;
    using Result = KoColorConversionStagesResult;
    try {
        Value value;
        if (!encoding(source, value.input) || !encoding(destination, value.output))
            return {Result::Unsupported, {}, QStringLiteral("Display stages require RGB U8/U16 ICC encodings")};
        if (quint64(value.input.iccProfile.size()) > std::numeric_limits<cmsUInt32Number>::max() ||
            quint64(value.output.iccProfile.size()) > std::numeric_limits<cmsUInt32Number>::max())
            return {Result::CapacityExceeded, {}, QStringLiteral("Display ICC exceeds LCMS byte capacity")};
        const cmsPipeline *pipeline = transform ? cmsGetTransformPipeline(transform) : nullptr;
        if (!pipeline)
            return {Result::Unsupported, {}, QStringLiteral("LCMS did not provide a display pipeline")};
        quint32 stageCount = 0;
        for (const cmsStage *stage = cmsPipelineGetPtrToFirstStage(pipeline);
             stage; stage = cmsStageNext(stage)) {
            if (++stageCount > MaximumStageCount)
                return {Result::CapacityExceeded, {}, QStringLiteral("Display pipeline has too many stages")};
            if (cmsStageInputChannels(stage) != 3 || cmsStageOutputChannels(stage) != 3)
                return {Result::Unsupported, {}, QStringLiteral("Display stage is not logical RGB")};
            if (cmsStageType(stage) == cmsSigIdentityElemType) {
                // An identity stage copies logical RGB unchanged.
                continue;
            } else if (cmsStageType(stage) == cmsSigCurveSetElemType) {
                const auto *data = static_cast<const _cmsStageToneCurvesData *>(cmsStageData(stage));
                if (!data || data->nCurves != 3)
                    return {Result::InvalidData, {}, QStringLiteral("Invalid RGB curve stage")};
                Value::Curves curves;
                for (int channel = 0; channel < 3; ++channel) {
                    const cmsToneCurve *curve = data->TheCurves[channel];
                    const int type = cmsGetToneCurveParametricType(curve);
                    if (cmsIsToneCurveMultisegment(curve))
                        return {Result::Unsupported, {}, QStringLiteral("Segmented display curves are unsupported")};
                    if (type) {
                        if (type < -5 || type > 5)
                            return {Result::Unsupported, {}, QStringLiteral("Unsupported parametric display curve")};
                        const int magnitude = std::abs(type);
                        const int counts[] = {0, 1, 3, 4, 5, 7};
                        const cmsCurveSegment *segment = cmsGetToneCurveSegment(0, curve);
                        const double *parameters = segment && segment->Type == type ? segment->Params : nullptr;
                        if (!parameters)
                            return {Result::InvalidData, {}, QStringLiteral("Missing display curve parameters")};
                        Value::ParametricCurve result;
                        result.functionType = type;
                        for (int i = 0; i < counts[magnitude]; ++i) {
                            if (!std::isfinite(parameters[i]))
                                return {Result::InvalidData, {}, QStringLiteral("Nonfinite display curve")};
                            result.parameters.append(parameters[i]);
                        }
                        curves.rgb[channel] = std::move(result);
                    } else {
                        const cmsUInt32Number count = cmsGetToneCurveEstimatedTableEntries(curve);
                        const cmsUInt16Number *table = cmsGetToneCurveEstimatedTable(curve);
                        // A segment-backed curve's estimated table would resample
                        // its definition. Only an original 16-bit table is retained.
                        if (cmsGetToneCurveSegment(0, curve))
                            return {Result::Unsupported, {}, QStringLiteral("Floating-point sampled display curves are unsupported")};
                        if (count < 2 || !table)
                            return {Result::InvalidData, {}, QStringLiteral("Invalid sampled display curve")};
                        if (count > quint64(std::numeric_limits<qint32>::max()) / sizeof(quint16))
                            return {Result::CapacityExceeded, {}, QStringLiteral("Display table exceeds owned byte-array capacity")};
                        Value::Table16Curve result;
                        result.values = QVector<quint16>(table, table + count);
                        curves.rgb[channel] = std::move(result);
                    }
                }
                value.stages.append(std::move(curves));
            } else if (cmsStageType(stage) == cmsSigMatrixElemType) {
                const auto *data = static_cast<const _cmsStageMatrixData *>(cmsStageData(stage));
                if (!data || !data->Double)
                    return {Result::InvalidData, {}, QStringLiteral("Invalid display matrix")};
                Value::Matrix matrix{};
                for (int i = 0; i < 9; ++i) {
                    if (!std::isfinite(data->Double[i]))
                        return {Result::InvalidData, {}, QStringLiteral("Nonfinite display matrix")};
                    matrix.coefficients[i] = data->Double[i];
                }
                for (int i = 0; i < 3; ++i) {
                    const double offset = data->Offset ? data->Offset[i] : 0.0;
                    if (!std::isfinite(offset))
                        return {Result::InvalidData, {}, QStringLiteral("Nonfinite display matrix offset")};
                    matrix.offset[i] = offset;
                }
                value.stages.append(matrix);
            } else if (cmsStageType(stage) == cmsSigCLutElemType) {
                const auto *data = static_cast<const _cmsStageCLutData *>(cmsStageData(stage));
                if (!data || !data->Params || !data->Tab.T || data->HasFloatValues)
                    return {Result::Unsupported, {}, QStringLiteral("Unsupported display CLUT representation")};
                const cmsInterpParams *params = data->Params;
                if (params->nInputs != 3 || params->nOutputs != 3 ||
                    (params->dwFlags & CMS_LERP_FLAGS_TRILINEAR) ||
                    (params->dwFlags & CMS_LERP_FLAGS_FLOAT))
                    return {Result::Unsupported, {}, QStringLiteral("Unsupported display CLUT interpolation")};
                Value::Clut clut;
                quint64 entries = 3;
                for (int i = 0; i < 3; ++i) {
                    if (params->nSamples[i] < 2 || params->nSamples[i] > 255)
                        return {Result::InvalidData, {}, QStringLiteral("Invalid display CLUT grid")};
                    clut.grid[i] = params->nSamples[i];
                    entries *= params->nSamples[i];
                    if (entries > MaximumClutEntries)
                        return {Result::CapacityExceeded, {}, QStringLiteral("Display CLUT exceeds owned value capacity")};
                }
                if (data->nEntries < entries)
                    return {Result::InvalidData, {}, QStringLiteral("Display CLUT table is too short")};
                clut.table = QVector<quint16>(data->Tab.T, data->Tab.T + entries);
                value.stages.append(std::move(clut));
            } else {
                return {Result::Unsupported, {}, QStringLiteral("Unsupported LCMS display stage")};
            }
        }
        return {Result::Success, std::move(value), {}};
    } catch (const std::bad_alloc &) {
        return {Result::CapacityExceeded, {}, QStringLiteral("Cannot allocate display conversion values")};
    }
}
