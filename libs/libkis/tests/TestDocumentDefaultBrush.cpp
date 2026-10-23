/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestDocument.h"
#include <simpletest.h>
#include <Document.h>
#include <InfoObject.h>
#include <KisGlobalResourcesInterface.h>
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
    if (!fixInformation(document)) return false;
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    document->setModified(true);
    if (!document->saveAs(out.filePath(name + QStringLiteral(".kra")), KisDocument::nativeFormatMimeType(), false, settings)) return false;
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
        if (!api.exportImage(out.filePath(name + QLatin1Char('.') + extension), options)) return false;
    QFile raw(out.filePath(name + QStringLiteral(".bgra")));
    const auto bytes = layerPixels(document);
    return raw.open(QIODevice::WriteOnly) && raw.write(bytes) == bytes.size();
}

void paint(KisDocument *document, KisPaintOpPresetSP preset, qreal opacity, int profile, int stroke, int calls)
{
    KisImageSP image = document->image();
    const auto layer = paintLayer(document);
    KoCanvasResourceProvider resources;
    KisViewManager::initializeResourceManager(&resources);
    resources.setResource(KoCanvasResource::CurrentKritaNode, QVariant::fromValue(KisNodeSP(layer)));
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(preset));
    resources.setResource(KoCanvasResource::Opacity, opacity);
    resources.setResource(KoCanvasResource::CurrentCompositeOp, COMPOSITE_OVER);
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
}
}

void TestDocument::testDefaultBrushGpuEditingKeepsPixelsHistoryAndFiles_data()
{
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("profile");
    QTest::addColumn<bool>("newDocument");
    QTest::addColumn<int>("calls");
    for (bool fresh : {false, true}) for (qreal opacity : {qreal(1), qreal(.57)}) {
        for (int profile = 0; profile < 3; ++profile) {
            const auto name = QStringLiteral("%1-opacity-%2-pressure-%3").arg(fresh ? "new" : "opened").arg(opacity).arg(profile);
            QTest::newRow(qPrintable(name)) << opacity << qreal(1) << profile << fresh << 0;
        }
        const auto name = QStringLiteral("%1-opacity-%2-half-flow").arg(fresh ? "new" : "opened").arg(opacity);
        QTest::newRow(qPrintable(name)) << opacity << qreal(.5) << 2 << fresh << 0;
        for (int calls = 1; calls <= 9; ++calls) {
            const auto kind = calls == 1 ? "compound-path" : calls == 2 ? "separate-paths" :
                              calls == 3 ? "line-path-line" : calls == 4 ? "path-line-path" :
                              calls == 5 ? "cubic-open" : calls == 6 ? "cubic-closed" :
                              calls == 7 ? "cubic-compound" : calls == 8 ? "cubic-separate" : "line-cubic-line";
            const auto group = QStringLiteral("%1-%2-opacity-%3").arg(fresh ? "new" : "opened").arg(QLatin1String(kind)).arg(opacity);
            QTest::newRow(qPrintable(group)) << opacity << qreal(.5) << 2 << fresh << calls;
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
    if (newDocument) {
        QVERIFY(create(1));
        QVERIFY(paintLayer(documents[1].data()));
        // One new document supplies the identity for the paired CPU reference.
        // Fixed imported documents below also retain whole-file identity across hosts.
        cpuLayer->setUuid(paintLayer(documents[1].data())->uuid());
        paintLayer(documents[1].data())->setName(QStringLiteral("Paint"));
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
        for (int stroke = 0; stroke < 2; ++stroke) {
            const int before = document->undoStack()->index();
            paint(document, preset, opacity, profile, stroke, calls);
            const auto bytes = layerPixels(document);
            QFile actual(out.filePath(QStringLiteral("stroke-%1.bgra").arg(stroke)));
            QVERIFY(actual.open(QIODevice::WriteOnly));
            QCOMPARE(actual.write(bytes), qint64(bytes.size()));
            const QString key = QStringLiteral("stroke-%1").arg(stroke);
            if (!owner) reference.insert(key, bytes); else QCOMPARE(bytes, reference.value(key));
            QCOMPARE(document->undoStack()->index(), before + 1);
        }
        const auto painted = layerPixels(document);
        QVERIFY(painted != QByteArray(painted.size(), '\0'));
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
        paint(reopened.data(), preset, opacity, profile, 2, calls);
        const auto after = layerPixels(reopened.data());
        QVERIFY(after != before);
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
