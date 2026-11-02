/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_resources_snapshot.h"
#include <cstring>
#include <new>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoCanvasResourceProvider.h>
#include <klocalizedstring.h>
#include <gpu/KisGpuPaintImage.h>
#include <qbitarray.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <QtGlobal>
#include <resources/KoAbstractGradient.h>
#include <KisDitherMaths.h>
#include <QtEndian>
#include <KoCompositeOpRegistry.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_settings.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/KisPaintopSettingsIds.h>
#include <compositeops/KoAlphaDarkenParamsWrapper.h>
#include <QDomDocument>
#include <resources/KoPattern.h>
#include <KisResourceTypes.h>
#include <KisResourcesInterface.h>
#include <brushengine/KisPerStrokeRandomSource.h>
#include "KisBrushTextureMask.h"
#include <QBuffer>
#include <QFileInfo>
#include <KoCanvasResourcesIds.h>
#include <KoCanvasResourcesInterface.h>
#include <KoResourceCacheInterface.h>
#include <KoResourceLoadResult.h>
#include <kis_brush.h>
#include <kis_gbr_brush.h>
#include <kis_imagepipe_brush.h>
#include <kis_pipebrush_parasite.h>
#include <kis_qimage_pyramid.h>
#include "KoColorSpaceConstants.h"
#include "KoCompositeOpIds.h"
#include "KisGpuBlendMode.h"
#include "KoResourceSignature.h"
#include "filter/kis_filter_configuration.h"
#include "kis_assert.h"
#include "kis_default_bounds.h"
#include "kis_default_bounds_base.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_paint_device.h"
#include "kis_paint_layer.h"
#include "kis_selection_mask.h"
#include "kis_algebra_2d.h"
#include "KisGlobalResourcesInterface.h"
#include "kis_types.h"
#include "kis_cubic_curve.h"
#include <algorithm>
#include <cmath>
#include <utility>

using FrozenGpuImageTips = QSharedPointer<const LibrePaint::Image::Gpu::ImageBrushTips>;
Q_DECLARE_METATYPE(FrozenGpuImageTips)

namespace {
bool captureCurveResponse(QString curve, LibrePaint::Image::Gpu::PressureResponse &response)
{
    using Response = LibrePaint::Image::Gpu::PressureResponse;
    if (curve.isEmpty()) curve = QStringLiteral("0,0;1,1;");
    QList<QPointF> points;
    for (const auto &text : curve.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
        const auto pair = text.split(QLatin1Char(','));
        if (pair.size() != 2) return false;
        bool xOk = false, yOk = false;
        const double x = pair[0].toDouble(&xOk), y = pair[1].toDouble(&yOk);
        if (!xOk || !yOk || !qIsFinite(x) || !qIsFinite(y) || x < 0 || x > 1 || y < 0 || y > 1 ||
            (!points.isEmpty() && x <= points.last().x())) return false;
        points.append({x, y});
    }
    // Native curves extend their endpoint values to the full sensor domain.
    if (points.size() < 2) return false;
    const KisCubicCurve parsed(points);
    if (parsed.isIdentity()) response.kind = Response::Kind::Direct;
    else {
        response.kind = Response::Kind::Curve;
        const auto values = parsed.floatTransfer(256);
        response.curve = QList<qreal>(256, 0);
        std::copy(values.cbegin(), values.cend(), response.curve.begin());
    }
    return true;
}

bool capturePressureResponse(const KisPaintOpSettingsSP &settings, const QString &name,
                             bool checkable, LibrePaint::Image::Gpu::PressureResponse &response,
                             qreal maximumStrength = 1)
{
    using Response = LibrePaint::Image::Gpu::PressureResponse;
    response = {};
    if (checkable && !settings->getBool(QStringLiteral("Pressure") + name)) return true;
    response.strength = settings->getDouble(name + QStringLiteral("Value"), 1);
    if (!qIsFinite(response.strength) || response.strength < 0 || response.strength > maximumStrength) return false;
    if (!settings->getBool(name + QStringLiteral("UseCurve"), true)) return true;
    if (settings->getInt(name + QStringLiteral("curveMode"), 0) != 0) return false;
    const auto definition = settings->getString(name + QStringLiteral("Sensor"));
    QString curve;
    if (!definition.isEmpty()) {
        QDomDocument sensor;
        if (!sensor.setContent(definition)) return false;
        const auto element = sensor.documentElement();
        if (element.attribute(QStringLiteral("id")) != QLatin1String("pressure")) return false;
        curve = element.firstChildElement(QStringLiteral("curve")).text();
    }
    if (settings->getBool(name + QStringLiteral("UseSameCurve"), true))
        curve = settings->getString(name + QStringLiteral("commonCurve"), curve);
    return captureCurveResponse(curve, response);
}

bool captureSizeResponse(const KisPaintOpSettingsSP &settings, LibrePaint::Image::Gpu::Brush &brush)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    brush.sizeSpeedResponse.reset();
    brush.sizeSensor = Gpu::Brush::SizeSensor::Pressure;
    if (capturePressureResponse(settings, QStringLiteral("Size"), true, brush.sizeResponse)) return true;
    if (!settings->getBool("PressureSize") || !settings->getBool("SizeUseCurve", true) ||
        settings->getInt("SizecurveMode", 0) != 0) return false;
    const qreal strength = settings->getDouble("SizeValue", 1);
    if (!qIsFinite(strength) || strength < 0 || strength > 1) return false;
    QDomDocument sensors;
    const auto definition = settings->getString("SizeSensor");
    if (!sensors.setContent(definition)) return false;
    if (sensors.documentElement().attribute("id") == QLatin1String("declination")) {
        auto curve = sensors.documentElement().firstChildElement("curve").text();
        if (settings->getBool("SizeUseSameCurve", true)) curve = settings->getString("SizecommonCurve", curve);
        if (!captureCurveResponse(curve, brush.sizeResponse)) return false;
        brush.sizeResponse.strength = strength;
        brush.sizeSensor = Gpu::Brush::SizeSensor::TiltElevation;
        return true;
    }
    const auto root = sensors.documentElement();
    QList<QDomElement> elements;
    if (root.attribute("id") == QLatin1String("sensorslist")) {
        for (auto child = root.firstChildElement("ChildSensor"); !child.isNull(); child = child.nextSiblingElement("ChildSensor")) elements.append(child);
    } else elements.append(root);
    if (elements.isEmpty()) return false;
    QString common = elements.last().firstChildElement("curve").text();
    const bool legacy = !definition.contains(QLatin1String("curve"));
    if (legacy) common = settings->getBool("CustomSize") ? settings->getString("CurveSize") : QString();
    common = settings->getString("SizecommonCurve", common);
    brush.sizeResponse = {Gpu::PressureResponse::Kind::Constant, strength, {}};
    bool pressure = false;
    for (const auto &element : elements) {
        Gpu::PressureResponse response;
        const auto curve = legacy || settings->getBool("SizeUseSameCurve", true) ? common : element.firstChildElement("curve").text();
        if (!captureCurveResponse(curve, response)) return false;
        if (element.attribute("id") == QLatin1String("pressure")) {
            if (pressure) return false;
            pressure = true;
            response.strength = strength;
            brush.sizeResponse = response;
        } else if (element.attribute("id") == QLatin1String("speed")) {
            if (brush.sizeSpeedResponse) return false;
            brush.sizeSpeedResponse = response;
        } else return false;
    }
    return bool(brush.sizeSpeedResponse);
}

bool captureBrushTexture(const KisPaintOpSettingsSP &settings, bool erase, LibrePaint::Image::Gpu::Brush &brush)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!settings->getBool("Texture/Pattern/Enabled")) return true;
    Gpu::BrushTexture texture;
    switch (settings->getInt("Texture/Pattern/TexturingMode", 0)) {
    case 0: texture.mode = Gpu::BrushTexture::Mode::Multiply; break;
    case 1: texture.mode = Gpu::BrushTexture::Mode::Subtract; break;
    case 4: texture.mode = Gpu::BrushTexture::Mode::Darken; break;
    default: return false;
    }
    texture.soft = settings->getBool("Texture/Pattern/UseSoftTexturing");
    const QString strengthKey = QStringLiteral("Texture/Strength/");
    if (settings->getBool("Pressure" + strengthKey)) {
        texture.strength = settings->getDouble(strengthKey + "Value", 1);
        if (!qIsFinite(texture.strength) || texture.strength < 0 || texture.strength > 1) return false;
        if (settings->getBool(strengthKey + "UseCurve", true)) {
            texture.curveMode = settings->getInt(strengthKey + "curveMode", 0);
            if (texture.curveMode < 0 || texture.curveMode > 4) return false;
            QDomDocument sensors;
            auto definition = settings->getString(strengthKey + "Sensor");
            if (definition.isEmpty()) definition = QStringLiteral("<params id='pressure'/>");
            if (!sensors.setContent(definition)) return false;
            const auto root = sensors.documentElement();
            QList<QDomElement> elements;
            if (root.attribute("id") == QLatin1String("sensorslist")) {
                for (auto child = root.firstChildElement("ChildSensor"); !child.isNull(); child = child.nextSiblingElement("ChildSensor")) elements.append(child);
            } else elements.append(root);
            if (elements.isEmpty()) {
                QDomElement pressure = sensors.createElement("ChildSensor");
                pressure.setAttribute("id", "pressure");
                elements.append(pressure);
            }
            QString common = elements.last().firstChildElement("curve").text();
            const bool legacyCurve = !definition.contains(QLatin1String("curve"));
            if (legacyCurve) common = settings->getBool("Custom" + strengthKey) ? settings->getString("Curve" + strengthKey) : QString();
            common = settings->getString(strengthKey + "commonCurve", common);
            for (const auto &element : elements) {
                Gpu::PressureResponse response;
                auto curve = element.firstChildElement("curve").text();
                if (legacyCurve || settings->getBool(strengthKey + "UseSameCurve", true)) curve = common;
                if (!captureCurveResponse(curve, response)) return false;
                if (element.attribute("id") == QLatin1String("pressure")) {
                    if (texture.pressure) return false;
                    texture.pressure = response;
                } else if (element.attribute("id") == QLatin1String("fade")) {
                    if (texture.fade) return false;
                    bool lengthOk = false;
                    texture.fadeLength = element.attribute("length", "1000").toInt(&lengthOk);
                    texture.fadePeriodic = element.attribute("periodic", "0").toInt() != 0;
                    if (!lengthOk || texture.fadeLength <= 0) return false;
                    texture.fade = response;
                } else return false;
            }
        }
    }
    auto resources = settings->resourcesInterface();
    if (!resources) resources = KisGlobalResourcesInterface::instance();
    auto md5 = settings->getString("Texture/Pattern/PatternMD5Sum");
    if (md5.isEmpty()) md5 = QByteArray::fromBase64(settings->getString("Texture/Pattern/PatternMD5").toLatin1()).toHex();
    const auto file = QFileInfo(settings->getString("Texture/Pattern/PatternFileName")).fileName();
    auto pattern = resources->source<KoPattern>(ResourceType::Patterns).bestMatch(md5, file, settings->getString("Texture/Pattern/Name"));
    if (!pattern && !settings->getString("Texture/Pattern/Pattern").isEmpty()) {
        QBuffer embedded;
        embedded.setData(QByteArray::fromBase64(settings->getString("Texture/Pattern/Pattern").toLatin1()));
        embedded.open(QIODevice::ReadOnly);
        pattern.reset(new KoPattern(file));
        if (!pattern->loadFromDevice(&embedded, resources)) return false;
    }
    if (!pattern || !pattern->valid()) return false;
    KisBrushTextureMaskSettings import;
    import.scale = settings->getDouble("Texture/Pattern/Scale", 1);
    import.brightness = settings->getDouble("Texture/Pattern/Brightness", 0);
    import.contrast = settings->getDouble("Texture/Pattern/Contrast", 1);
    import.neutralPoint = settings->getDouble("Texture/Pattern/NeutralPoint", .5);
    import.cutoffLeft = settings->getInt("Texture/Pattern/CutoffLeft", 0);
    import.cutoffRight = settings->getInt("Texture/Pattern/CutoffRight", 255);
    import.cutoffPolicy = settings->getInt("Texture/Pattern/CutoffPolicy", 0);
    import.invert = settings->getBool("Texture/Pattern/Invert") != (erase && settings->getBool("Texture/Pattern/AutoInvertOnErase"));
    const qreal scale = qFuzzyCompare(import.scale, 0.) ? 1 : import.scale;
    if (!qIsFinite(scale) || scale <= 0 || !qIsFinite(import.brightness) || !qIsFinite(import.contrast) ||
        !qIsFinite(import.neutralPoint) || import.neutralPoint < 0 || import.neutralPoint > 1 ||
        import.cutoffLeft < 0 || import.cutoffRight > 255 || import.cutoffLeft > import.cutoffRight ||
        import.cutoffPolicy < 0 || import.cutoffPolicy > 2 ||
        std::max(qreal(2), pattern->width() * scale) * std::max(qreal(2), pattern->height() * scale) > 16 * 1024 * 1024) return false;
    const auto mask = prepareBrushTextureMask(pattern->pattern(), import);
    const qint64 pixels = qint64(mask.width()) * mask.height();
    if (mask.isNull() || pixels > 16 * 1024 * 1024) return false;
    texture.size = mask.size();
    texture.alpha = QByteArray((pixels + 3) & ~qint64(3), char(0));
    for (int row = 0; row < mask.height(); ++row)
        std::memcpy(texture.alpha.data() + row * mask.width(), mask.constScanLine(row), mask.width());
    KisPerStrokeRandomSource random;
    texture.offset = {
        settings->getBool("Texture/Pattern/isRandomOffsetX") ? random.generate("texture_offset_x", 0, mask.width()) : settings->getInt("Texture/Pattern/OffsetX"),
        settings->getBool("Texture/Pattern/isRandomOffsetY") ? random.generate("texture_offset_y", 0, mask.height()) : settings->getInt("Texture/Pattern/OffsetY")};
    brush.texture = std::move(texture);
    return true;
}

FrozenGpuImageTips captureImageBrushTips(const KisPaintOpSettingsSP &settings, const QDomElement &definition, qreal &diameter)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    auto resources = settings->resourcesInterface();
    if (!resources) resources = KisGlobalResourcesInterface::instance();
    const auto native = KisBrush::fromXMLLoadResult(definition, resources).resource<KisBrush>();
    if (!native || native->brushApplication() != ALPHAMASK) return {};
    diameter = native->userEffectiveSize();
    if (!qIsFinite(diameter) || diameter <= 0) return {};
    QList<QImage> images;
    bool random = false;
    if (const auto pipe = native.dynamicCast<KisImagePipeBrush>()) {
        const auto &parasite = pipe->parasite();
        const auto tips = pipe->brushes();
        if (parasite.dim != 1 || parasite.rank[0] != tips.size() ||
            parasite.selection[0] != KisParasite::Random) return {};
        for (const auto &tip : tips) images.append(tip->brushTipImage());
        random = true;
    } else images.append(native->brushTipImage());
    if (images.isEmpty() || images.first().isNull()) return {};
    const auto size = images.first().size();
    if (size.width() > 8192 || size.height() > 8192) return {};
    QString cacheKey = QStringLiteral("librepaint/gpu/image-tips/") + QString::number(random) + QLatin1Char('/');
    for (const auto &image : images) {
        if (image.size() != size) return {};
        cacheKey += QString::number(image.cacheKey()) + QLatin1Char('/');
    }
    const auto cache = settings->resourceCacheInterface();
    if (cache) {
        const auto cached = cache->fetch(cacheKey).value<FrozenGpuImageTips>();
        if (cached) return cached;
    }
    qint64 requiredBytes = 0;
    for (qreal scale = 8; ; scale *= .5) {
        const QSize scaled = size * scale;
        if (scaled.isEmpty()) break;
        if (scale > 1 && scaled.width() > 512 && scaled.height() > 512) continue;
        requiredBytes += ((qint64(scaled.width() + 2) * (scaled.height() + 2) * 2 + 3) & ~qint64(3)) * images.size();
        if (requiredBytes > 64 * 1024 * 1024) return {};
    }
    auto result = QSharedPointer<Gpu::ImageBrushTips>::create();
    result->originalSize = size; result->tipCount = images.size(); result->randomSelection = random;
    QList<QSharedPointer<KisQImagePyramid>> pyramids;
    for (const auto &image : images) pyramids.append(QSharedPointer<KisQImagePyramid>::create(image));
    qreal previousScale = 0;
    for (qreal scale = 8; ; scale *= .5) {
        qreal actual = 0;
        const auto first = pyramids.first()->getClosest(QTransform::fromScale(scale, scale), &actual);
        if (first.isNull()) return {};
        if (actual == previousScale) {
            if (scale < actual) break;
            continue;
        }
        previousScale = actual;
        const QSize levelSize(first.width() - 2, first.height() - 2);
        const qint64 tipBytes = (qint64(first.width()) * first.height() * 2 + 3) & ~qint64(3);
        if (tipBytes * images.size() > 64 * 1024 * 1024 - result->pixels.size()) return {};
        Gpu::ImageBrushLevel level{levelSize, actual, quint32(result->pixels.size() / 4), quint32(tipBytes / 4)};
        result->levels.append(level);
        for (const auto &pyramid : pyramids) {
            qreal tipScale = 0;
            const auto image = pyramid->getClosest(QTransform::fromScale(scale, scale), &tipScale).convertToFormat(QImage::Format_ARGB32);
            if (tipScale != actual || image.size() != first.size()) return {};
            const auto start = result->pixels.size();
            result->pixels.resize(start + tipBytes);
            std::fill(result->pixels.begin() + start, result->pixels.end(), char(0));
            for (int y = 0; y < image.height(); ++y) {
                const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    const auto offset = start + (y * image.width() + x) * 2;
                    result->pixels[offset] = char(qRed(row[x]));
                    result->pixels[offset + 1] = char(qAlpha(row[x]));
                }
            }
        }
        if (levelSize.width() == 1 || levelSize.height() == 1) break;
    }
    const FrozenGpuImageTips frozen = result;
    if (cache) cache->put(cacheKey, QVariant::fromValue(frozen));
    return frozen;
}

bool captureAutoBrush(const KisPaintOpSettingsSP &settings, LibrePaint::Image::Gpu::Brush &brush, bool secondary = false)
{
    const int action = settings->getInt("PaintOpAction", 1);
    const bool incremental = !secondary && action == 1;
    if ((!secondary && action != 1 && action != 2) ||
        (incremental && (brush.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::Marker ||
                         settings->getBool(KisPaintOpUtils::MaskingBrushEnabledTag))) ||
        settings->getString("ColorSource/Type", "plain") != QLatin1String("plain") ||
        (secondary && settings->getBool(KisPaintOpUtils::MaskingBrushEnabledTag)) ||
        settings->getBool("PaintOpSettings/isAirbrushing") || settings->getBool("PaintOpSettings/ignoreSpacing") ||
        settings->getBool("PaintOpSettings/updateSpacingBetweenDabs") ||
        settings->getBool("HorizontalMirrorEnabled") || settings->getBool("VerticalMirrorEnabled")) return false;
    for (const auto *name : {"PressureDarken", "PressureMirror", "PressureMix", "PressureRate", "PressureRatio",
                            "PressureSoftness",
                            "PressureSpacing", "Pressureh", "Pressures", "Pressurev"})
        if (settings->getBool(QLatin1String(name))) return false;
    const bool checkedRotation = settings->getBool("PressureRotation");
    const bool constantRotation = checkedRotation && !settings->getBool("RotationUseCurve", true);
    if (constantRotation && !settings->getString("RotationSensor").isEmpty()) {
        QDomDocument rotation;
        if (!rotation.setContent(settings->getString("RotationSensor"))) return false;
        QList<QDomElement> sensors{rotation.documentElement()};
        const auto children = rotation.documentElement().elementsByTagName(QStringLiteral("ChildSensor"));
        for (int i = 0; i < children.size(); ++i) sensors.append(children.at(i).toElement());
        for (const auto &sensor : sensors)
            if (sensor.attribute(QStringLiteral("fanCornersEnabled")).toInt() &&
                !sensor.attribute(QStringLiteral("lockedAngleMode")).toInt()) return false;
    }
    QDomDocument definition;
    if (!definition.setContent(settings->getString("brush_definition"))) return false;
    const auto root = definition.documentElement();
    const auto mask = root.firstChildElement(QStringLiteral("MaskGenerator"));
    const bool imageTip = root.attribute("type") == QLatin1String("gbr_brush") || root.attribute("type") == QLatin1String("png_brush");
    if (!imageTip && (root.attribute(QStringLiteral("type")) != QLatin1String("auto_brush") ||
        (mask.attribute(QStringLiteral("id")) != QLatin1String("gauss") &&
         mask.attribute(QStringLiteral("id")) != QLatin1String("default")) ||
        (mask.attribute(QStringLiteral("type")) != QLatin1String("circle") &&
         mask.attribute(QStringLiteral("type")) != QLatin1String("rect")))) return false;
    const auto equalNumber = [](const QDomElement &element, const QString &name, double expected) {
        bool ok = false;
        const double value = element.attribute(name).toDouble(&ok);
        return ok && value == expected;
    };
    if (!imageTip && (!equalNumber(mask, QStringLiteral("spikes"), 2) ||
        !equalNumber(root, QStringLiteral("randomness"), 0) || !equalNumber(root, QStringLiteral("density"), 1))) return false;
    if (!qIsFinite(brush.diameter) || brush.diameter <= 0) return false;
    LibrePaint::Image::Gpu::AutoBrush value;
    value.incremental = incremental;
    value.constantRotation = constantRotation;
    if (checkedRotation && !constantRotation) {
        QDomDocument sensors;
        if (!sensors.setContent(settings->getString("RotationSensor"))) return false;
        const auto sensor = sensors.documentElement();
        const bool tilt = sensor.attribute("id") == QLatin1String("ascension");
        if (!tilt && (sensor.attribute("id") != QLatin1String("drawingangle") || sensor.attribute("lockedAngleMode").toInt())) return false;
        LibrePaint::Image::Gpu::PressureResponse rotation;
        auto curve = sensor.firstChildElement("curve").text();
        if (settings->getBool("RotationUseSameCurve", true)) curve = settings->getString("RotationcommonCurve", curve);
        if (!captureCurveResponse(curve, rotation)) return false;
        if (tilt) {
            rotation.strength = settings->getDouble("RotationValue", 1);
            if (!qIsFinite(rotation.strength) || rotation.strength < 0 || rotation.strength > 1) return false;
            value.tiltDirectionRotation = std::move(rotation);
        } else {
            bool offsetOk = false;
            value.drawingAngleOffset = sensor.attribute("angleOffset", "0").toDouble(&offsetOk);
            if (!offsetOk || !qIsFinite(value.drawingAngleOffset) || std::abs(value.drawingAngleOffset) > 360) return false;
            if (sensor.attribute("fanCornersEnabled").toInt()) {
                bool stepOk = false;
                const int step = sensor.attribute("fanCornersStep", "30").toInt(&stepOk);
                if (!stepOk || step < 1 || step > 360) return false;
                value.fanCornersStep = qreal(step) * M_PI / 180.;
            }
            value.drawingAngleRotation = std::move(rotation);
        }
    }
    if (settings->getBool("PressureScatter")) {
        LibrePaint::Image::Gpu::PressureResponse scatter;
        if (!capturePressureResponse(settings, QStringLiteral("Scatter"), true, scatter, 5)) return false;
        if (settings->hasProperty("Scattering/Amount") && !settings->hasProperty("ScatterValue")) {
            scatter.strength = settings->getDouble("Scattering/Amount");
            if (!qIsFinite(scatter.strength) || scatter.strength < 0 || scatter.strength > 5) return false;
        }
        value.scatter = std::move(scatter);
        value.scatterAxisX = settings->getBool("Scattering/AxisX", true);
        value.scatterAxisY = settings->getBool("Scattering/AxisY", true);
    }
    value.precisionLevel = settings->getInt("KisPrecisionOption/precisionLevel", 5);
    value.autoPrecision = settings->getBool("KisPrecisionOption/AutoPrecisionEnabled", false);
    if (value.precisionLevel < 1 || value.precisionLevel > 5) return false;
    if (settings->getBool("PressureSharpness")) {
        LibrePaint::Image::Gpu::PressureResponse sharpness;
        if (!capturePressureResponse(settings, QStringLiteral("Sharpness"), true, sharpness)) return false;
        value.alignOutlinePixels = settings->getBool("Sharpness/alignoutline");
        value.sharpnessSoftness = settings->getInt("Sharpness/softness");
        if (settings->hasProperty("Sharpness/factor") && !settings->hasProperty("SharpnessValue")) {
            sharpness.strength = settings->getDouble("Sharpness/factor");
            if (!qIsFinite(sharpness.strength) || sharpness.strength < 0 || sharpness.strength > 1) return false;
            value.sharpnessSoftness = int(sharpness.strength * 100);
        }
        if (value.sharpnessSoftness < 0 || value.sharpnessSoftness > 100) return false;
        value.sharpnessResponse = std::move(sharpness);
    }
    value.isotropicSpacing = settings->getBool("Spacing/Isotropic", false);
    if (imageTip) {
        value.mask = LibrePaint::Image::Gpu::AutoBrush::Mask::Image;
        value.imageTips = captureImageBrushTips(settings, root, brush.diameter);
        if (!value.imageTips) return false;
        value.ratio = 1; value.fade = 1; value.verticalFade = 1;
    } else {
    if (mask.attribute(QStringLiteral("id")) == QLatin1String("default")) {
        value.mask = mask.attribute(QStringLiteral("type")) == QLatin1String("rect") ?
            LibrePaint::Image::Gpu::AutoBrush::Mask::Rectangle : LibrePaint::Image::Gpu::AutoBrush::Mask::Circle;
    } else if (mask.attribute(QStringLiteral("type")) != QLatin1String("circle") || brush.diameter < 1) return false;
    bool fadeOk = false, verticalOk = false, ratioOk = false;
    value.fade = mask.attribute(QStringLiteral("hfade")).toDouble(&fadeOk);
    value.verticalFade = mask.attribute(QStringLiteral("vfade")).toDouble(&verticalOk);
    value.ratio = mask.attribute(QStringLiteral("ratio")).toDouble(&ratioOk);
    if (!fadeOk || !verticalOk || !ratioOk || !qIsFinite(value.fade) || value.fade < 0 || value.fade > 1 ||
        !qIsFinite(*value.verticalFade) || *value.verticalFade < 0 || *value.verticalFade > 1 ||
        !qIsFinite(value.ratio) || value.ratio <= 0 || value.ratio > 1) return false;
    if (equalNumber(mask, QStringLiteral("antialiasEdges"), 1)) value.antialiasEdges = true;
    else if (equalNumber(mask, QStringLiteral("antialiasEdges"), 0)) value.antialiasEdges = false;
    else return false;
    if (value.mask == LibrePaint::Image::Gpu::AutoBrush::Mask::GaussianCircle && value.antialiasEdges &&
        (value.fade != 1 || *value.verticalFade != 1 || value.ratio != 1)) return false;
    }
    brush.autoSpacing = root.attribute(QStringLiteral("useAutoSpacing"), QStringLiteral("0")).toInt() != 0;
    bool coefficientOk = false;
    brush.autoSpacingCoefficient = root.attribute(QStringLiteral("autoSpacingCoeff"), QStringLiteral("1")).toDouble(&coefficientOk);
    if (!coefficientOk || !qIsFinite(brush.autoSpacingCoefficient) || brush.autoSpacingCoefficient < 0) return false;
    bool angleOk = false, spacingOk = false;
    value.angle = root.attribute(QStringLiteral("angle")).toDouble(&angleOk);
    brush.spacing = root.attribute(QStringLiteral("spacing")).toDouble(&spacingOk);
    value.flow = settings->getDouble("FlowValue", 1);
    value.creamy = useCreamyAlphaDarken();
    if (!angleOk || !spacingOk || !qIsFinite(value.angle) || !qIsFinite(brush.spacing) ||
        brush.spacing <= 0 || brush.spacing > 1 || !qIsFinite(value.flow) || value.flow < 0 || value.flow > 1) return false;
    if (!captureSizeResponse(settings, brush) ||
        !capturePressureResponse(settings, QStringLiteral("Opacity"), false, value.opacityResponse) ||
        !capturePressureResponse(settings, QStringLiteral("Flow"), false, value.flowResponse)) return false;
    // WASH applies constant opacity once when merging the temporary stroke.
    if (!secondary) value.opacityResponse.strength = 1;
    value.flowResponse.strength = 1;
    brush.autoBrush = std::move(value);
    if (!captureBrushTexture(settings, brush.erase, brush)) return false;
    if (settings->getBool(KisPaintOpUtils::MaskingBrushEnabledTag)) {
        if (settings->maskingBrushCompositeOp() != QLatin1String("linear_dodge")) return false;
        const auto maskSettings = settings->createMaskingSettings();
        if (!maskSettings) return false;
        auto mask = QSharedPointer<LibrePaint::Image::Gpu::Brush>::create(brush);
        mask->autoBrush.reset(); mask->texture.reset();
        mask->rgba = {255, 255, 255, 255};
        mask->diameter = maskSettings->paintOpSize();
        mask->opacity = 1; mask->erase = false; mask->alphaLocked = false;
        mask->maskPainting = true; mask->channelWriteMask = 15;
        mask->blendMode = LibrePaint::Image::Gpu::LayerBlendMode::Normal;
        if (!captureAutoBrush(maskSettings, *mask, true) || mask->texture) return false;
        // Native painters share their random stream in input-call order.
        const auto usesRandom = [](const LibrePaint::Image::Gpu::AutoBrush &tip) {
            return (tip.scatter && (tip.scatterAxisX || tip.scatterAxisY)) ||
                   (tip.imageTips && tip.imageTips->randomSelection);
        };
        if (usesRandom(*brush.autoBrush) && usesRandom(*mask->autoBrush)) return false;
        brush.additiveMaskingBrush = std::move(mask);
    }
    return true;
}

KisGpuBrushResources captureGpuPaintingResources(
    KisImageSP image, KisNodeSP node, KoCanvasResourceProvider *resources, bool withBrushTip)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisGpuBrushResources result;
    const auto reject = [&](const QString &message) {
        result.diagnostic = message;
        return result;
    };
    if (!image || !image->gpuImage() || !node || node->image().data() != image.data() || !resources)
        return reject(i18n("A stroke could not be started on this GPU paint layer."));
    const bool maskView = image->isGpuEditableMaskView(node);
    const KisNodeSP target = image->isGpuTransparencyMaskView(node) ? node->parent() : node;
    if (!target) return reject(i18n("A stroke could not be started on this GPU paint layer."));
    for (const auto &layer : image->gpuImage()->operations().snapshot()->layers)
        if (layer.uuid == target->uuid() && (layer.kind == Gpu::LayerKind::Paint ||
            (maskView && (layer.kind == Gpu::LayerKind::Group || Gpu::isFilterNode(layer.kind))))) result.layer = layer.id;
    if (!result.layer)
        return reject(i18n("A stroke could not be started on this GPU paint layer."));
    if (maskView) {
        result.transparencyMask = true;
    }
    const auto preset = resources->resource(KoCanvasResource::CurrentPaintOpPreset).value<KisPaintOpPresetSP>();
    if (withBrushTip && (!preset || (preset->paintOp().id() != QLatin1String("roundmarker") &&
                                    preset->paintOp().id() != QLatin1String("paintbrush"))))
        return reject(i18n("This brush is not yet available for GPU painting."));
    const QString composite = resources->resource(KoCanvasResource::CurrentEffectiveCompositeOp).toString();
    const bool eraserMode = resources->resource(KoCanvasResource::EraserMode).toBool();
    const bool erase = composite == COMPOSITE_ERASE || eraserMode;
    Gpu::LayerBlendMode blendMode = Gpu::LayerBlendMode::Normal;
    if (erase) {
        // Erase remains coverage removal and never carries a blend mode.
        if (composite != COMPOSITE_ERASE && composite != COMPOSITE_OVER)
            return reject(i18n("This blend mode is not yet available for this document."));
    } else if (const auto mode = Gpu::layerBlendModeFromCompositeOp(composite)) {
        blendMode = *mode;
    } else {
        return reject(i18n("This blend mode is not yet available for this document."));
    }
    auto &brush = result.brush;
    brush.blendMode = blendMode;
    brush.mirrorHorizontal = resources->resource(KoCanvasResource::MirrorHorizontal).toBool();
    brush.mirrorVertical = resources->resource(KoCanvasResource::MirrorVertical).toBool();
    if (brush.mirrorHorizontal || brush.mirrorVertical) {
        const auto center = KisAlgebra2D::relativeToAbsolute(image->mirrorAxesCenter(), image->bounds());
        if (!qIsFinite(center.x()) || !qIsFinite(center.y()) ||
            qAbs(center.x()) > Gpu::OperationDocument::CoordinateLimit ||
            qAbs(center.y()) > Gpu::OperationDocument::CoordinateLimit)
            return reject(i18n("The painting symmetry axis is invalid."));
        brush.mirrorCenter = center.toPoint();
    }
    KoColor color = resources->foregroundColor();
    color.convertTo(maskView ? KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), QString()) : image->colorSpace());
    // Figure or brush coverage owns alpha; canvas opacity is applied separately.
    if (maskView) {
        const auto gray = color.data()[0];
        brush.rgba = {gray, gray, gray, 255};
    } else if (image->gpuImage()->operations().snapshot()->format == Gpu::PixelFormat::Rgba16) {
        quint16 bgra[4];
        std::memcpy(bgra, color.data(), sizeof(bgra));
        brush.rgba = {bgra[2], bgra[1], bgra[0], 65535};
    } else {
        const auto bgra = color.data();
        brush.rgba = {bgra[2], bgra[1], bgra[0], 255};
    }
    if (withBrushTip) brush.diameter = resources->resource(KoCanvasResource::Size).toReal();
    const qreal opacity = resources->resource(KoCanvasResource::Opacity).toReal();
    if (!qIsFinite(opacity) || opacity < 0 || opacity > 1)
        return reject(i18n("The brush opacity is invalid."));
    brush.opacity = opacity;
    brush.erase = erase;
    brush.alphaLocked = !brush.erase && resources->resource(KoCanvasResource::GlobalAlphaLock).toBool();
    if (withBrushTip && preset->paintOp().id() == QLatin1String("roundmarker")) {
        const auto settings = preset->settings();
        brush.spacing = settings->getDouble("spacing", .02);
        brush.autoSpacing = settings->getBool("useAutoSpacing", false);
        brush.autoSpacingCoefficient = settings->getDouble("autoSpacingCoeff", 1);
        if (!qIsFinite(brush.spacing) || brush.spacing <= 0 || brush.spacing > 1 ||
            !qIsFinite(brush.autoSpacingCoefficient) || brush.autoSpacingCoefficient < 0 ||
            !captureSizeResponse(settings, brush))
            return reject(i18n("These brush settings are not yet available for GPU painting."));
    } else if (withBrushTip && preset->paintOp().id() == QLatin1String("paintbrush") &&
               !captureAutoBrush(preset->settings(), brush)) {
        return reject(i18n("These brush settings are not yet available for GPU painting."));
    }
    return result;
}
}

KisGpuBrushResources captureGpuBrushResources(
    KisImageSP image, KisNodeSP node, KoCanvasResourceProvider *resources)
{
    try {
        return captureGpuPaintingResources(image, node, resources, true);
    } catch (const std::bad_alloc &) {
        KisGpuBrushResources result;
        result.diagnostic = i18n("The brush resources could not be prepared within the available memory.");
        return result;
    }
}

KisGpuBrushResources captureGpuFillResources(
    KisImageSP image, KisNodeSP node, KoCanvasResourceProvider *resources)
{
    return captureGpuPaintingResources(image, node, resources, false);
}

std::optional<LibrePaint::Image::Gpu::GradientFill> captureGpuGradientResource(
    KoAbstractGradientSP gradient, const KoColorSpace *colorSpace, int steps, bool dither, QString &diagnostic)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    diagnostic.clear();
    const auto reject = [&](const QString &message) -> std::optional<Gpu::GradientFill> {
        diagnostic = message;
        return std::nullopt;
    };
    const bool gray = colorSpace && colorSpace->colorModelId() == GrayAColorModelID;
    if (!gradient || !gradient->valid() || !colorSpace ||
        (!gray && colorSpace->colorModelId() != RGBAColorModelID) ||
        (colorSpace->colorDepthId() != Integer8BitsColorDepthID && colorSpace->colorDepthId() != Integer16BitsColorDepthID))
        return reject(i18n("No usable gradient is selected for this document."));
    if (steps < 1 || qint64(steps) * 8 + (dither ? 16384 : 0) > Gpu::GradientFill::MaximumBytes)
        return reject(i18n("The selected gradient exceeds the document's resource capacity."));
    try {
        const auto *mixSpace = KoColorSpaceRegistry::instance()->colorSpace(
            colorSpace->colorModelId().id(), Integer16BitsColorDepthID.id(), colorSpace->profile());
        if (!mixSpace) return reject(i18n("The gradient mixing color space is unavailable."));
        Gpu::GradientFill result;
        result.dither = dither;
        result.rgba16.resize(qsizetype(steps) * 8);
        // Match KoCachedGradient's product resolution. A one-pixel region
        // samples its first endpoint; larger regions include both endpoints.
        KoColor color(mixSpace);
        for (int i = 0; i < steps; ++i) {
            gradient->colorAt(color, steps == 1 ? 0.0 : qreal(i) / (steps - 1));
            quint16 channels[4]{};
            std::memcpy(channels, color.data(), mixSpace->pixelSize());
            const std::array<quint16, 4> rgba = gray ?
                std::array<quint16, 4>{channels[0], channels[0], channels[0], channels[1]} :
                std::array<quint16, 4>{channels[2], channels[1], channels[0], channels[3]};
            for (int channel = 0; channel < 4; ++channel)
                qToLittleEndian(rgba[channel], result.rgba16.data() + qsizetype(i) * 8 + channel * 2);
        }
        if (dither) {
            result.noise.resize(64 * 64 * 4);
            for (int i = 0; i < 64 * 64; ++i)
                qToLittleEndian(quint32(KisDitherMaths::mask[i]), result.noise.data() + i * 4);
        }
        return result;
    } catch (const std::bad_alloc &) {
        return reject(i18n("The gradient resource could not be allocated."));
    }
}

std::optional<LibrePaint::Image::Gpu::PatternFill> captureGpuPatternResource(
    KoPatternSP pattern, const KoColorSpace *colorSpace, const QTransform &transform, QString &diagnostic)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    diagnostic.clear();
    const auto reject = [&](const QString &message) -> std::optional<Gpu::PatternFill> {
        diagnostic = message;
        return std::nullopt;
    };
    const bool gray = colorSpace && colorSpace->colorModelId() == GrayAColorModelID &&
        colorSpace->colorDepthId() == Integer8BitsColorDepthID;
    if (!pattern || !pattern->valid() || !colorSpace || (!gray && colorSpace->colorModelId() != RGBAColorModelID))
        return reject(i18n("No usable pattern is selected for this document."));
    try {
        Gpu::PatternFill result;
        if (colorSpace->colorDepthId() == Integer16BitsColorDepthID) result.format = Gpu::PixelFormat::Rgba16;
        else if (colorSpace->colorDepthId() != Integer8BitsColorDepthID)
            return reject(i18n("The pattern color space is not available for this document."));
        const QImage original = pattern->pattern();
        const qint64 count = qint64(original.width()) * original.height();
        const auto stride = Gpu::pixelByteCount(result.format);
        if (original.isNull() || count <= 0 || count > Gpu::PatternFill::MaximumBytes / stride)
            return reject(i18n("The selected pattern exceeds the document's resource capacity."));
        const QImage source = original.convertToFormat(QImage::Format_ARGB32);
        if (source.isNull()) return reject(i18n("The pattern resource could not be allocated."));
        result.size = source.size();
        const auto inverse = transform.inverted();
        result.inverse = {inverse.m11(), inverse.m12(), inverse.m21(), inverse.m22(), inverse.dx(), inverse.dy()};
        result.sampled = transform.type() > QTransform::TxTranslate;
        result.rgba = QByteArray(count * stride, '\0');
        if (!result.isValid()) return reject(i18n("The pattern transform is outside the document's coordinate capacity."));
        // Match the product pattern import: an untagged BGRA8 source uses the
        // registry default profile, while an RGBA8 destination retains raw bytes.
        const auto *sourceSpace = KoColorSpaceRegistry::instance()->rgb8();
        const auto convertedStride = colorSpace->pixelSize();
        QByteArray row(source.width() * convertedStride, '\0');
        for (int y = 0; y < source.height(); ++y) {
            const auto *pixels = source.constScanLine(y);
            if (colorSpace->id() != QLatin1String("RGBA")) {
                sourceSpace->convertPixelsTo(pixels, reinterpret_cast<quint8 *>(row.data()), colorSpace, source.width(),
                    KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
                pixels = reinterpret_cast<const quint8 *>(row.constData());
            }
            auto *destination = result.rgba.data() + qint64(y) * source.width() * stride;
            for (int x = 0; x < source.width(); ++x) {
                const auto *pixel = pixels + x * convertedStride;
                if (gray) {
                    auto *encoded = destination + x * stride;
                    encoded[0] = encoded[1] = encoded[2] = char(pixel[0]);
                    encoded[3] = char(pixel[1]);
                    continue;
                }
                const int component = stride / 4;
                std::memcpy(destination + x * stride, pixel + component * 2, component);
                std::memcpy(destination + x * stride + component, pixel + component, component);
                std::memcpy(destination + x * stride + component * 2, pixel, component);
                std::memcpy(destination + x * stride + component * 3, pixel + component * 3, component);
            }
        }
        return result;
    } catch (const std::bad_alloc &) {
        return reject(i18n("The pattern resource could not be allocated."));
    }
}

struct KisResourcesSnapshot::Private {
    Private()
    {
    }

    KisImageSP image;
    KisDefaultBoundsBaseSP bounds;
    KoColor currentFgColor;
    KoColor currentBgColor;
    KoPatternSP currentPattern {0};
    KoAbstractGradientSP currentGradient;
    KisPaintOpPresetSP currentPaintOpPreset;
    KisNodeSP currentNode {0};
    qreal currentExposure {0.0};
    KisFilterConfigurationSP currentGenerator;
    KisNodeList selectedNodes;
    bool isUsingOtherColor {false};

    QPointF axesCenter;
    bool mirrorMaskHorizontal {false};
    bool mirrorMaskVertical {false};

    qreal opacity {OPACITY_OPAQUE_F};
    QString compositeOpId {COMPOSITE_OVER};
    const KoCompositeOp *compositeOp {0};

    KisPainter::StrokeStyle strokeStyle {KisPainter::StrokeStyleBrush};
    KisPainter::FillStyle fillStyle {KisPainter::FillStyleForegroundColor};
    QTransform fillTransform;

    bool globalAlphaLock {false};
    qreal effectiveZoom {1.0};
    bool presetAllowsLod {false};
    KisSelectionSP selectionOverride;
    bool hasOverrideSelection {false};

    KoCanvasResourcesInterfaceSP globalCanvasResourcesInterface;
};

KisResourcesSnapshot::KisResourcesSnapshot(KisImageSP image, KisNodeSP currentNode, KoCanvasResourcesInterfaceSP resources, KisDefaultBoundsBaseSP bounds, KisNodeList selectedNodes, KisPaintOpPresetSP presetOverride)
    : m_d(new Private())
{
    m_d->image = image;
    if (!bounds) {
        bounds = new KisDefaultBounds(m_d->image);
    }
    m_d->bounds = bounds;
    m_d->globalCanvasResourcesInterface = resources;
    m_d->currentFgColor = resources->resource(KoCanvasResource::ForegroundColor).value<KoColor>();
    m_d->currentBgColor = resources->resource(KoCanvasResource::BackgroundColor).value<KoColor>();
    m_d->currentPattern = resources->resource(KoCanvasResource::CurrentPattern).value<KoPatternSP>();
    if (resources->resource(KoCanvasResource::CurrentGradient).value<KoAbstractGradientSP>()) {
        m_d->currentGradient = resources->resource(KoCanvasResource::CurrentGradient).value<KoAbstractGradientSP>();
        if(m_d->currentGradient) {
            m_d->currentGradient = resources->resource(KoCanvasResource::CurrentGradient).value<KoAbstractGradientSP>()
                    ->cloneAndBakeVariableColors(m_d->globalCanvasResourcesInterface);
        }
    }
    m_d->isUsingOtherColor = resources->resource(KoCanvasResource::UsingOtherColor).toBool();

    /**
     * We should deep-copy the preset, so that long-running actions
     * will have correct brush parameters. Theoretically this cloning
     * can be expensive, but according to measurements, it takes
     * something like 0.1 ms for an average preset.
     */
    if (presetOverride) {
        /// we don't use global resource cache object in this case,
        /// because the passed preset might be not the global one
        m_d->currentPaintOpPreset =
            presetOverride->cloneWithResourcesSnapshot(
                KisGlobalResourcesInterface::instance(),
                m_d->globalCanvasResourcesInterface,
                nullptr);
    } else {
        KisPaintOpPresetSP p = resources->resource(KoCanvasResource::CurrentPaintOpPreset).value<KisPaintOpPresetSP>();
        if (p) {
            KoResourceCacheInterfaceSP cacheInterface = resources->resource(KoCanvasResource::CurrentPaintOpPresetCache).value<KoResourceCacheInterfaceSP>();

            KIS_SAFE_ASSERT_RECOVER(!cacheInterface || p->sanityCheckResourceCacheIsValid(cacheInterface)) {
                cacheInterface.clear();
            }

            m_d->currentPaintOpPreset =
                    p->cloneWithResourcesSnapshot(KisGlobalResourcesInterface::instance(),
                                                  m_d->globalCanvasResourcesInterface,
                                                  cacheInterface);
        }
    }

#ifdef HAVE_THREADED_TEXT_RENDERING_WORKAROUND
    KisPaintOpRegistry::instance()->preinitializePaintOpIfNeeded(m_d->currentPaintOpPreset);
#endif /* HAVE_THREADED_TEXT_RENDERING_WORKAROUND */

    m_d->currentExposure = resources->resource(KoCanvasResource::HdrExposure).toDouble();


    m_d->currentGenerator = resources->resource(KoCanvasResource::CurrentGeneratorConfiguration).value<KisFilterConfiguration*>();
    if (m_d->currentGenerator) {
        m_d->currentGenerator = m_d->currentGenerator->cloneWithResourcesSnapshot();
    }

    QPointF relativeAxesCenter(0.5, 0.5);
    if (m_d->image) {
        relativeAxesCenter = m_d->image->mirrorAxesCenter();
    }
    m_d->axesCenter = KisAlgebra2D::relativeToAbsolute(relativeAxesCenter, m_d->bounds->imageBorderRect());

    m_d->mirrorMaskHorizontal = resources->resource(KoCanvasResource::MirrorHorizontal).toBool();
    m_d->mirrorMaskVertical = resources->resource(KoCanvasResource::MirrorVertical).toBool();

    m_d->opacity = resources->resource(KoCanvasResource::Opacity).toDouble();

    m_d->compositeOpId = resources->resource(KoCanvasResource::CurrentEffectiveCompositeOp).toString();
    setCurrentNode(currentNode);

    m_d->selectedNodes = selectedNodes;

    /**
     * Fill and Stroke styles are not a part of the resource manager
     * so the tools should set them manually
     * TODO: port stroke and fill styles to be a part
     *       of the resource manager
     */
    m_d->strokeStyle = KisPainter::StrokeStyleBrush;
    m_d->fillStyle = KisPainter::FillStyleNone;

    // Erasing and alpha lock don't mix. If both are enabled, erasing takes priority.
    m_d->globalAlphaLock = !resources->resource(KoCanvasResource::EraserMode).toBool()
            && resources->resource(KoCanvasResource::GlobalAlphaLock).toBool();
    m_d->effectiveZoom = resources->resource(KoCanvasResource::EffectiveZoom).toDouble();

    m_d->presetAllowsLod = resources->resource(KoCanvasResource::EffectiveLodAvailability).toBool();
}

KisResourcesSnapshot::KisResourcesSnapshot(KisImageSP image, KisNodeSP currentNode, KisDefaultBoundsBaseSP bounds)
    : m_d(new Private())
{
    m_d->image = image;
    if (!bounds) {
        bounds = new KisDefaultBounds(m_d->image);
    }
    m_d->bounds = bounds;

#ifdef HAVE_THREADED_TEXT_RENDERING_WORKAROUND
    KisPaintOpRegistry::instance()->preinitializePaintOpIfNeeded(m_d->currentPaintOpPreset);
#endif /* HAVE_THREADED_TEXT_RENDERING_WORKAROUND */

    QPointF relativeAxesCenter(0.5, 0.5);
    if (m_d->image) {
        relativeAxesCenter = m_d->image->mirrorAxesCenter();
    }
    m_d->axesCenter = KisAlgebra2D::relativeToAbsolute(relativeAxesCenter, m_d->bounds->imageBorderRect());
    m_d->opacity = OPACITY_OPAQUE_F;

    setCurrentNode(currentNode);

    /**
     * Fill and Stroke styles are not a part of the resource manager
     * so the tools should set them manually
     * TODO: port stroke and fill styles to be a part
     *       of the resource manager
     */
    m_d->strokeStyle = KisPainter::StrokeStyleBrush;
    m_d->fillStyle = KisPainter::FillStyleNone;
}


KisResourcesSnapshot::~KisResourcesSnapshot()
{
    delete m_d;
}

void KisResourcesSnapshot::setupPainter(KisPainter* painter)
{
    painter->setPaintColor(currentFgColor());
    painter->setBackgroundColor(currentBgColor());
    painter->setGenerator(m_d->currentGenerator);
    painter->setPattern(m_d->currentPattern);
    painter->setGradient(m_d->currentGradient);

    QBitArray lockflags = channelLockFlags();
    if (lockflags.size() > 0) {
        painter->setChannelFlags(lockflags);
    }

    painter->setOpacityF(m_d->opacity);
    painter->setCompositeOpId(m_d->compositeOpId);
    painter->setMirrorInformation(m_d->axesCenter, m_d->mirrorMaskHorizontal, m_d->mirrorMaskVertical);

    painter->setStrokeStyle(m_d->strokeStyle);
    painter->setFillStyle(m_d->fillStyle);
    painter->setPatternTransform(m_d->fillTransform);


    /**
     * The paintOp should be initialized the last, because it may
     * ask the painter for some options while initialization
     */
    painter->setPaintOpPreset(m_d->currentPaintOpPreset, m_d->currentNode, m_d->image);
}

void KisResourcesSnapshot::setupMaskingBrushPainter(KisPainter *painter)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(painter->device());
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->currentPaintOpPreset->hasMaskingPreset());

    painter->setPaintColor(KoColor(Qt::white, painter->device()->colorSpace()));
    painter->setBackgroundColor(KoColor(Qt::black, painter->device()->colorSpace()));

    painter->setOpacityToUnit();
    painter->setChannelFlags(QBitArray());

    // masked brush always paints in indirect mode
    painter->setCompositeOpId(COMPOSITE_ALPHA_DARKEN);

    painter->setMirrorInformation(m_d->axesCenter, m_d->mirrorMaskHorizontal, m_d->mirrorMaskVertical);

    painter->setStrokeStyle(m_d->strokeStyle);

    /**
     * The paintOp should be initialized the last, because it may
     * ask the painter for some options while initialization
     */
    painter->setPaintOpPreset(m_d->currentPaintOpPreset->createMaskingPreset(),
                              m_d->currentNode, m_d->image);
}

KisPostExecutionUndoAdapter* KisResourcesSnapshot::postExecutionUndoAdapter() const
{
    return m_d->image ? m_d->image->postExecutionUndoAdapter() : 0;
}

void KisResourcesSnapshot::setCurrentNode(KisNodeSP node)
{
    m_d->currentNode = node;
}

void KisResourcesSnapshot::setStrokeStyle(KisPainter::StrokeStyle strokeStyle)
{
    m_d->strokeStyle = strokeStyle;
}

void KisResourcesSnapshot::setFillStyle(KisPainter::FillStyle fillStyle)
{
    m_d->fillStyle = fillStyle;
}

void KisResourcesSnapshot::setFillTransform(QTransform transform)
{
    m_d->fillTransform = transform;
}

KisNodeList KisResourcesSnapshot::selectedNodes() const
{
    return m_d->selectedNodes;
}

KisNodeSP KisResourcesSnapshot::currentNode() const
{
    return m_d->currentNode;
}

KisImageSP KisResourcesSnapshot::image() const
{
    return m_d->image;
}

bool KisResourcesSnapshot::needsIndirectPainting() const
{
    return !m_d->currentPaintOpPreset->settings()->paintIncremental();
}

QString KisResourcesSnapshot::indirectPaintingCompositeOp() const
{
    return m_d->currentPaintOpPreset ?
            m_d->currentPaintOpPreset->settings()->indirectPaintingCompositeOp()
              : COMPOSITE_ALPHA_DARKEN;
}

bool KisResourcesSnapshot::needsMaskingBrushRendering() const
{
    return m_d->currentPaintOpPreset && m_d->currentPaintOpPreset->hasMaskingPreset();
}

KisSelectionSP KisResourcesSnapshot::activeSelection() const
{
    /**
     * It is possible to have/use the snapshot without the image. Such
     * usecase is present for example in the scratchpad.
     */
    if (m_d->hasOverrideSelection) {
        return m_d->selectionOverride;
    }

    KisSelectionSP selection = m_d->image ? m_d->image->globalSelection() : 0;

    KisLayerSP layer;
    KisSelectionMaskSP mask;
    if((layer = qobject_cast<KisLayer*>(m_d->currentNode.data()))) {
         selection = layer->selection();
    } else if ((mask = dynamic_cast<KisSelectionMask*>(m_d->currentNode.data())) &&
               mask->selection() == selection) {

         selection = 0;
    }

    return selection;
}

bool KisResourcesSnapshot::needsAirbrushing() const
{
    return (   m_d->currentPaintOpPreset
            && m_d->currentPaintOpPreset->settings()
            && m_d->currentPaintOpPreset->settings()->isAirbrushing());
}

qreal KisResourcesSnapshot::airbrushingInterval() const
{
    return (   m_d->currentPaintOpPreset
            && m_d->currentPaintOpPreset->settings()
            && m_d->currentPaintOpPreset->settings()->airbrushInterval());
}

bool KisResourcesSnapshot::needsSpacingUpdates() const
{
    return (   m_d->currentPaintOpPreset
            && m_d->currentPaintOpPreset->settings()
            && m_d->currentPaintOpPreset->settings()->useSpacingUpdates());
}

void KisResourcesSnapshot::setOpacity(qreal opacity)
{
    m_d->opacity = opacity;
}

void KisResourcesSnapshot::setMirroring(bool horizontal, bool vertical)
{
    m_d->mirrorMaskHorizontal = horizontal;
    m_d->mirrorMaskVertical = vertical;
}

qreal KisResourcesSnapshot::opacity() const
{
    return m_d->opacity;
}

QString KisResourcesSnapshot::compositeOpId() const
{
    return m_d->compositeOpId;
}

KoPatternSP KisResourcesSnapshot::currentPattern() const
{
    return m_d->currentPattern;
}

KoResourceSignature KisResourcesSnapshot::currentPatternSignature() const
{
    return m_d->currentPattern ? m_d->currentPattern->signature() : KoResourceSignature();
}

KoColor KisResourcesSnapshot::currentFgColor() const
{
    if (m_d->isUsingOtherColor) {
        // temporarily swap colors if requested by user
        return m_d->currentBgColor;
    } else {
        return m_d->currentFgColor;
    }
}

KoColor KisResourcesSnapshot::currentBgColor() const
{
    if (m_d->isUsingOtherColor) {
        // temporarily swap colors if requested by user
        return m_d->currentFgColor;
    } else {
        return m_d->currentBgColor;
    }
}

KisPaintOpPresetSP KisResourcesSnapshot::currentPaintOpPreset() const
{
    return m_d->currentPaintOpPreset;
}

KoResourceSignature KisResourcesSnapshot::currentPaintOpPresetSignature() const
{
    return m_d->currentPaintOpPreset ? m_d->currentPaintOpPreset->signature() : KoResourceSignature();
}

QTransform KisResourcesSnapshot::fillTransform() const
{
    return m_d->fillTransform;
}

KoAbstractGradientSP KisResourcesSnapshot::currentGradient() const
{
    return m_d->currentGradient;
}

KoResourceSignature KisResourcesSnapshot::currentGradientSignature() const
{
    return m_d->currentGradient ? m_d->currentGradient->signature() : KoResourceSignature();
}

KisFilterConfigurationSP KisResourcesSnapshot::currentGenerator() const
{
    return m_d->currentGenerator;
}

bool KisResourcesSnapshot::isUsingOtherColor() const
{
    return m_d->isUsingOtherColor;
}

QBitArray KisResourcesSnapshot::channelLockFlags() const
{
    QBitArray channelFlags;
    KisPaintLayer *paintLayer;
    if ((paintLayer = dynamic_cast<KisPaintLayer*>(m_d->currentNode.data()))) {

        channelFlags = paintLayer->channelLockFlags();
        if (m_d->globalAlphaLock) {
            if (channelFlags.isEmpty()) {
                channelFlags = paintLayer->colorSpace()->channelFlags(true, true);
            }

            channelFlags &= paintLayer->colorSpace()->channelFlags(true, false);
        }
    }
    return channelFlags;
}

qreal KisResourcesSnapshot::effectiveZoom() const
{
    return m_d->effectiveZoom;
}

bool KisResourcesSnapshot::presetAllowsLod() const
{
    return m_d->presetAllowsLod;
}

bool KisResourcesSnapshot::presetNeedsAsynchronousUpdates() const
{
    return m_d->currentPaintOpPreset && m_d->currentPaintOpPreset->settings()->needsAsynchronousUpdates();
}

void KisResourcesSnapshot::setFGColorOverride(const KoColor &color)
{
    m_d->currentFgColor = color;
}

void KisResourcesSnapshot::setBGColorOverride(const KoColor &color)
{
    m_d->currentBgColor = color;
}

void KisResourcesSnapshot::setSelectionOverride(KisSelectionSP selection)
{
    m_d->selectionOverride = selection;
    m_d->hasOverrideSelection = true; // needed if selection passed is null to ignore selection
}

void KisResourcesSnapshot::setBrush(const KisPaintOpPresetSP &brush)
{
    m_d->currentPaintOpPreset =
        brush->cloneWithResourcesSnapshot(
            KisGlobalResourcesInterface::instance(),
            m_d->globalCanvasResourcesInterface,
            nullptr);

#ifdef HAVE_THREADED_TEXT_RENDERING_WORKAROUND
    KisPaintOpRegistry::instance()->preinitializePaintOpIfNeeded(m_d->currentPaintOpPreset);
#endif /* HAVE_THREADED_TEXT_RENDERING_WORKAROUND */
}
