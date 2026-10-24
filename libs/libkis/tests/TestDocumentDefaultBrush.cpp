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

namespace {
constexpr int side = 128;

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
           bool basicSmoothing = false, int eraser = 0)
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
        KisPaintingInformationBuilder builder;
        auto *options = new KisSmoothingOptions(false);
        options->setSmoothingType(basicSmoothing ? KisSmoothingOptions::SIMPLE_SMOOTHING : KisSmoothingOptions::NO_SMOOTHING);
        KisToolFreehandHelper helper(&builder, &resources, kundo2_i18n("Default brush input contract"), options);
        QStringList errors;
        QObject::connect(&helper, &KisToolFreehandHelper::requestErrorMessage, &helper,
                         [&](const QString &message) { errors.append(message); });
        const auto before = layerPixels(document);
        bool initialUnchanged = false;
        for (qsizetype i = 0; i < path.size(); ++i) {
            const auto point = path[i] + offset;
            QMouseEvent event(i ? QEvent::MouseMove : QEvent::MouseButtonPress, point, point,
                              i ? Qt::NoButton : Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            event.setTimestamp(1000 + i * 16);
            KoPointerEvent pointer(&event, point);
            if (!i) {
                helper.initPaint(&pointer, point, image, layer, image.data());
                initialUnchanged = layerPixels(document) == before;
            } else helper.paintEvent(&pointer);
        }
        helper.endPaint();
        image->waitForDone();
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

void TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles_data()
{
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("profile");
    QTest::addColumn<bool>("newDocument");
    QTest::addColumn<int>("calls");
    QTest::addColumn<int>("eraser");
    for (bool fresh : {false, true}) for (qreal opacity : {qreal(1), qreal(.57)}) {
        for (int profile = 0; profile < 3; ++profile) {
            const auto name = QStringLiteral("%1-opacity-%2-pressure-%3").arg(fresh ? "new" : "opened").arg(opacity).arg(profile);
            QTest::newRow(qPrintable(name)) << opacity << qreal(1) << profile << fresh << 0 << 0;
        }
        const auto name = QStringLiteral("%1-opacity-%2-half-flow").arg(fresh ? "new" : "opened").arg(opacity);
        QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << 0 << 0;
        for (int calls = 1; calls <= 9; ++calls) {
            const auto kind = calls == 1 ? "compound-path" : calls == 2 ? "separate-paths" :
                              calls == 3 ? "line-path-line" : calls == 4 ? "path-line-path" :
                              calls == 5 ? "cubic-open" : calls == 6 ? "cubic-closed" :
                              calls == 7 ? "cubic-compound" : calls == 8 ? "cubic-separate" : "line-cubic-line";
            const auto group = QStringLiteral("%1-%2-opacity-%3").arg(fresh ? "new" : "opened").arg(QLatin1String(kind)).arg(opacity);
            QTest::newRow(qPrintable(group)) << opacity << qreal(.5) << 2 << fresh << calls << 0;
        }
    }
    const std::array<const char *, 9> kinds{{"move", "tap", "two", "closed", "stationary", "duplicate-first", "return",
                                          "stationary-two", "stationary-eight"}};
    for (bool fresh : {false, true}) for (qreal opacity : {qreal(1), qreal(.57)}) {
        for (std::size_t i = 0; i < kinds.size(); ++i) {
            const auto name = QStringLiteral("%1-freehand-%2-opacity-%3").arg(fresh ? "new" : "opened").arg(QLatin1String(kinds[i])).arg(opacity);
            QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << -int(i + 1) << 0;
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
            QTest::newRow(qPrintable(name)) << opacity[i] << flow[i] << 2 << fresh << calls[i] << eraser;
        }
        for (qreal opacity : {qreal(1), qreal(.57)}) {
            for (std::size_t i = 0; i < kinds.size(); ++i) {
                const auto name = QStringLiteral("%1-erase-%2-freehand-%3-opacity-%4").arg(fresh ? "new" : "opened")
                    .arg(eraser).arg(QLatin1String(kinds[i])).arg(opacity);
                QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << -int(i + 1) << eraser;
            }
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
        // One new document supplies the identity for the paired CPU reference.
        // Fixed imported documents below also retain whole-file identity across hosts.
        cpuLayer->setUuid(paintLayer(documents[1].data())->uuid());
        paintLayer(documents[1].data())->setName(QStringLiteral("Paint"));
        QVERIFY(initializePixels(documents[1].data()));
    } else {
        cpuLayer->setUuid(QUuid(QStringLiteral("{33b00001-1122-3344-5566-778899aabbcc}")));
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
            QVERIFY(paint(document, preset, opacity, profile, stroke, calls, false, eraser));
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
        QVERIFY(paint(reopened.data(), preset, opacity, profile, 2, calls, false, eraser));
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
    for (int configuration = 0; configuration < 3; ++configuration) {
        for (int repetitions : {2, 3, 8}) for (int eraser : {0, 1, 2}) {
            auto name = QStringLiteral("%1-%2").arg(configuration == 2 ? "gpu-none" :
                configuration == 1 ? "cpu-basic" : "cpu-none").arg(repetitions);
            if (eraser) name += QStringLiteral("-erase-%1").arg(eraser);
            QTest::newRow(qPrintable(name)) << (configuration == 2) << (configuration == 1) << repetitions << eraser;
        }
    }
}

void TestDocument::testDefaultBrushStationaryInputKeepsOneTap()
{
    QFETCH(bool, gpu);
    QFETCH(bool, basicSmoothing);
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
    const auto layer = paintLayer(document.data());
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
    QVERIFY(paint(document.data(), preset, .57, 2, 0, -2, basicSmoothing, eraser));
    const auto expected = layerPixels(document.data());
    QVERIFY(expected != background);
    QCOMPARE(document->undoStack()->index(), initialHistory + 1);
    QVERIFY(save(document.data(), tap, QStringLiteral("document")));
    document->undoStack()->undo();
    document->image()->waitForDone();
    QCOMPARE(layerPixels(document.data()), background);
    const int calls = repetitions == 2 ? -8 : repetitions == 3 ? -5 : -9;
    QVERIFY(paint(document.data(), preset, .57, 2, 0, calls, basicSmoothing, eraser));
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
        QVERIFY(paint(reopened[i].data(), preset, .57, 2, 2, i ? calls : -2, basicSmoothing, eraser));
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
