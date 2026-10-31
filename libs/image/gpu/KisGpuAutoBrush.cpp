/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuAutoBrush_p.h"
#include "KisGpuBrushDabs_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QTransform>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
quint32 floatBits(float value)
{
    quint32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}
void doubleWords(AutoBrushDab &dab, std::size_t index, double value)
{
    quint64 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    dab.words[index] = quint32(bits);
    dab.words[index + 1] = quint32(bits >> 32);
}
int maskSide(qreal metric, qreal subpixel)
{
    if (std::abs(metric - qRound(metric)) < 1e-6) metric = qRound(metric);
    if (subpixel < 1e-6) subpixel = 0;
    return std::max(1, int(std::ceil(metric + subpixel)));
}
bool imageDab(const Brush &brush, const BrushDab &point, QPointF subpixel, AutoBrushDab &dab)
{
    const auto &tips = *brush.autoBrush->imageTips;
    const qreal scale = point.scale * (brush.diameter / tips.originalSize.width());
    qsizetype index = 0;
    while (index + 1 < tips.levels.size() &&
           (tips.levels[index].scale * .5 > scale || std::abs(tips.levels[index].scale * .5 - scale) < 1e-6)) ++index;
    const auto &level = tips.levels[index];
    if (point.imageTipIndex >= quint32(tips.tipCount)) return false;
    qreal angle = std::fmod(point.angle, 2 * M_PI);
    if (angle < 0) angle += 2 * M_PI;
    angle = -angle;
    QTransform transform;
    transform.scale(scale / (qreal(level.size.width()) / tips.originalSize.width()),
                    scale / (qreal(level.size.height()) / tips.originalSize.height()));
    if (!qFuzzyCompare(angle, qreal(0))) {
        transform = transform * QTransform().rotateRadians(angle);
        const auto bounds = transform.mapRect(QRectF(QPointF(), level.size));
        transform = transform * QTransform::fromTranslate(-bounds.x(), -bounds.y());
    }
    transform = transform * QTransform::fromTranslate(subpixel.x(), subpixel.y());
    dab.words[3] |= 256u;
    if (transform.isIdentity()) dab.words[3] |= 512u;
    while (transform.type() == QTransform::TxTranslate) {
        const qreal fakeScale = transform.m11() - 10 * std::numeric_limits<qreal>::epsilon();
        transform *= QTransform::fromScale(fakeScale, fakeScale);
    }
    bool invertible = false;
    const auto inverse = (QTransform::fromTranslate(-1, -1) * transform).inverted(&invertible);
    if (!invertible) return false;
    if (level.size.width() + 2 > 16384 || level.size.height() + 2 > 16384 ||
        dab.words[1] > quint32(PaintGeometryElementLimit / 6)) return false;
    for (const auto value : {inverse.m11() * 65536., inverse.m12() * 65536.})
        if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<qint32>::max()) return false;
    dab.words[16] = quint32(qint32(inverse.m11() * 65536.));
    dab.words[17] = quint32(qint32(inverse.m12() * 65536.));
    dab.words[24] = quint32(level.size.width() + 2); dab.words[25] = quint32(level.size.height() + 2);
    dab.words[26] = level.wordOffset + point.imageTipIndex * level.tipWordStride;
    auto coordinates = QSharedPointer<QList<quint32>>::create(qsizetype(dab.words[1]) * 4, 0);
    for (quint32 y = 0; y < dab.words[1]; ++y) {
        const qreal cy = y + .5;
        qreal left = 0, right = dab.words[0];
        const auto intersect = [&](qreal coefficient, qreal offset, qreal side) {
            if (coefficient == 0) { if (offset < 0 || offset > side) right = left; return; }
            qreal low = -offset / coefficient, high = (side - offset) / coefficient;
            if (low > high) std::swap(low, high);
            left = std::max(left, low); right = std::min(right, high);
        };
        intersect(inverse.m11(), inverse.m21() * cy + inverse.dx(), dab.words[24]);
        intersect(inverse.m12(), inverse.m22() * cy + inverse.dy(), dab.words[25]);
        if (right <= left) continue;
        const auto first = quint32(std::clamp(qRound(left), 0, int(dab.words[0])));
        const auto last = quint32(std::clamp(qRound(right), 0, int(dab.words[0])));
        if (last <= first) continue;
        (*coordinates)[y * 4] = first; (*coordinates)[y * 4 + 1] = last - first;
        (*coordinates)[y * 4 + 2] = quint32(coordinates->size());
        for (quint32 x = first; x < last; x += 2048) {
            const qreal cx = x + .5;
            const qreal fx = (inverse.m21() * cy + inverse.m11() * cx + inverse.dx()) * 65536.;
            const qreal fy = (inverse.m22() * cy + inverse.m12() * cx + inverse.dy()) * 65536.;
            if (std::abs(fx) >= std::numeric_limits<qint32>::max() || std::abs(fy) >= std::numeric_limits<qint32>::max() ||
                coordinates->size() > PaintGeometryElementLimit - 2) return false;
            coordinates->append(quint32(qint32(fx) - 32768));
            coordinates->append(quint32(qint32(fy) - 32768));
        }
    }
    dab.imageCoordinates = coordinates;
    return true;
}
qreal textureStrength(const BrushTexture &texture, const BrushDab &dab)
{
    qreal scaling = 1;
    if (texture.pressure) scaling = pressureValue(*texture.pressure, dab.pressure);
    if (texture.fade) {
        const auto index = texture.fadePeriodic ? dab.sequence % texture.fadeLength :
            std::min(dab.sequence, qsizetype(texture.fadeLength));
        const qreal fade = pressureValue(*texture.fade, qreal(index) / texture.fadeLength);
        if (!texture.pressure) scaling = fade;
        else switch (texture.curveMode) {
        case 1: scaling += fade; break;
        case 2: scaling = std::max(scaling, fade); break;
        case 3: scaling = std::min(scaling, fade); break;
        case 4: scaling = std::abs(scaling - fade); break;
        default: scaling *= fade; break;
        }
    }
    return std::clamp(texture.strength * scaling, qreal(0), qreal(1));
}
}

std::optional<QList<AutoBrushDab>> autoBrushDabs(const PaintOperation &operation, bool pending)
{
    const auto &brush = operation.brush;
    if (!brush.autoBrush) return std::nullopt;
    if (!operation.figureStyle.outline) return QList<AutoBrushDab>{};
    const auto &settings = *brush.autoBrush;
    const quint32 mirrors = quint32(brush.mirrorHorizontal) | (quint32(brush.mirrorVertical) << 1);
    const qsizetype reflectionCount = (brush.mirrorHorizontal ? 2 : 1) * (brush.mirrorVertical ? 2 : 1);
    QList<AutoBrushDab> result;
    qreal averageOpacity = 0;
    std::optional<AutoBrushDab> original;
    QPointF savedSubpixel;
    qreal savedAngle = 0;
    quint32 savedImageTip = 0;
    qsizetype imageCoordinateWords = 0;
    const bool unaliasedGaussian = settings.mask == AutoBrush::Mask::GaussianCircle && !settings.antialiasEdges;
    constexpr double sqrtTwo = 1.41421356237309504880;
    std::array<quint32, 3> gaussianParameters{};
    double gaussianFadeDenominator = 0;
    if (unaliasedGaussian) {
        double fade = 1 - (settings.fade + settings.verticalFade.value_or(settings.fade)) / 2;
        if (fade == 0) fade = 1e-6;
        else if (fade == 1) fade = 1 - 1e-6;
        const double center = (2.5 * (6761 * fade - 10000)) / (sqrtTwo * 6761 * fade);
        gaussianFadeDenominator = 6761 * fade;
        gaussianParameters = {floatBits(float(center)), floatBits(float(255 / (2 * std::erf(center)))),
                              floatBits(float(1 / settings.ratio))};
    }

    const bool complete = forEachBrushDab(operation, pending, [&](const BrushDab &point) {
        if (point.directPixel) {
            AutoBrushDab dab;
            dab.words[0] = dab.words[1] = dab.words[2] = 1;
            dab.words[3] = 64u;
            dab.words[18] = quint32(qint32(point.position.x()));
            dab.words[19] = quint32(qint32(point.position.y()));
            dab.words[20] = dab.words[21] = dab.words[22] = floatBits(1.f);
            dab.words[23] = quint32(settings.creamy);
            result.append(dab);
            return result.size() <= PaintGeometryElementLimit / reflectionCount;
        }
        const auto &metric = point.size;
        const QPointF hotSpot(std::max(qreal(1), metric.width()) / 2, std::max(qreal(1), metric.height()) / 2);
        auto offset = point.position - hotSpot;
        const auto sharpness = settings.sharpnessResponse ?
            std::optional<qreal>(pressureValue(*settings.sharpnessResponse, point.pressure)) : std::nullopt;
        if (sharpness && settings.alignOutlinePixels && settings.sharpnessResponse->strength > 0) {
            if (qFuzzyCompare(*sharpness, qreal(1))) offset = QPointF(qRound(offset.x()), qRound(offset.y()));
            else offset = *sharpness * QPointF(qRound(offset.x()), qRound(offset.y())) + (1 - *sharpness) * offset;
        }
        QPoint origin(int(std::floor(offset.x())), int(std::floor(offset.y())));
        auto subpixel = offset - origin;
        if (point.mirroredH) {
            const auto value = -(point.position.x() + hotSpot.x());
            subpixel.setX(value - std::floor(value));
            origin.setX(qRound(point.position.x() + subpixel.x() + hotSpot.x()) - maskSide(metric.width(), subpixel.x()));
        }
        if (point.mirroredV) {
            const auto value = -(point.position.y() + hotSpot.y());
            subpixel.setY(value - std::floor(value));
            origin.setY(qRound(point.position.y() + subpixel.y() + hotSpot.y()) - maskSide(metric.height(), subpixel.y()));
        }
        const double radius = brush.diameter * point.scale / 2;
        const double fadeStart = std::max(0., radius - 1.);
        const qreal opacity = pressureValue(settings.opacityResponse, point.pressure);
        constexpr float exponent = .1f;
        averageOpacity = averageOpacity < opacity ? opacity : exponent * opacity + (1.0 - exponent) * averageOpacity;
        AutoBrushDab dab;
        dab.words[0] = maskSide(metric.width(), subpixel.x());
        dab.words[1] = maskSide(metric.height(), subpixel.y());
        dab.words[3] = (point.mirroredH ? 4u : 0u) | (point.mirroredV ? 8u : 0u);
        const qreal narrowSide = brush.diameter * settings.ratio * point.scale;
        dab.words[2] = settings.antialiasEdges ? (narrowSide < 1 ? 6 : narrowSide < 10 ? 3 : 1) : 1;
        doubleWords(dab, 4, hotSpot.x() - .5 + subpixel.x());
        doubleWords(dab, 6, hotSpot.y() - .5 + subpixel.y());
        doubleWords(dab, 8, std::cos(point.angle));
        doubleWords(dab, 10, std::sin(point.angle));
        if (settings.imageTips) {
            if (!imageDab(brush, point, subpixel, dab)) return false;
        } else if (settings.mask != AutoBrush::Mask::GaussianCircle) {
            dab.words[3] |= 1u | (quint32(settings.antialiasEdges) << 1) |
                (settings.mask == AutoBrush::Mask::Rectangle ? 16u : 0u);
            doubleWords(dab, 12, 1. / radius);
            doubleWords(dab, 14, settings.fade == 0 ? 1. : 1. / (settings.fade * radius));
            const qreal verticalFade = settings.verticalFade.value_or(settings.fade);
            doubleWords(dab, 24, 2. / narrowSide);
            doubleWords(dab, 26, verticalFade == 0 ? 1. : 2. / (verticalFade * narrowSide));
        } else {
            doubleWords(dab, 12, radius);
            doubleWords(dab, 14, fadeStart);
            doubleWords(dab, 16, 255. / (radius - fadeStart));
        }
        if (unaliasedGaussian) {
            dab.words[3] |= 32u;
            dab.words[24] = gaussianParameters[0];
            dab.words[25] = floatBits(float(sqrtTwo * 12500 / (gaussianFadeDenominator * radius)));
            dab.words[26] = gaussianParameters[1];
            dab.words[27] = gaussianParameters[2];
        }
        // Native dab caching reuses the original tip before applying paper at
        // the new document position. Only geometry is shared; flow, opacity
        // and paper strength still belong to this input sample.
        const int precision = settings.autoPrecision ?
            (brush.texture && std::min(dab.words[0], dab.words[1]) >= 30 ? 3 : 5) : settings.precisionLevel;
        constexpr qreal sizeTolerance[] = {.05, .01, 0, 0, 0};
        constexpr qreal positionTolerance[] = {1, 1, 1, .5, 1e-6};
        const qreal angleTolerance = precision == 5 ? 1e-6 : M_PI / 180;
        const QPoint paperOrigin = origin;
        const bool reuse = original && point.imageTipIndex == savedImageTip &&
            std::abs(point.angle - savedAngle) <= angleTolerance &&
            std::abs(int(dab.words[0]) - int(original->words[0])) <= int(sizeTolerance[precision - 1] * dab.words[0]) &&
            std::abs(int(dab.words[1]) - int(original->words[1])) <= int(sizeTolerance[precision - 1] * dab.words[1]) &&
            std::abs(subpixel.x() - savedSubpixel.x()) <= positionTolerance[precision - 1] &&
            std::abs(subpixel.y() - savedSubpixel.y()) <= positionTolerance[precision - 1] &&
            (dab.words[3] & 12u) == (original->words[3] & 12u);
        if (reuse) {
            origin += (QPoint(int(dab.words[0]), int(dab.words[1])) -
                       QPoint(int(original->words[0]), int(original->words[1]))) / 2;
            dab = *original;
        } else {
            original = dab;
            savedSubpixel = subpixel;
            savedAngle = point.angle;
            savedImageTip = point.imageTipIndex;
        }
        dab.words[18] = quint32(origin.x());
        dab.words[19] = quint32(origin.y());
        // The queued native paintbrush applies paper at the requested rect,
        // then centers the cached bitmap for painting.
        dab.words[32] = quint32(paperOrigin.x() - origin.x());
        dab.words[33] = quint32(paperOrigin.y() - origin.y());
        dab.words[20] = floatBits(float(opacity));
        dab.words[21] = floatBits(float(settings.flow * pressureValue(settings.flowResponse, point.pressure)));
        dab.words[22] = floatBits(float(averageOpacity));
        dab.words[23] = quint32(settings.creamy);
        if (brush.texture) {
            const auto strength = textureStrength(*brush.texture, point);
            dab.words[28] = qRound(strength * 255);
            dab.words[29] = qRound(strength * 65535);
            dab.words[30] = quint32(brush.texture->mode) | (quint32(brush.texture->soft) << 8);
        }
        if (sharpness) {
            const quint32 tolerance = quint32(255 - *sharpness * 255);
            const quint32 transparent = (100 - settings.sharpnessSoftness) * tolerance / 100;
            dab.words[30] |= 512u | (tolerance << 16) | (transparent << 24);
        }
        if (dab.imageCoordinates) {
            if (dab.imageCoordinates->size() > PaintGeometryElementLimit - imageCoordinateWords) return false;
            imageCoordinateWords += dab.imageCoordinates->size();
        }
        result.append(dab);
        return result.size() <= PaintGeometryElementLimit / reflectionCount;
    });
    if (!complete) return std::nullopt;
    const auto originalCount = result.size();
    // The native brush paints the prepared dabs before each reflected batch.
    for (quint32 reflection : {1u, 3u, 2u}) {
        if ((reflection & mirrors) != reflection) continue;
        for (qsizetype i = 0; i < originalCount; ++i) {
            auto mirrored = result[i];
            mirrored.words[31] = reflection;
            if (reflection & 1u) {
                mirrored.words[18] = quint32(2 * brush.mirrorCenter.x() - qint32(mirrored.words[18]) - int(mirrored.words[0]));
                mirrored.words[3] ^= 4u;
            }
            if (reflection & 2u) {
                mirrored.words[19] = quint32(2 * brush.mirrorCenter.y() - qint32(mirrored.words[19]) - int(mirrored.words[1]));
                mirrored.words[3] ^= 8u;
            }
            result.append(mirrored);
        }
    }
    if (brush.additiveMaskingBrush) {
        auto maskOperation = operation;
        maskOperation.brush = *brush.additiveMaskingBrush;
        maskOperation.brush.basicSmoothing = brush.basicSmoothing;
        maskOperation.brush.weightedSmoothing = brush.weightedSmoothing;
        maskOperation.brush.pixelPerfect = brush.pixelPerfect;
        maskOperation.figureStyle.fill.reset();
        maskOperation.figureStyle.pattern.reset();
        const auto mask = autoBrushDabs(maskOperation, pending);
        if (!mask || mask->size() > PaintGeometryElementLimit - result.size()) return std::nullopt;
        for (auto dab : *mask) {
            const auto &secondaryTips = brush.additiveMaskingBrush->autoBrush->imageTips;
            if (secondaryTips && settings.imageTips && secondaryTips != settings.imageTips)
                dab.words[26] += quint32(settings.imageTips->pixels.size() / 4);
            dab.words[3] |= 128u;
            result.append(dab);
        }
    }
    return result;
}
}
