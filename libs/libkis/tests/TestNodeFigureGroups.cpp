/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestNode.h"
#include <simpletest.h>
#include <KoCanvasResourceProvider.h>
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <KoCompositeOpRegistry.h>
#include <KisGlobalResourcesInterface.h>
#include <application/kis_config.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_settings.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <kis_figure_painting_stroke.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kundo2stack.h>
#include <kundo2magicstring.h>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <algorithm>
#include <memory>

namespace Gpu = LibrePaint::Image::Gpu;

void TestNode::testFigureGroupsKeepSegmentsSensorsAndHistory_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<qreal>("opacity");
    for (const auto &kind : {QStringLiteral("connected-lines"), QStringLiteral("separate-lines"),
                             QStringLiteral("separate-paths"), QStringLiteral("mixed")}) {
        for (qreal opacity : {qreal(1), qreal(.57)}) {
            const auto name = kind + QStringLiteral("-opacity-%1").arg(opacity);
            QTest::newRow(qPrintable(name)) << kind << opacity;
        }
    }
}

void TestNode::testFigureGroupsKeepSegmentsSensorsAndHistory()
{
    QFETCH(QString, kind);
    QFETCH(qreal, opacity);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Grouped outlines"), 128, 128, cs,
        KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    KisImageSP image = document->image();
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    KisNodeSP layer = image->root()->firstChild();
    QVERIFY(layer);
    const auto layerId = owner->operations().activeLayer();
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(
        KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    QVERIFY(preset);
    preset->settings()->setPaintOpSize(12);
    KoCanvasResourceProvider resources;
    KisViewManager::initializeResourceManager(&resources);
    resources.setResource(KoCanvasResource::CurrentKritaNode, QVariant::fromValue(layer));
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(preset));
    resources.setResource(KoCanvasResource::CurrentCompositeOp, COMPOSITE_OVER);
    resources.setResource(KoCanvasResource::Size, qreal(12));
    resources.setResource(KoCanvasResource::Opacity, opacity);
    resources.setResource(KoCanvasResource::MirrorHorizontal, false);
    resources.setResource(KoCanvasResource::MirrorVertical, false);
    resources.setForegroundColor(KoColor(QColor(17, 129, 241), cs));

    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QDir out(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path()) +
                  QStringLiteral("/figure-groups/") + QString::fromLatin1(QTest::currentDataTag()));
    QVERIFY(QDir().mkpath(out.path()));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            temporary.setAutoRemove(false);
            qInfo() << "Grouped figure artifacts at" << out.path();
        }
    });
    const auto savePixels = [&](const QString &name, const QByteArray &bytes) {
        QFile file(out.filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    const QRect bounds(0, 0, 128, 128);
    const auto read = [&](const Gpu::PaintImage &source, Gpu::LayerId id) {
        return source.readRgbaPixels(id, bounds, 128 * 128 * 4);
    };
    const auto empty = read(*owner, layerId);
    QCOMPARE(empty.code, Gpu::ExecutionCode::Success);
    QCOMPARE(empty.bytes, QByteArray(128 * 128 * 4, '\0'));
    const int history = document->undoStack()->index();
    Gpu::Shape expectedShape;
    expectedShape.kind = Gpu::ShapeKind::Path;
    QList<Gpu::InputSample> expectedSamples;
    {
        KisFigurePaintingStroke stroke(kundo2_i18n("Grouped outlines"), image, layer, &resources,
            KisFigurePaintingOptions::StrokeStyleForeground, KisFigurePaintingOptions::FillStyleNone);
        const auto line = [&](QPointF first, QPointF last, qreal firstPressure, qreal lastPressure, int time) {
            KisPaintInformation a(first, firstPressure, .1, -.2, .3);
            KisPaintInformation b(last, lastPressure, -.4, .5, -.6);
            a.setCurrentTime(time);
            b.setCurrentTime(time + 16);
            stroke.paintLine(a, b);
            expectedShape.pathCommands.append({Gpu::PathCommand::MoveTo, Gpu::PathCommand::LineTo});
            expectedSamples.append({first, firstPressure, a.xTilt(), a.yTilt(), a.rotation(), quint64(time)});
            expectedSamples.append({last, lastPressure, b.xTilt(), b.yTilt(), b.rotation(), quint64(time + 16)});
        };
        const auto path = [&](const QPainterPath &value) {
            stroke.paintPainterPath(value);
            for (int i = 0; i < value.elementCount(); ++i) {
                const auto element = value.elementAt(i);
                if (element.type == QPainterPath::CurveToElement) {
                    expectedShape.pathCommands.append(Gpu::PathCommand::CubicTo);
                    for (int j = 0; j < 3; ++j) {
                        const auto control = value.elementAt(i + j);
                        expectedSamples.append({{control.x, control.y}, 1});
                    }
                    i += 2;
                } else {
                    expectedShape.pathCommands.append(element.type == QPainterPath::MoveToElement ?
                        Gpu::PathCommand::MoveTo : Gpu::PathCommand::LineTo);
                    expectedSamples.append({{element.x, element.y}, 1});
                }
            }
        };
        if (kind == QLatin1String("separate-paths") || kind == QLatin1String("mixed")) {
            QPainterPath first({12, 24});
            first.lineTo(42, 24);
            first.cubicTo(48, 24, 48, 42, 54, 42);
            path(first);
        } else line({12.25, 24.75}, {52.5, 24.75}, .5, 1, 0);
        if (kind == QLatin1String("separate-paths")) {
            QPainterPath second({76, 96});
            second.lineTo(112, 96);
            path(second);
        } else if (kind == QLatin1String("connected-lines")) {
            // The shared position carries distinct endpoint sensors in the two jobs.
            line({52.5, 24.75}, {88.25, 56.5}, .75, .25, 32);
        } else line({76, 96}, {112, 96}, .9, .4, 32);
    }
    image->waitForDone();
    const auto painted = read(*owner, layerId);
    QCOMPARE(painted.code, Gpu::ExecutionCode::Success);
    QVERIFY(savePixels(QStringLiteral("painted.rgba"), painted.bytes));
    QVERIFY(painted.bytes != empty.bytes);
    QCOMPARE(document->undoStack()->index(), history + 1);

    // The existing path renderer is the independent equivalent operation. It
    // preserves subpath boundaries and the pressure carried by each endpoint.
    Gpu::PaintImage reference(owner->execution(), QSize(128, 128));
    Gpu::Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 12;
    brush.opacity = quint8(qRound(255 * opacity));
    QCOMPARE(reference.paintFigure(reference.operations().activeLayer(), brush, expectedSamples,
                                   expectedShape, {}, 0), Gpu::Result::Success);
    const auto expected = read(reference, reference.operations().activeLayer());
    QCOMPARE(expected.code, Gpu::ExecutionCode::Success);
    QVERIFY(savePixels(QStringLiteral("expected.rgba"), expected.bytes));
    QCOMPARE(painted.bytes, expected.bytes);
    QCOMPARE(painted.bytes.mid((64 * 128 + 64) * 4, 4), QByteArray(4, '\0'));
    const auto version = owner->operations().snapshot();
    const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [&](const auto &value) { return value.id == layerId; });
    QVERIFY(found != version->layers.cend());
    QVERIFY(!found->operations.isEmpty());
    const auto operation = std::get<Gpu::PaintSnapshot>(found->operations.last());
    QVERIFY(operation);
    QCOMPARE(operation->inputSamples(), expectedSamples);
    const auto recovery = owner->operations().recoveryData();
    QVERIFY(!recovery.isEmpty());
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(read(*owner, layerId).bytes, empty.bytes);
    document->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(read(*owner, layerId).bytes, painted.bytes);
    QCOMPARE(owner->restoreRecovery(recovery), Gpu::Result::Success);
    QCOMPARE(read(*owner, layerId).bytes, painted.bytes);
    QVERIFY(savePixels(QStringLiteral("recovered.rgba"), read(*owner, layerId).bytes));
    const auto path = out.filePath(QStringLiteral("painted.kra"));
    QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    reopened->setAutoSaveActive(false);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    const auto loaded = reopened->image()->gpuImage();
    QVERIFY(loaded);
    const auto loadedPixels = read(*loaded, loaded->operations().activeLayer());
    QCOMPARE(loadedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(loadedPixels.bytes, painted.bytes);
    QVERIFY(savePixels(QStringLiteral("reopened.rgba"), loadedPixels.bytes));
}

void TestNode::testFigureGroupInputLimitsKeepPixelsAndRedo_data()
{
    QTest::addColumn<int>("strokeLimit");
    QTest::addColumn<int>("documentLimit");
    QTest::addColumn<int>("lines");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("exact-limit") << 4 << 4 << 2 << true;
    QTest::newRow("stroke-limit") << 4 << 8 << 3 << false;
    QTest::newRow("document-limit") << 8 << 4 << 3 << false;
}

void TestNode::testFigureGroupInputLimitsKeepPixelsAndRedo()
{
    QFETCH(int, strokeLimit);
    QFETCH(int, documentLimit);
    QFETCH(int, lines);
    QFETCH(bool, accepted);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::Limits limits;
    limits.samplesPerStroke = strokeLimit;
    limits.samplesPerDocument = documentLimit;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(128, 128), limits);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Bounded figure group")));
    KisImageSP image = document->image();
    const auto layer = image->root()->firstChild();
    const auto layerId = owner->operations().activeLayer();
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(
        KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    QVERIFY(preset);
    KoCanvasResourceProvider resources;
    KisViewManager::initializeResourceManager(&resources);
    resources.setResource(KoCanvasResource::CurrentKritaNode, QVariant::fromValue(layer));
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(preset));
    resources.setResource(KoCanvasResource::CurrentCompositeOp, COMPOSITE_OVER);
    resources.setResource(KoCanvasResource::Size, qreal(12));
    resources.setResource(KoCanvasResource::Opacity, qreal(1));
    resources.setForegroundColor(KoColor(Qt::black, image->colorSpace()));
    const auto paint = [&](int count) {
        KisFigurePaintingStroke stroke(kundo2_i18n("Bounded figure group"), image, layer, &resources,
            KisFigurePaintingOptions::StrokeStyleForeground, KisFigurePaintingOptions::FillStyleNone);
        for (int i = 0; i < count; ++i)
            stroke.paintLine(KisPaintInformation({12., 16. + i * 32}, 1),
                             KisPaintInformation({112., 16. + i * 32}, 1));
    };
    paint(1);
    image->waitForDone();
    QCOMPARE(document->undoStack()->index(), 1);
    document->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(document->undoStack()->index(), 0);
    QVERIFY(document->undoStack()->canRedo());
    const auto before = owner->operations().snapshot();
    const auto recovery = owner->operations().recoveryData();
    const auto empty = owner->readRgbaPixels(layerId, {0, 0, 128, 128}, 128 * 128 * 4);
    QCOMPARE(empty.code, Gpu::ExecutionCode::Success);
    paint(lines);
    image->waitForDone();
    const auto after = owner->readRgbaPixels(layerId, {0, 0, 128, 128}, 128 * 128 * 4);
    QCOMPARE(after.code, Gpu::ExecutionCode::Success);
    if (accepted) {
        QCOMPARE(document->undoStack()->index(), 1);
        QVERIFY(!document->undoStack()->canRedo());
        QCOMPARE(owner->operations().snapshot()->sampleCount, qsizetype(4));
        QVERIFY(after.bytes != empty.bytes);
    } else {
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->operations().recoveryData(), recovery);
        QCOMPARE(after.bytes, empty.bytes);
        QCOMPARE(document->undoStack()->index(), 0);
        QVERIFY(document->undoStack()->canRedo());
        document->undoStack()->redo();
        image->waitForDone();
        QCOMPARE(document->undoStack()->index(), 1);
        QVERIFY(owner->readRgbaPixels(layerId, {0, 0, 128, 128}, 128 * 128 * 4).bytes != empty.bytes);
    }
}
