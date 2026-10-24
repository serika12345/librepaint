/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestDocument.h"
#include <simpletest.h>
#include <Document.h>
#include <InfoObject.h>
#include <Node.h>
#include <KisGlobalResourcesInterface.h>
#include <KisImageBarrierLock.h>
#include <KoCanvasResourceProvider.h>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpRegistry.h>
#include <KoResourceBundle.h>
#include <application/kis_config.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_figure_painting_stroke.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_properties_configuration.h>
#include <kis_random_source.h>
#include <gpu/KisGpuPaintImage.h>
#include <metadata/KoDocumentInfo.h>
#include <document/KisDocument.h>
#include <kundo2stack.h>
#include <kundo2magicstring.h>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <array>
#include <KoPointerEvent.h>
#include <kis_painting_information_builder.h>
#include <kis_smoothing_options.h>
#include <tool/kis_tool_freehand_helper.h>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <cstring>

namespace {
constexpr int side = 128;

class TimedFreehandHelper final : public KisToolFreehandHelper
{
public:
    using KisToolFreehandHelper::KisToolFreehandHelper;

    void setElapsedStrokeTime(int time) { m_time = time; }

    enum PrimitiveKind { Point, Line, Curve };
    struct Primitive {
        PrimitiveKind kind;
        KisPaintInformation first;
        KisPaintInformation last;
        QPointF control1;
        QPointF control2;
    };
    void recordPrimitives() { m_recordPrimitives = true; }
    const QList<Primitive> &primitives() const { return m_primitives; }

protected:
    int elapsedStrokeTime() const override { return m_time; }
    void paintAt(const KisPaintInformation &pi) override
    {
        if (m_recordPrimitives) m_primitives.append({Point, pi, pi, {}, {}});
        KisToolFreehandHelper::paintAt(pi);
    }
    void paintLine(const KisPaintInformation &first, const KisPaintInformation &last) override
    {
        if (m_recordPrimitives) m_primitives.append({Line, first, last, {}, {}});
        KisToolFreehandHelper::paintLine(first, last);
    }
    void paintBezierCurve(const KisPaintInformation &first, const QPointF &control1,
                          const QPointF &control2, const KisPaintInformation &last) override
    {
        if (m_recordPrimitives) m_primitives.append({Curve, first, last, control1, control2});
        KisToolFreehandHelper::paintBezierCurve(first, control1, control2, last);
    }

private:
    int m_time = 0;
    bool m_recordPrimitives = false;
    QList<Primitive> m_primitives;
};

KisPaintLayerSP paintLayer(KisDocument *document)
{
    return dynamic_cast<KisPaintLayer *>(document->image()->root()->firstChild().data());
}

QByteArray layerPixels(KisDocument *document)
{
    QByteArray bytes(side * side * 4, '\0');
    paintLayer(document)->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 0, 0, side, side);
    return bytes;
}

bool setInitialLayerIdentity(KisDocument *document, const QUuid &uuid)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisImageSP image = document->image();
    KisPaintLayerSP layer = paintLayer(document);
    if (!layer || layer->uuid().isNull() || uuid.isNull()) return false;
    const auto owner = image->gpuImage();
    if (!owner) {
        layer->setUuid(uuid);
        return layer->uuid() == uuid;
    }
    const auto before = owner->operations().snapshot();
    if (before->layers.size() != 1 || before->size != image->size() ||
        before->activeLayer != before->layers.first().id || owner->operations().hasActiveStroke() ||
        owner->operations().undoCount() || owner->operations().redoCount()) return false;
    const auto &initial = before->layers.first();
    if (initial.uuid != layer->uuid() || initial.kind != Gpu::LayerKind::Paint ||
        !initial.parentUuid.isNull() || !initial.operations.isEmpty()) return false;
    const auto pixels = layerPixels(document);
    if (pixels != QByteArray(pixels.size(), '\0')) return false;
    Gpu::RasterLayer input{initial.name, initial.visible, uuid, {}, initial.defaultPixel, initial.opacity,
                          initial.userLocked, initial.pinnedToTimeline, initial.channelWriteMask};
    input.position = initial.position;
    input.kind = initial.kind;
    input.parentUuid = initial.parentUuid;
    input.colorLabelIndex = initial.colorLabelIndex;
    input.projectionChannelMask = initial.projectionChannelMask;
    input.passThrough = initial.passThrough;
    input.blendMode = initial.blendMode;
    Gpu::OperationDocument fixed(Gpu::RasterImage{before->size, {input}, 0, before->projectionBackground}, owner->limits());
    if (owner->restoreRecovery(fixed.recoveryData()) != Gpu::Result::Success) return false;
    layer = paintLayer(document);
    const auto after = owner->operations().snapshot();
    if (!layer || after->layers.size() != 1) return false;
    const auto &actual = after->layers.first();
    return actual.uuid == uuid && layer->uuid() == uuid && actual.name == initial.name &&
        actual.visible == initial.visible && actual.defaultPixel == initial.defaultPixel &&
        actual.opacity == initial.opacity && actual.userLocked == initial.userLocked &&
        actual.pinnedToTimeline == initial.pinnedToTimeline && actual.channelWriteMask == initial.channelWriteMask &&
        actual.position == initial.position && actual.kind == initial.kind && actual.parentUuid == initial.parentUuid &&
        actual.colorLabelIndex == initial.colorLabelIndex && actual.projectionChannelMask == initial.projectionChannelMask &&
        actual.passThrough == initial.passThrough && actual.blendMode == initial.blendMode && actual.operations.isEmpty() &&
        after->size == before->size && after->projectionBackground == before->projectionBackground &&
        after->activeLayer == actual.id && !owner->operations().hasActiveStroke() &&
        !owner->operations().undoCount() && !owner->operations().redoCount() && layerPixels(document) == pixels;
}

KisPaintOpPresetSP defaultBrush(qreal flow, QString &diagnostic)
{
    const QString relative = QStringLiteral("/bundles/Krita_4_Default_Resources.bundle");
    QString path = qEnvironmentVariable("KIS_TEST_PREFIX_PATH") + QStringLiteral("/share/krita") + relative;
    if (!QFileInfo::exists(path)) path = QStringLiteral(SYSTEM_RESOURCES_DATA_DIR) + relative;
    KoResourceBundle bundle(path);
    QByteArray bytes;
    QBuffer output(&bytes);
    output.open(QIODevice::WriteOnly);
    if (!bundle.load() || !bundle.exportResource(ResourceType::PaintOpPresets,
            QStringLiteral("b)_Basic-5_Size_Opacity.kpp"), &output)) {
        diagnostic = QStringLiteral("Cannot load the shipped default brush from %1").arg(path);
        return {};
    }
    // The tracked Krita 4 bundle supplies this exact preset and its pressure
    // curves. Its existing resource provenance also owns the thumbnail.
    if (QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() !=
        QByteArrayLiteral("bd0610171db6d3eca006e7b12697d7baf3868ef34cbf3b1e792592d2f0957939")) {
        diagnostic = QStringLiteral("The shipped default brush definition changed");
        return {};
    }
    KisPaintOpPresetSP preset(new KisPaintOpPreset(QStringLiteral("default-brush-contract.kpp")));
    QBuffer input(&bytes);
    input.open(QIODevice::ReadOnly);
    if (!preset->loadFromDevice(&input, KisGlobalResourcesInterface::instance())) {
        diagnostic = QStringLiteral("Cannot parse the shipped default brush");
        return {};
    }
    preset->settings()->setProperty("FlowValue", flow);
    return preset;
}

QByteArray partialAlphaBackground()
{
    QByteArray background(side * side * 4, '\0');
    const std::array<int, 8> alphas{{0, 1, 2, 63, 127, 128, 254, 255}};
    for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x) {
        auto *pixel = background.data() + 4 * (y * side + x);
        pixel[0] = char((x * 17 + y * 3) % 256);
        pixel[1] = char((x * 5 + y * 11) % 256);
        pixel[2] = char((x * 7 + y * 13) % 256);
        pixel[3] = char(alphas[(x / 16 + y / 16) % alphas.size()]);
    }
    return background;
}

bool fixInformation(KisDocument *document)
{
    QDomDocument info;
    if (!info.setContent(QByteArrayLiteral("<document-info><about><title>Default brush contract</title>"
        "<creation-date>2000-01-02T03:04:05Z</creation-date><date>2001-02-03T04:05:06Z</date>"
        "<editing-cycles>7</editing-cycles><editing-time>10</editing-time><language>en</language></about>"
        "<author><full-name>LibrePaint fixture</full-name></author></document-info>")) ||
        !document->documentInfo()->load(info)) return false;
    document->documentInfo()->setAuthorInfo("creator", QStringLiteral("LibrePaint fixture"));
    document->setColorHistoryColors({});
    document->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    document->setPreActivatedNode(paintLayer(document));
    return true;
}

bool save(KisDocument *document, const QDir &out, const QString &name)
{
    QString lastStatus;
    const auto statusConnection = QObject::connect(document, &KisDocument::statusBarMessage,
        document, [&](const QString &text, int) { lastStatus = text; });
    const auto disconnectStatus = qScopeGuard([&] { QObject::disconnect(statusConnection); });
    const auto failed = [&](const char *stage) {
        qWarning() << "Default brush save failed" << stage << out.filePath(name)
                   << document->errorMessage() << lastStatus
                   << "image idle" << document->image()->isIdle()
                   << "saving" << document->isSaving();
        return false;
    };
    if (!fixInformation(document)) return failed("document information");
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    document->setModified(true);
    {
        // Hold the observed image version through save's event processing and clone.
        KisImageReadOnlyBarrierLock imageLock(document->image());
        if (!document->saveAs(out.filePath(name + QStringLiteral(".kra")), KisDocument::nativeFormatMimeType(), false, settings))
            return failed("KRA start");
    }
    document->waitForSavingToComplete();
    Document api(document, false);
    InfoObject options;
    options.setProperty("quality", 90);
    options.setProperty("forceSRGB", false);
    options.setProperty("saveSRGBProfile", true);
    options.setProperty("alpha", true);
    options.setProperty("compression", 3);
    options.setProperty("storeMetaData", true);
    options.setProperty("storeAuthor", false);
    options.setProperty("exif", true);
    options.setProperty("iptc", true);
    options.setProperty("xmp", true);
    for (const auto &extension : {QStringLiteral("png"), QStringLiteral("jpg")})
        if (!api.exportImage(out.filePath(name + QLatin1Char('.') + extension), options))
            return failed(qPrintable(extension));
    QFile raw(out.filePath(name + QStringLiteral(".bgra")));
    const auto bytes = layerPixels(document);
    if (!raw.open(QIODevice::WriteOnly) || raw.write(bytes) != bytes.size()) return failed("raw pixels");
    return true;
}

bool paint(KisDocument *document, KisPaintOpPresetSP preset, qreal opacity, int profile, int stroke, int calls,
           bool basicSmoothing = false, int eraser = 0, bool weightedSmoothing = false, bool pixelPerfect = false)
{
    KisImageSP image = document->image();
    const auto layer = paintLayer(document);
    KoCanvasResourceProvider resources;
    KisViewManager::initializeResourceManager(&resources);
    resources.setResource(KoCanvasResource::CurrentKritaNode, QVariant::fromValue(KisNodeSP(layer)));
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(preset));
    resources.setResource(KoCanvasResource::Opacity, opacity);
    resources.setResource(KoCanvasResource::CurrentCompositeOp, eraser == 2 ? COMPOSITE_ERASE : COMPOSITE_OVER);
    resources.setResource(KoCanvasResource::EraserMode, eraser == 1);
    resources.setResource(KoCanvasResource::MirrorHorizontal, false);
    resources.setResource(KoCanvasResource::MirrorVertical, false);
    resources.setResource(KoCanvasResource::HdrExposure, qreal(0));
    resources.setResource(KoCanvasResource::EffectiveZoom, qreal(1));
    if (weightedSmoothing || pixelPerfect) resources.setResource(KoCanvasResource::DisablePressure, true);
    const auto color = stroke == 0 ? QColor(17, 34, 51) : stroke == 1 ? QColor(201, 29, 117) : QColor(83, 177, 41);
    resources.setForegroundColor(KoColor(color, image->colorSpace()));
    resources.setBackgroundColor(KoColor(Qt::white, image->colorSpace()));
    const auto offset = stroke == 0 ? QPointF() : stroke == 1 ? QPointF(-3.25, 4.5) : QPointF(.375, -.25);
    const std::array<std::array<qreal, 3>, 3> pressures{{{{.12, .75, 1}}, {{.92, .68, .17}}, {{.12, .91, .18}}}};
    const QList<QPointF> positions{QPointF(12.25, 13.75), QPointF(60.125, 31.5), QPointF(105.5, 91.25)};
    KisRandomSourceSP random(new KisRandomSource(1789));
    QList<KisPaintInformation> points;
    for (int i = 0; i < positions.size(); ++i) {
        KisPaintInformation point(positions[i] + offset, pressures[profile][i]);
        point.setCurrentTime(i * 16);
        point.setRandomSource(random);
        points.append(point);
    }
    if (calls < 0) {
        auto path = positions;
        if (calls == -2) path = path.first(1);
        else if (calls == -3) path = path.first(2);
        else if (calls == -4) path.append(path.first());
        else if (calls == -5) path = {path.first(), path.first(), path.first()};
        else if (calls == -6) path = {path.first(), path.first(), path.last()};
        else if (calls == -7) path = {path.first(), path[1], path.first()};
        else if (calls == -8 || calls == -9) path = QList<QPointF>(calls == -8 ? 2 : 8, path.first());
        else if (calls == -10 || calls == -11) path.append(QPointF(40.5, 102.25));
        const bool weightedMoving = weightedSmoothing && calls <= -12;
        if (weightedMoving) {
            path = {{10, 10}, {12, 18}, {18, 14}, {22, 8}, {28, 15}, {32, 11}};
            if (calls == -13) path[1] = path[0];
        }
        // Same raw mouse recipe as KisGpuGaussianPixelPerfectFixture_p.h.
        if (pixelPerfect) {
            switch (calls) {
            case -15: path = {{12.2, 12.7}}; break;
            case -16: case -17: case -18:
                path = QList<QPointF>(calls == -16 ? 2 : calls == -17 ? 3 : 8, QPointF(12.2, 12.7));
                break;
            case -19: path = {{4, 4}, {12.2, 12.7}, {13.2, 12.3}, {13.8, 13.8}}; break;
            case -20: path = {{4, 4}, {12.2, 12.7}, {13.2, 12.3}, {20.8, 20.8}}; break;
            case -21: path = {{300, 300}, {12.2, 12.7}}; break;
            case -22: path = {{4, 4}, {-.2, 12.7}, {20.8, 20.8}}; break;
            default: return false;
            }
        }
        const bool pixelPointOracle = pixelPerfect && !image->gpuImage() &&
            (calls == -16 || calls == -17 || calls == -18 || calls == -21 ||
             (calls == -19 && stroke == 0));
        // Legacy CPU zero-LINE blanks lose the intended singleton. Compare
        // singleton inputs to a normal centered POINT, keeping their original
        // full pixel input on the GPU and the real CPU path for tap/moving.
        // Fractional offsets make the held-corner's later strokes move.
        if (pixelPointOracle) path = {path[1]};
        KisPaintingInformationBuilder builder;
        KisPaintingInformationSettings settings;
        settings.useEventTimestamps = true;
        if (weightedSmoothing || pixelPerfect) {
            settings.maximumSpeed = 40;
            settings.speedSmoothingSamples = 1;
        }
        builder.setSettings(settings);
        if (weightedSmoothing || pixelPerfect) builder.reset();
        auto *options = new KisSmoothingOptions(false);
        options->setSmoothingType(pixelPerfect ?
            (pixelPointOracle ? KisSmoothingOptions::NO_SMOOTHING : KisSmoothingOptions::PIXEL_PERFECT) :
            weightedSmoothing ? KisSmoothingOptions::WEIGHTED_SMOOTHING :
            basicSmoothing ? KisSmoothingOptions::SIMPLE_SMOOTHING : KisSmoothingOptions::NO_SMOOTHING);
        if (weightedSmoothing) {
            options->setSmoothnessDistanceMin(24);
            options->setSmoothnessDistanceMax(calls == -14 ? 48 : 24);
            options->setUseScalableDistance(false);
            options->setTailAggressiveness(.01);
            options->setSmoothPressure(calls == -14);
        }
        TimedFreehandHelper helper(&builder, &resources, kundo2_i18n("Default brush input contract"), options);
        QStringList errors;
        QObject::connect(&helper, &KisToolFreehandHelper::requestErrorMessage, &helper,
                         [&](const QString &message) { errors.append(message); });
        const auto before = layerPixels(document);
        bool initialUnchanged = false;
        const QList<int> times{7, 41, 79, 113, 157, 211};
        constexpr std::array<int, 8> pixelTimes{{7, 41, 79, 113, 157, 211, 263, 317}};
        const QList<qreal> pressures{1, .8, .6, .4, .2, .1};
        const QPointingDevice device(QStringLiteral("Weighted contract stylus"), 1,
            QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 1);
        namespace Gpu = LibrePaint::Image::Gpu;
        QList<Gpu::InputSample> expectedInput;
        const auto gpu = image->gpuImage();
        const auto flush = [&] {
            if (!QMetaObject::invokeMethod(&helper, "finishStroke", Qt::DirectConnection)) return false;
            const auto once = gpu ? gpu->operations().recoveryData() : QByteArray();
            if (!QMetaObject::invokeMethod(&helper, "finishStroke", Qt::DirectConnection)) return false;
            return !gpu || gpu->operations().recoveryData() == once;
        };
        for (qsizetype i = 0; i < path.size(); ++i) {
            const int inputIndex = pixelPointOracle ? 1 : int(i);
            const int elapsed = pixelPerfect ? pixelTimes[inputIndex] : weightedMoving ? times[i] : i * 16;
            helper.setElapsedStrokeTime(elapsed);
            auto point = path[i] + offset;
            // Offset first: snapping a loaded document's continued stroke is
            // not translation invariant for fractional offsets.
            if (pixelPointOracle) point = {std::floor(point.x()) + .5, std::floor(point.y()) + .5};
            const auto consume = [&](KoPointerEvent &pointer) {
                if (!i) {
                    helper.initPaint(&pointer, point, image, layer, image.data());
                    initialUnchanged = layerPixels(document) == before;
                } else helper.paintEvent(&pointer);
                if ((weightedSmoothing || pixelPerfect) && gpu) {
                    if (!helper.isRunning() || !errors.isEmpty()) return false;
                    const auto observed = builder.hover(point, &pointer, true);
                    expectedInput.append({point, builder.pressureToCurve(pointer.pressure()), pointer.xTilt(),
                        pointer.yTilt(), pointer.rotation(), quint64(elapsed), observed.drawingSpeed()});
                    const auto preview = gpu->operations().preview();
                    if (!preview || preview->layers.isEmpty() || preview->layers.first().operations.isEmpty()) return false;
                    const auto operation = std::get<Gpu::PaintSnapshot>(preview->layers.first().operations.last());
                    const auto actualInput = operation->inputSamples();
                    if (actualInput != expectedInput) return false;
                    if (pixelPerfect) {
                        if (!operation->brush.gaussianWash || !operation->brush.pixelPerfect ||
                            operation->brush.basicSmoothing || operation->brush.weightedSmoothing) return false;
                    } else if (!operation->brush.basicSmoothing || !operation->brush.weightedSmoothing ||
                        !(*operation->brush.weightedSmoothing == Gpu::WeightedSmoothing{24, calls == -14 ? 48. : 24., .01, calls == -14}))
                        return false;
                    for (qsizetype sample = 0; sample < actualInput.size(); ++sample) {
                        if (actualInput[sample].position.x() != expectedInput[sample].position.x() ||
                            actualInput[sample].position.y() != expectedInput[sample].position.y() ||
                            actualInput[sample].pressure != expectedInput[sample].pressure ||
                            actualInput[sample].timestamp != expectedInput[sample].timestamp) {
                            qWarning() << "GPU input position changed" << sample
                                       << actualInput[sample].position << expectedInput[sample].position;
                            return false;
                        }
                    }
                }
                return true;
            };
            if (calls == -14) {
                QTabletEvent event(i ? QEvent::TabletMove : QEvent::TabletPress, &device, point, point,
                    pressures[i], 0, 0, 0, 0, 0, Qt::NoModifier,
                    i ? Qt::NoButton : Qt::LeftButton, Qt::LeftButton);
                event.setTimestamp(100 + i * 10);
                KoPointerEvent pointer(&event, point);
                if (!consume(pointer)) return false;
            } else {
                QMouseEvent event(i ? QEvent::MouseMove : QEvent::MouseButtonPress, point, point,
                                  i ? Qt::NoButton : Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                event.setTimestamp(pixelPerfect ? 100 + inputIndex * 10 :
                    weightedMoving ? 100 + i * 10 : 1000 + i * 16);
                KoPointerEvent pointer(&event, point);
                if (!consume(pointer)) return false;
            }
            if ((calls == -10 && i == 1) || (calls == -11 && i == 2)) {
                if (!QMetaObject::invokeMethod(&helper, "finishStroke", Qt::DirectConnection)) return false;
            }
            if (calls == -13 && i == 4 && !flush()) return false;
        }
        if (calls == -13 && !flush()) return false;
        if ((weightedSmoothing || pixelPerfect) && gpu) {
            Gpu::OperationDocument restored(QSize(1, 1));
            const auto record = gpu->operations().recoveryData();
            if (restored.restoreRecovery(record) != Gpu::Result::Success || restored.recoveryData() != record ||
                !restored.hasActiveStroke() || std::get<Gpu::PaintSnapshot>(restored.preview()->layers.first().operations.last())->inputSamples() != expectedInput)
                return false;
        }
        helper.endPaint();
        image->waitForDone();
        if (!errors.isEmpty()) qWarning() << "Default brush input rejected" << errors;
        return initialUnchanged && !helper.isRunning() && errors.isEmpty();
    }
    {
        KisFigurePaintingStroke operation(kundo2_i18n("Default brush contract"), image, layer, &resources,
            KisFigurePaintingOptions::StrokeStyleForeground, KisFigurePaintingOptions::FillStyleNone);
        if (calls == 0) {
            for (int i = 1; i < points.size(); ++i) operation.paintLine(points[i - 1], points[i]);
        } else if (calls >= 5) {
            const QPointF firstPoint = QPointF(12.25, 13.75) + offset;
            QPainterPath first(firstPoint);
            if (calls == 6)
                first.cubicTo(QPointF(114.125, -15.75) + offset, QPointF(-26.375, 115.25) + offset, firstPoint);
            else
                first.cubicTo(QPointF(18.5, 48.75) + offset, QPointF(86.125, 5.25) + offset, QPointF(105.5, 91.25) + offset);
            QPainterPath second(QPointF(18.125, 91.5) + offset);
            second.cubicTo(QPointF(21.75, 43.125) + offset, QPointF(108.25, 120.375) + offset,
                           QPointF(112.5, 78.25) + offset);
            if (calls <= 6) operation.paintPainterPath(first);
            else if (calls == 7) { first.addPath(second); operation.paintPainterPath(first); }
            else if (calls == 8) { operation.paintPainterPath(first); operation.paintPainterPath(second); }
            else {
                operation.paintLine(KisPaintInformation(firstPoint, .2), KisPaintInformation(QPointF(105.5, 25.75) + offset, .8));
                operation.paintPainterPath(second);
                operation.paintLine(KisPaintInformation(QPointF(24.25, 48.125) + offset, .9),
                                    KisPaintInformation(QPointF(101.5, 53.75) + offset, .4));
            }
        } else {
            QPainterPath first(QPointF(12.25, 13.75) + offset);
            first.lineTo(QPointF(105.5, 25.75) + offset);
            QPainterPath second(QPointF(18.125, 91.5) + offset);
            second.lineTo(QPointF(112.5, 78.25) + offset);
            QPainterPath third(QPointF(24.25, 48.125) + offset);
            third.lineTo(QPointF(101.5, 53.75) + offset);
            if (calls == 1) {
                first.addPath(second);
                operation.paintPainterPath(first);
            } else if (calls == 2) {
                operation.paintPainterPath(first);
                operation.paintPainterPath(second);
            } else if (calls == 3) {
                operation.paintLine(KisPaintInformation(first.pointAtPercent(0), .2), KisPaintInformation(first.pointAtPercent(1), .8));
                operation.paintPainterPath(second);
                operation.paintLine(KisPaintInformation(third.pointAtPercent(0), .9), KisPaintInformation(third.pointAtPercent(1), .4));
            } else {
                operation.paintPainterPath(first);
                operation.paintLine(KisPaintInformation(second.pointAtPercent(0), .2), KisPaintInformation(second.pointAtPercent(1), .8));
                operation.paintPainterPath(third);
            }
        }
    }
    image->waitForDone();
    return true;
}
}

void TestDocument::testDefaultBrushMouseInputKeepsFixedTimeThroughRecovery_data()
{
    QTest::addColumn<bool>("weighted");
    QTest::addColumn<bool>("idleFlush");
    QTest::newRow("gpu-none") << false << false;
    QTest::newRow("cpu-weighted-end") << true << false;
    QTest::newRow("cpu-weighted-duplicate-idle") << true << true;
}

void TestDocument::testDefaultBrushMouseInputKeepsFixedTimeThroughRecovery()
{
    QFETCH(bool, weighted);
    QFETCH(bool, idleFlush);
    qInfo() << "Fixed time input platform" << QGuiApplication::platformName();
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    KisImageConfig imageConfig(false);
    const auto previousThreads = imageConfig.maxNumberOfThreads();
    const auto restore = qScopeGuard([&] {
        if (weighted) imageConfig.setMaxNumberOfThreads(previousThreads);
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", weighted ? "0" : "1");
    if (weighted) imageConfig.setMaxNumberOfThreads(4);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Fixed time input contract"), side, side, cs,
        KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    KisImageSP image = document->image();
    const auto layer = paintLayer(document.data());
    const auto owner = image->gpuImage();
    QVERIFY(layer);
    QCOMPARE(bool(owner), !weighted);
    QString diagnostic;
    const auto preset = defaultBrush(.57, diagnostic);
    QVERIFY2(preset, qPrintable(diagnostic));
    KoCanvasResourceProvider resources;
    KisViewManager::initializeResourceManager(&resources);
    resources.setResource(KoCanvasResource::CurrentKritaNode, QVariant::fromValue(KisNodeSP(layer)));
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(preset));
    resources.setResource(KoCanvasResource::Opacity, qreal(.57));
    resources.setResource(KoCanvasResource::CurrentCompositeOp, COMPOSITE_OVER);
    resources.setResource(KoCanvasResource::EraserMode, false);
    resources.setResource(KoCanvasResource::MirrorHorizontal, false);
    resources.setResource(KoCanvasResource::MirrorVertical, false);
    resources.setForegroundColor(KoColor(QColor(17, 34, 51), cs));
    KisPaintingInformationBuilder builder;
    KisPaintingInformationSettings settings;
    settings.useEventTimestamps = true;
    settings.maximumSpeed = 40;
    settings.speedSmoothingSamples = 1;
    builder.setSettings(settings);
    builder.reset();
    auto *options = new KisSmoothingOptions(false);
    options->setSmoothingType(weighted ? KisSmoothingOptions::WEIGHTED_SMOOTHING : KisSmoothingOptions::NO_SMOOTHING);
    if (weighted) {
        options->setSmoothnessDistanceMin(24);
        options->setSmoothnessDistanceMax(24);
        options->setUseScalableDistance(false);
        options->setTailAggressiveness(.01);
        options->setSmoothPressure(false);
    }
    TimedFreehandHelper helper(&builder, &resources, kundo2_i18n("Fixed time input contract"), options);
    QStringList errors;
    QObject::connect(&helper, &KisToolFreehandHelper::requestErrorMessage, &helper,
                     [&](const QString &message) { errors.append(message); });
    if (weighted) {
        helper.recordPrimitives();
        const QList<int> times{7, 41, 79, 113, 157, 211};
        QList<QPointF> positions{{10, 10}, {12, 18}, {18, 14}, {22, 8}, {28, 15}, {32, 11}};
        if (idleFlush) positions[1] = positions[0];
        const auto before = layerPixels(document.data());
        const int initialHistory = document->undoStack()->index();
        QJsonArray boundaries;
        bool flushesValid = true;
        const auto flush = [&](const char *phase) {
            const auto count = helper.primitives().size();
            const bool first = QMetaObject::invokeMethod(&helper, "finishStroke", Qt::DirectConnection);
            const auto once = helper.primitives().size();
            const bool second = QMetaObject::invokeMethod(&helper, "finishStroke", Qt::DirectConnection);
            boundaries.append(QJsonObject{{"phase", QString::fromLatin1(phase)}, {"before", int(count)},
                                          {"once", int(once)}, {"twice", int(helper.primitives().size())}});
            return first && second && once == count + 1 && helper.primitives().size() == once;
        };
        for (qsizetype i = 0; i < positions.size(); ++i) {
            helper.setElapsedStrokeTime(times[i]);
            const auto point = positions[i];
            QMouseEvent event(i ? QEvent::MouseMove : QEvent::MouseButtonPress, point, point,
                              i ? Qt::NoButton : Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            event.setTimestamp(100 + i * 10);
            KoPointerEvent pointer(&event, point);
            if (!i) helper.initPaint(&pointer, point, image, layer, image.data());
            else helper.paintEvent(&pointer);
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QLatin1Char('\n'))));
            QVERIFY(helper.isRunning());
            boundaries.append(QJsonObject{{"phase", "input"}, {"index", int(i)},
                                          {"primitives", int(helper.primitives().size())}});
            // Flush after weighted position filtering has started.
            if (idleFlush && i == 4) flushesValid &= flush("idle");
        }
        if (idleFlush) flushesValid &= flush("tail");
        const auto beforeEnd = helper.primitives().size();
        helper.endPaint();
        image->waitForDone();
        const auto painted = layerPixels(document.data());
        boundaries.append(QJsonObject{{"phase", "end"}, {"before", int(beforeEnd)},
                                      {"primitives", int(helper.primitives().size())}});

        // Record interpolation values for diagnosing shared-control differences.
        const auto bits = [](qreal value) {
            const double number = value;
            quint64 raw;
            static_assert(sizeof(raw) == sizeof(number));
            std::memcpy(&raw, &number, sizeof(raw));
            return QString::number(raw, 16).rightJustified(16, QLatin1Char('0'));
        };
        const auto info = [&](const KisPaintInformation &pi) {
            return QJsonArray{bits(pi.pos().x()), bits(pi.pos().y()), bits(pi.currentTime()),
                              bits(pi.pressure()), bits(pi.xTilt()), bits(pi.yTilt()),
                              bits(pi.rotation()), bits(pi.tangentialPressure()),
                              bits(pi.perspective()), bits(pi.drawingSpeed())};
        };
        QJsonArray primitives, inputs;
        for (qsizetype i = 0; i < positions.size(); ++i)
            inputs.append(QJsonObject{{"x", bits(positions[i].x())}, {"y", bits(positions[i].y())},
                                      {"elapsed", times[i]}, {"eventTimestamp", int(100 + i * 10)}});
        for (const auto &primitive : helper.primitives())
            primitives.append(QJsonObject{{"kind", int(primitive.kind)},
                {"first", info(primitive.first)}, {"last", info(primitive.last)},
                {"control1", QJsonArray{bits(primitive.control1.x()), bits(primitive.control1.y())}},
                {"control2", QJsonArray{bits(primitive.control2.x()), bits(primitive.control2.y())}}});
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QDir destination(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path()) +
            QStringLiteral("/fixed-time/") + QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(destination.path()));
        const auto retainFailure = qScopeGuard([&] {
            if (QTest::currentTestFailed()) {
                temporary.setAutoRemove(false);
                qInfo() << "Weighted input artifacts at" << destination.path();
            }
        });
        QFile trace(destination.filePath(QStringLiteral("input.json")));
        QVERIFY(trace.open(QIODevice::WriteOnly));
        const auto data = QJsonDocument(QJsonObject{{"platform", QGuiApplication::platformName()}, {"input", inputs}, {"boundaries", boundaries},
            {"primitives", primitives}, {"infoOrder", "x,y,time,pressure,xTilt,yTilt,rotation,tangentialPressure,perspective,speed"},
            {"paintedSha256", QString::fromLatin1(QCryptographicHash::hash(painted, QCryptographicHash::Sha256).toHex())}}).toJson();
        QCOMPARE(trace.write(data), qint64(data.size()));
        trace.close();
        QFile pixels(destination.filePath(QStringLiteral("painted.bgra")));
        QVERIFY(pixels.open(QIODevice::WriteOnly));
        QCOMPARE(pixels.write(painted), qint64(painted.size()));
        pixels.close();

        // Fixed CPU output of this elapsed-time mouse input before sharing curve controls.
        const auto expectedPixels = idleFlush ?
            QByteArrayLiteral("09b141425f64557c9178665198da3e7fd09c784448c2e73a393494129d8cb4cc") :
            QByteArrayLiteral("f1b6d3b71965ac268c49f2086d258ee3927f80415f1e7c83b855df819855d64d");
        QCOMPARE(QCryptographicHash::hash(painted, QCryptographicHash::Sha256).toHex(), expectedPixels);
        QVERIFY(errors.isEmpty());
        QVERIFY(!helper.isRunning());
        QVERIFY(flushesValid);
        QCOMPARE(helper.primitives().size(), qsizetype(5));
        const QList<int> firstTimes = idleFlush ? QList<int>{41, 41, 79, 113, 157} : QList<int>{7, 41, 79, 113, 157};
        const QList<int> lastTimes = idleFlush ? QList<int>{79, 79, 113, 157, 211} : QList<int>{41, 79, 113, 157, 211};
        for (qsizetype i = 0; i < helper.primitives().size(); ++i) {
            const auto &primitive = helper.primitives()[i];
            QCOMPARE(int(primitive.kind), int(idleFlush && !i ? TimedFreehandHelper::Line : TimedFreehandHelper::Curve));
            QCOMPARE(primitive.first.currentTime(), qreal(firstTimes[i]));
            QCOMPARE(primitive.last.currentTime(), qreal(lastTimes[i]));
            for (const auto &pi : {primitive.first, primitive.last}) {
                const int index = times.indexOf(int(pi.currentTime()));
                QVERIFY(index >= 0);
                QCOMPARE(pi.pressure(), qreal(1));
                QCOMPARE(pi.xTilt(), qreal(0));
                QCOMPARE(pi.yTilt(), qreal(0));
                QCOMPARE(pi.rotation(), qreal(0));
                QCOMPARE(pi.tangentialPressure(), qreal(0));
                QCOMPARE(pi.perspective(), qreal(1));
                if (index < 4) QCOMPARE(pi.pos(), positions[index]);
                else {
                    QVERIFY(std::isfinite(pi.pos().x()) && std::isfinite(pi.pos().y()));
                    QVERIFY(pi.pos().x() >= 10 && pi.pos().x() < positions[index].x());
                    QVERIFY(pi.pos().y() >= 8 && pi.pos().y() <= 18);
                }
            }
            if (primitive.kind == TimedFreehandHelper::Curve) {
                QVERIFY(std::isfinite(primitive.control1.x()) && std::isfinite(primitive.control1.y()));
                QVERIFY(std::isfinite(primitive.control2.x()) && std::isfinite(primitive.control2.y()));
            }
        }
        QVERIFY(painted != before);
        QCOMPARE(document->undoStack()->index(), initialHistory + 1);
        document->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(layerPixels(document.data()), before);
        document->undoStack()->redo();
        image->waitForDone();
        QCOMPARE(layerPixels(document.data()), painted);
        return;
    }
    const QList<Gpu::InputSample> expected{{{10, 10}, 1, 0, 0, 0, 7, 0},
                                         {{20, 10}, 1, 0, 0, 0, 41, .0125},
                                         {{30, 10}, 1, 0, 0, 0, 79, .025}};
    const auto samples = [](const Gpu::Snapshot &version) {
        return std::get<Gpu::PaintSnapshot>(version->layers.first().operations.last())->inputSamples();
    };
    for (qsizetype i = 0; i < expected.size(); ++i) {
        helper.setElapsedStrokeTime(int(expected[i].timestamp));
        const auto point = expected[i].position;
        QMouseEvent event(i ? QEvent::MouseMove : QEvent::MouseButtonPress, point, point,
                          i ? Qt::NoButton : Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        event.setTimestamp(100 + i * 10);
        KoPointerEvent pointer(&event, point);
        if (!i) helper.initPaint(&pointer, point, image, layer, image.data());
        else helper.paintEvent(&pointer);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QLatin1Char('\n'))));
        QVERIFY(helper.isRunning());
        const auto pending = samples(owner->operations().preview());
        QCOMPARE(pending.size(), i + 1);
        QCOMPARE(pending.last().timestamp, expected[i].timestamp);
        QCOMPARE(pending.last().speed, expected[i].speed);
        QCOMPARE(pending, expected.first(i + 1));
    }
    Gpu::OperationDocument pendingRecovery(QSize(1, 1));
    QCOMPARE(pendingRecovery.restoreRecovery(owner->operations().recoveryData()), Gpu::Result::Success);
    QVERIFY(pendingRecovery.hasActiveStroke());
    QCOMPARE(samples(pendingRecovery.preview()), expected);
    helper.endPaint();
    image->waitForDone();
    QVERIFY(errors.isEmpty());
    QVERIFY(!helper.isRunning());
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(samples(owner->operations().snapshot()), expected);
    Gpu::OperationDocument committedRecovery(QSize(1, 1));
    QCOMPARE(committedRecovery.restoreRecovery(owner->operations().recoveryData()), Gpu::Result::Success);
    QVERIFY(!committedRecovery.hasActiveStroke());
    QCOMPARE(samples(committedRecovery.snapshot()), expected);
}

void TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles_data()
{
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("profile");
    QTest::addColumn<bool>("newDocument");
    QTest::addColumn<int>("calls");
    QTest::addColumn<int>("eraser");
    QTest::addColumn<bool>("basicSmoothing");
    QTest::addColumn<bool>("weightedSmoothing");
    QTest::addColumn<bool>("pixelPerfect");
    for (bool fresh : {false, true}) for (qreal opacity : {qreal(1), qreal(.57)}) {
        for (int profile = 0; profile < 3; ++profile) {
            const auto name = QStringLiteral("%1-opacity-%2-pressure-%3").arg(fresh ? "new" : "opened").arg(opacity).arg(profile);
            QTest::newRow(qPrintable(name)) << opacity << qreal(1) << profile << fresh << 0 << 0 << false << false << false;
        }
        const auto name = QStringLiteral("%1-opacity-%2-half-flow").arg(fresh ? "new" : "opened").arg(opacity);
        QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << 0 << 0 << false << false << false;
        for (int calls = 1; calls <= 9; ++calls) {
            const auto kind = calls == 1 ? "compound-path" : calls == 2 ? "separate-paths" :
                              calls == 3 ? "line-path-line" : calls == 4 ? "path-line-path" :
                              calls == 5 ? "cubic-open" : calls == 6 ? "cubic-closed" :
                              calls == 7 ? "cubic-compound" : calls == 8 ? "cubic-separate" : "line-cubic-line";
            const auto group = QStringLiteral("%1-%2-opacity-%3").arg(fresh ? "new" : "opened").arg(QLatin1String(kind)).arg(opacity);
            QTest::newRow(qPrintable(group)) << opacity << qreal(.5) << 2 << fresh << calls << 0 << false << false << false;
        }
    }
    const std::array<const char *, 9> kinds{{"move", "tap", "two", "closed", "stationary", "duplicate-first", "return",
                                          "stationary-two", "stationary-eight"}};
    for (bool fresh : {false, true}) for (qreal opacity : {qreal(1), qreal(.57)}) {
        for (std::size_t i = 0; i < kinds.size(); ++i) {
            const auto name = QStringLiteral("%1-freehand-%2-opacity-%3").arg(fresh ? "new" : "opened").arg(QLatin1String(kinds[i])).arg(opacity);
            QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << -int(i + 1) << 0 << false << false << false;
        }
    }
    for (bool fresh : {false, true}) for (int eraser : {1, 2}) {
        const std::array<int, 8> calls{{0, 5, 6, 9, 0, 0, 0, 0}};
        const std::array<qreal, 8> flow{{.5, .5, .5, 1, 0, 1, 1, 1}};
        const std::array<qreal, 8> opacity{{.57, .57, 1, 1, 1, 0, 126.5 / 255., 1.5 / 255.}};
        const std::array<const char *, 8> figures{{"line", "cubic-open", "cubic-closed", "mixed",
                                                 "zero-flow", "zero-opacity", "half-opacity-byte", "low-opacity-byte"}};
        for (std::size_t i = 0; i < calls.size(); ++i) {
            const auto name = QStringLiteral("%1-erase-%2-%3").arg(fresh ? "new" : "opened").arg(eraser).arg(QLatin1String(figures[i]));
            QTest::newRow(qPrintable(name)) << opacity[i] << flow[i] << 2 << fresh << calls[i] << eraser << false << false << false;
        }
        for (qreal opacity : {qreal(1), qreal(.57)}) {
            for (std::size_t i = 0; i < kinds.size(); ++i) {
                const auto name = QStringLiteral("%1-erase-%2-freehand-%3-opacity-%4").arg(fresh ? "new" : "opened")
                    .arg(eraser).arg(QLatin1String(kinds[i])).arg(opacity);
                    QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << -int(i + 1) << eraser << false << false << false;
            }
        }
    }
    const std::array<const char *, 11> basicKinds{{"tap", "two", "curve", "closed", "stationary3", "duplicate-first", "return",
                                                "idle-two-continue", "idle-curve-continue", "stationary2", "stationary8"}};
    const std::array<int, 11> basicCalls{{-2, -3, -1, -4, -5, -6, -7, -10, -11, -8, -9}};
    for (bool fresh : {false, true}) for (int eraser : {0, 1, 2}) {
        for (std::size_t i = 0; i < basicKinds.size(); ++i) {
            const auto name = QStringLiteral("%1-basic-%2-erase-%3").arg(fresh ? "new" : "opened")
                .arg(QLatin1String(basicKinds[i])).arg(eraser);
            QTest::newRow(qPrintable(name)) << qreal(.57) << qreal(.5) << 2 << fresh << basicCalls[i] << eraser << true << false << false;
        }
    }
    for (bool fresh : {false, true}) for (int eraser : {0, 1, 2}) {
        for (int calls : {-12, -13, -14}) {
            const auto kind = calls == -12 ? "mouse-end" : calls == -13 ? "mouse-duplicate-idle" : "tablet-variable";
            const auto name = QStringLiteral("%1-weighted-%2-erase-%3").arg(fresh ? "new" : "opened")
                .arg(QLatin1String(kind)).arg(eraser);
            QTest::newRow(qPrintable(name)) << qreal(.57) << qreal(.57) << 2 << fresh << calls << eraser << false << true << false;
        }
    }
    const std::array<const char *, 8> pixelKinds{{"tap", "static2", "static3", "static8", "held-corner",
                                                "flushed-corner", "initial-tile-removed", "negative-floor"}};
    for (bool fresh : {false, true}) for (std::size_t i = 0; i < pixelKinds.size(); ++i) {
        for (int eraser : {0, 1, 2}) {
            // Singleton completion and moving coverage connect both erase entries.
            if (eraser && i != 4 && i != 5) continue;
            const auto name = QStringLiteral("%1-pixel-%2-erase-%3").arg(fresh ? "new" : "opened")
                .arg(QLatin1String(pixelKinds[i])).arg(eraser);
            QTest::newRow(qPrintable(name)) << qreal(.57) << qreal(.57) << 2 << fresh << -int(i + 15)
                << eraser << false << false << true;
        }
    }
}

void TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles()
{
    QFETCH(qreal, opacity);
    QFETCH(qreal, flow);
    QFETCH(int, profile);
    QFETCH(bool, newDocument);
    QFETCH(int, calls);
    QFETCH(int, eraser);
    QFETCH(bool, basicSmoothing);
    QFETCH(bool, weightedSmoothing);
    QFETCH(bool, pixelPerfect);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    KisImageConfig imageConfig(false);
    const auto previousThreads = imageConfig.maxNumberOfThreads();
    KisConfig config(false);
    const auto previousCompression = config.readEntry<bool>("compressLayers", true);
    const auto restore = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
        imageConfig.setMaxNumberOfThreads(previousThreads);
        config.writeEntry("compressLayers", previousCompression);
    });
    imageConfig.setMaxNumberOfThreads(4);
    config.writeEntry("compressLayers", true);
    QString diagnostic;
    const auto preset = defaultBrush(flow, diagnostic);
    QVERIFY2(preset, qPrintable(diagnostic));
    QCOMPARE(preset->paintOp().id(), QStringLiteral("paintbrush"));
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QDir destination(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path()) +
        QStringLiteral("/default-brush/") + QString::fromLatin1(QTest::currentDataTag()));
    QVERIFY(QDir().mkpath(destination.path()));
    const auto retainFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) { temporary.setAutoRemove(false); qInfo() << "Default brush artifacts at" << destination.path(); }
    });
    QScopedPointer<KisDocument> documents[2];
    const auto create = [&](int owner) {
        qputenv("LIBREPAINT_GPU_ENGINE", owner ? "1" : "0");
        documents[owner].reset(KisPart::instance()->createDocument());
        documents[owner]->setFileBatchMode(true);
        documents[owner]->setAutoSaveActive(false);
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        return documents[owner]->newImage(QStringLiteral("Default brush observation"), side, side, cs,
            KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1);
    };
    QVERIFY(create(0));
    auto cpuLayer = paintLayer(documents[0].data());
    QVERIFY(cpuLayer);
    QVERIFY(!cpuLayer->uuid().isNull());
    const QUuid inputUuid(newDocument ? QStringLiteral("{33b00002-1122-3344-5566-778899aabbcc}") :
                                       QStringLiteral("{33b00001-1122-3344-5566-778899aabbcc}"));
    QVERIFY(setInitialLayerIdentity(documents[0].data(), inputUuid));
    cpuLayer->setName(QStringLiteral("Paint"));
    const auto initializePixels = [&](KisDocument *document) {
        if (!eraser) return true;
        QScopedPointer<Node> node(Node::createNode(document->image(), paintLayer(document)));
        const auto background = partialAlphaBackground();
        if (!node->setPixelData(background, 0, 0, side, side)) return false;
        document->image()->waitForDone();
        return layerPixels(document) == background;
    };
    QVERIFY(initializePixels(documents[0].data()));
    if (newDocument) {
        QVERIFY(create(1));
        QVERIFY(paintLayer(documents[1].data()));
        // Fixed layer identity is part of the input for whole-file comparisons.
        QVERIFY(setInitialLayerIdentity(documents[1].data(), inputUuid));
        paintLayer(documents[1].data())->setName(QStringLiteral("Paint"));
        QVERIFY(initializePixels(documents[1].data()));
    } else {
        cpuLayer->setPinnedToTimeline(false);
        QVERIFY(save(documents[0].data(), destination, QStringLiteral("source")));
        qputenv("LIBREPAINT_GPU_ENGINE", "1");
        documents[1].reset(KisPart::instance()->createDocument());
        documents[1]->setFileBatchMode(true);
        documents[1]->setAutoSaveActive(false);
        QVERIFY2(documents[1]->openPath(destination.filePath(QStringLiteral("source.kra")), KisDocument::DontAddToRecent),
                 qPrintable(documents[1]->errorMessage()));
    }
    QVERIFY(!documents[0]->image()->gpuImage());
    QVERIFY(documents[1]->image()->gpuImage());
    QCOMPARE(layerPixels(documents[1].data()), layerPixels(documents[0].data()));
    QMap<QString, QByteArray> reference;
    for (int owner = 0; owner < 2; ++owner) {
        qputenv("LIBREPAINT_GPU_ENGINE", owner ? "1" : "0");
        auto *document = documents[owner].data();
        const QDir out(destination.filePath(owner ? QStringLiteral("gpu") : QStringLiteral("cpu")));
        QVERIFY(QDir().mkpath(out.path()));
        const auto baseline = layerPixels(document);
        for (int stroke = 0; stroke < 2; ++stroke) {
            const int before = document->undoStack()->index();
            QVERIFY(paint(document, preset, opacity, profile, stroke, calls, basicSmoothing, eraser, weightedSmoothing, pixelPerfect));
            const auto bytes = layerPixels(document);
            QFile actual(out.filePath(QStringLiteral("stroke-%1.bgra").arg(stroke)));
            QVERIFY(actual.open(QIODevice::WriteOnly));
            QCOMPARE(actual.write(bytes), qint64(bytes.size()));
            if (eraser) {
                bool colorsPreserved = true, alphaNonIncreasing = true;
                for (qsizetype i = 0; i < bytes.size(); i += 4) {
                    colorsPreserved &= bytes[i] == baseline[i] && bytes[i + 1] == baseline[i + 1] &&
                                       bytes[i + 2] == baseline[i + 2];
                    alphaNonIncreasing &= quint8(bytes[i + 3]) <= quint8(baseline[i + 3]);
                }
                QVERIFY(colorsPreserved);
                QVERIFY(alphaNonIncreasing);
            }
            const QString key = QStringLiteral("stroke-%1").arg(stroke);
            if (!owner) reference.insert(key, bytes); else QCOMPARE(bytes, reference.value(key));
            QCOMPARE(document->undoStack()->index(), before + 1);
        }
        const auto painted = layerPixels(document);
        if (eraser && (opacity == 0 || flow == 0)) QCOMPARE(painted, baseline);
        else if (eraser) QVERIFY(painted != baseline);
        else QVERIFY(painted != QByteArray(painted.size(), '\0'));
        QVERIFY(save(document, out, QStringLiteral("painted")));
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        reopened->setAutoSaveActive(false);
        QVERIFY2(reopened->openPath(out.filePath(QStringLiteral("painted.kra")), KisDocument::DontAddToRecent),
                 qPrintable(reopened->errorMessage()));
        reopened->image()->waitForDone();
        QCOMPARE(bool(reopened->image()->gpuImage()), bool(owner));
        QCOMPARE(layerPixels(reopened.data()), painted);
        const auto before = layerPixels(reopened.data());
        QVERIFY(paint(reopened.data(), preset, opacity, profile, 2, calls, basicSmoothing, eraser, weightedSmoothing, pixelPerfect));
        const auto after = layerPixels(reopened.data());
        if (eraser && (opacity == 0 || flow == 0)) QCOMPARE(after, before);
        else QVERIFY(after != before);
        reopened->undoStack()->undo();
        reopened->image()->waitForDone();
        QCOMPARE(layerPixels(reopened.data()), before);
        reopened->undoStack()->redo();
        reopened->image()->waitForDone();
        QCOMPARE(layerPixels(reopened.data()), after);
        QVERIFY(save(reopened.data(), out, QStringLiteral("after")));
        for (const auto &state : {QStringLiteral("painted"), QStringLiteral("after")}) {
            for (const auto &extension : {QStringLiteral("bgra"), QStringLiteral("kra"), QStringLiteral("png"), QStringLiteral("jpg")}) {
                const QString name = state + QLatin1Char('.') + extension;
                QFile file(out.filePath(name));
                QVERIFY(file.open(QIODevice::ReadOnly));
                const auto bytes = file.readAll();
                QVERIFY(!bytes.isEmpty());
                if (!owner) reference.insert(name, bytes); else QCOMPARE(bytes, reference.value(name));
            }
        }
    }
}

void TestDocument::testDefaultBrushStationaryInputKeepsOneTap_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("basicSmoothing");
    QTest::addColumn<int>("repetitions");
    QTest::addColumn<int>("eraser");
    QTest::addColumn<bool>("weightedSmoothing");
    for (int configuration = 0; configuration < 5; ++configuration) {
        for (int repetitions : {2, 3, 8}) for (int eraser : {0, 1, 2}) {
            auto name = QStringLiteral("%1-%2").arg(configuration == 4 ? "gpu-weighted" : configuration == 3 ? "gpu-basic" :
                configuration == 2 ? "gpu-none" : configuration == 1 ? "cpu-basic" : "cpu-none").arg(repetitions);
            if (eraser) name += QStringLiteral("-erase-%1").arg(eraser);
            QTest::newRow(qPrintable(name)) << (configuration >= 2) << (configuration == 1 || configuration == 3) << repetitions << eraser << (configuration == 4);
        }
    }
}

void TestDocument::testDefaultBrushStationaryInputKeepsOneTap()
{
    QFETCH(bool, gpu);
    QFETCH(bool, basicSmoothing);
    QFETCH(bool, weightedSmoothing);
    QFETCH(int, repetitions);
    QFETCH(int, eraser);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    KisImageConfig imageConfig(false);
    const auto previousThreads = imageConfig.maxNumberOfThreads();
    KisConfig config(false);
    const auto previousCompression = config.readEntry<bool>("compressLayers", true);
    const auto restore = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
        imageConfig.setMaxNumberOfThreads(previousThreads);
        config.writeEntry("compressLayers", previousCompression);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    imageConfig.setMaxNumberOfThreads(4);
    config.writeEntry("compressLayers", true);
    QString diagnostic;
    const auto preset = defaultBrush(.5, diagnostic);
    QVERIFY2(preset, qPrintable(diagnostic));
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QDir destination(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path()) +
        QStringLiteral("/stationary-tap/") + QString::fromLatin1(QTest::currentDataTag()));
    QVERIFY(QDir().mkpath(destination.path()));
    const auto retainFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) { temporary.setAutoRemove(false); qInfo() << "Stationary tap artifacts at" << destination.path(); }
    });
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Stationary tap contract"), side, side, cs,
        KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    QCOMPARE(bool(document->image()->gpuImage()), gpu);
    QVERIFY(setInitialLayerIdentity(document.data(), QUuid(QStringLiteral("{33b00003-1122-3344-5566-778899aabbcc}"))));
    KisPaintLayerSP layer = paintLayer(document.data());
    QVERIFY(layer);
    const auto background = partialAlphaBackground();
    QScopedPointer<Node> node(Node::createNode(document->image(), layer));
    QVERIFY(node->setPixelData(background, 0, 0, side, side));
    document->image()->waitForDone();
    QCOMPARE(layerPixels(document.data()), background);
    const int initialHistory = document->undoStack()->index();
    const auto output = [&](const QString &name) {
        const QDir out(destination.filePath(name));
        QDir().mkpath(out.path());
        return out;
    };
    const QDir tap = output(QStringLiteral("tap"));
    QVERIFY(paint(document.data(), preset, .57, 2, 0, -2, basicSmoothing, eraser, weightedSmoothing));
    const auto expected = layerPixels(document.data());
    QVERIFY(expected != background);
    QCOMPARE(document->undoStack()->index(), initialHistory + 1);
    QVERIFY(save(document.data(), tap, QStringLiteral("document")));
    document->undoStack()->undo();
    document->image()->waitForDone();
    QCOMPARE(layerPixels(document.data()), background);
    const int calls = repetitions == 2 ? -8 : repetitions == 3 ? -5 : -9;
    QVERIFY(paint(document.data(), preset, .57, 2, 0, calls, basicSmoothing, eraser, weightedSmoothing));
    const auto actual = layerPixels(document.data());
    QFile actualPixels(destination.filePath(QStringLiteral("repeated-input.bgra")));
    QVERIFY(actualPixels.open(QIODevice::WriteOnly));
    QCOMPARE(actualPixels.write(actual), qint64(actual.size()));
    actualPixels.close();
    QCOMPARE(actual, expected);
    QCOMPARE(document->undoStack()->index(), initialHistory + 1);
    document->undoStack()->undo();
    document->image()->waitForDone();
    QCOMPARE(layerPixels(document.data()), background);
    document->undoStack()->redo();
    document->image()->waitForDone();
    QCOMPARE(layerPixels(document.data()), expected);
    const QDir repeated = output(QStringLiteral("repeated"));
    QVERIFY(save(document.data(), repeated, QStringLiteral("document")));
    const auto compareFiles = [&](const QDir &first, const QDir &second) {
        for (const auto &extension : {QStringLiteral("bgra"), QStringLiteral("kra"), QStringLiteral("png"), QStringLiteral("jpg")}) {
            QFile reference(first.filePath(QStringLiteral("document.") + extension));
            QFile actual(second.filePath(QStringLiteral("document.") + extension));
            if (!reference.open(QIODevice::ReadOnly) || !actual.open(QIODevice::ReadOnly)) {
                qWarning() << "Cannot read stationary tap artifacts" << reference.fileName() << actual.fileName();
                return false;
            }
            const auto bytes = reference.readAll();
            if (bytes.isEmpty() || bytes != actual.readAll()) {
                qWarning() << "Stationary tap file differs" << reference.fileName() << actual.fileName();
                return false;
            }
        }
        return true;
    };
    QVERIFY(compareFiles(tap, repeated));
    QScopedPointer<KisDocument> reopened[2];
    QByteArray afterTap;
    for (int i = 0; i < 2; ++i) {
        reopened[i].reset(KisPart::instance()->createDocument());
        reopened[i]->setFileBatchMode(true);
        reopened[i]->setAutoSaveActive(false);
        QVERIFY2(reopened[i]->openPath(repeated.filePath(QStringLiteral("document.kra")), KisDocument::DontAddToRecent),
                 qPrintable(reopened[i]->errorMessage()));
        reopened[i]->image()->waitForDone();
        QCOMPARE(bool(reopened[i]->image()->gpuImage()), gpu);
        QCOMPARE(layerPixels(reopened[i].data()), expected);
        QVERIFY(paint(reopened[i].data(), preset, .57, 2, 2, i ? calls : -2, basicSmoothing, eraser, weightedSmoothing));
        const auto after = layerPixels(reopened[i].data());
        QVERIFY(after != expected);
        if (!i) afterTap = after;
        else {
            QFile actualPixels(destination.filePath(QStringLiteral("reopened-repeated-input.bgra")));
            QVERIFY(actualPixels.open(QIODevice::WriteOnly));
            QCOMPARE(actualPixels.write(after), qint64(after.size()));
            actualPixels.close();
            QCOMPARE(after, afterTap);
        }
        reopened[i]->undoStack()->undo();
        reopened[i]->image()->waitForDone();
        QCOMPARE(layerPixels(reopened[i].data()), expected);
        reopened[i]->undoStack()->redo();
        reopened[i]->image()->waitForDone();
        QCOMPARE(layerPixels(reopened[i].data()), after);
        QVERIFY(save(reopened[i].data(), output(i ? QStringLiteral("after-repeated") : QStringLiteral("after-tap")),
                     QStringLiteral("document")));
    }
    QVERIFY(compareFiles(output(QStringLiteral("after-tap")), output(QStringLiteral("after-repeated"))));
}
