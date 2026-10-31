/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QDir>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QPointer>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QScreen>
#include <QTemporaryDir>
#include <KLocalizedString>
#include <KisFigurePaintingOptions.h>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoPattern.h>
#include <KoToolBase.h>
#include <KoSnapGuide.h>
#include <KoResourceCacheInterface.h>
#include <KoCanvasResourceProvider.h>
#include <KoCanvasResourcesIds.h>
#include <KoPointerEvent.h>
#include <KoToolManager.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <actions/input/KisApplicationInputActions.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_settings.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_canvas_controller.h>
#include <canvas/kis_zoom_manager.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <input/ui/kis_input_profile_manager.h>
#include <kra/KisKraRasterCodec.h>
#include <nodes/kis_node_manager.h>
#include <kis_canvas2.h>
#include <kis_image.h>
#include <kis_tool.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_pixel_selection.h>
#include <kis_paint_layer.h>
#include <kis_selection.h>
#include <commands/kis_set_global_selection_command.h>
#include <QPainterPath>
#include <limits>
#include <kis_properties_configuration.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <algorithm>
#include <memory>
#include <cmath>
#include <cstring>
#include <utility>

enum class FigureKind { Ellipse, Rectangle, RoundedRectangle, Polyline, Polygon };
Q_DECLARE_METATYPE(FigureKind)

class KisEllipseToolDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        KisInputProfileManager::instance()->setActions(createApplicationInputActions());
        const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(bundlePath);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(bundlePath, bundle));
        m_preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(m_preset);
        m_preset->setName(QStringLiteral("Ellipse tool document fixture"));
        m_preset->setFilename(QStringLiteral("ellipse-tool-document-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        m_preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(m_preset));
        m_fillPreset.reset(new KisPaintOpPreset(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/paintoppresets/b)_Basic-5_Size_default.kpp")));
        QVERIFY(m_fillPreset->load(KisGlobalResourcesInterface::instance()));
        m_fillPreset->setFilename(QFileInfo(m_fillPreset->filename()).fileName());
        QVERIFY(presets.addResource(m_fillPreset));
        KisResourceModel brushes(ResourceType::Brushes);
        QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
        KisResourceModel patterns(ResourceType::Patterns);
        QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    }
    void drawingPreservesShapeHistoryAndSaving_data();
    void drawingPreservesShapeHistoryAndSaving();
    void solidFillsPreserveShapeHistoryAndSaving_data();
    void solidFillsPreserveShapeHistoryAndSaving();
    void ordinarySolidFillsPreserveShapeHistoryAndSaving_data();
    void ordinarySolidFillsPreserveShapeHistoryAndSaving();
    void fillOnlyUsesSelectedPreset_data();
    void fillOnlyUsesSelectedPreset();
    void fractionalSolidFillsUseCapturedOpacity_data();
    void fractionalSolidFillsUseCapturedOpacity();
    void fractionalFillMatchesPainterWithProtectedChannels_data();
    void fractionalFillMatchesPainterWithProtectedChannels();
    void nativeSolidFillsKeepOrdinaryEditing_data();
    void nativeSolidFillsKeepOrdinaryEditing();
    void standardBrushShapesKeepOrdinaryEditing_data();
    void standardBrushShapesKeepOrdinaryEditing();
    void selectedSolidFillsKeepOrdinaryEditing_data();
    void selectedSolidFillsKeepOrdinaryEditing();
private:
    void addSolidFigureRows(bool gpu);
    void exerciseFigure(KisPaintOpPresetSP preset = {}, bool gpuForPreset = false, QByteArray *firstPixels = nullptr,
                        qreal opacity = 1, bool words = false, QHash<QString, QByteArray> *nativeStates = nullptr);
    KisPaintOpPresetSP m_preset;
    KisPaintOpPresetSP m_fillPreset;
};

void KisEllipseToolDocumentTest::drawingPreservesShapeHistoryAndSaving_data()
{
    QTest::addColumn<FigureKind>("figure");
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("closeOnError");
    QTest::addColumn<bool>("canonical");
    QTest::addColumn<bool>("closeOnPublish");
    QTest::addColumn<int>("fillStyle");
    QTest::addColumn<int>("outlineStyle");
    const auto row = [](const char *name, FigureKind figure, bool gpu,
                        bool close = false, bool canonical = false, bool publish = false) {
        QTest::newRow(name) << figure << gpu << close << canonical << publish
                            << int(KisFigurePaintingOptions::FillStyleNone)
                            << int(KisFigurePaintingOptions::StrokeStyleForeground);
    };
    row("ordinary", FigureKind::Ellipse, false);
    row("gpu", FigureKind::Ellipse, true);
    row("gpu-close-on-rejection", FigureKind::Ellipse, true, true);
    row("gpu-canonical", FigureKind::Ellipse, true, false, true);
    row("rectangle-ordinary", FigureKind::Rectangle, false);
    row("rectangle-gpu", FigureKind::Rectangle, true);
    row("rectangle-gpu-close-on-rejection", FigureKind::Rectangle, true, true);
    row("rectangle-gpu-rounded", FigureKind::RoundedRectangle, true);
    row("rectangle-ordinary-rounded", FigureKind::RoundedRectangle, false);
    row("rectangle-gpu-rounded-close-on-invalid-pattern-resource", FigureKind::RoundedRectangle, true, true);
    row("rectangle-gpu-rounded-canonical", FigureKind::RoundedRectangle, true, false, true);
    row("rectangle-gpu-canonical", FigureKind::Rectangle, true, false, true);
    row("rectangle-gpu-close-on-publish", FigureKind::Rectangle, true, false, false, true);
    row("polyline-ordinary", FigureKind::Polyline, false);
    row("polyline-gpu", FigureKind::Polyline, true);
    row("polyline-gpu-canonical", FigureKind::Polyline, true, false, true);
    row("polyline-gpu-close-on-invalid-pattern-resource", FigureKind::Polyline, true, true);
    row("polyline-gpu-close-on-publish", FigureKind::Polyline, true, false, false, true);
    row("polygon-ordinary", FigureKind::Polygon, false);
    row("polygon-gpu", FigureKind::Polygon, true);
    row("polygon-gpu-canonical", FigureKind::Polygon, true, false, true);
    row("polygon-gpu-close-on-invalid-pattern-resource", FigureKind::Polygon, true, true);
}

void KisEllipseToolDocumentTest::solidFillsPreserveShapeHistoryAndSaving_data()
{
    addSolidFigureRows(true);
}

void KisEllipseToolDocumentTest::ordinarySolidFillsPreserveShapeHistoryAndSaving_data()
{
    addSolidFigureRows(false);
}

void KisEllipseToolDocumentTest::addSolidFigureRows(bool gpu)
{
    QTest::addColumn<FigureKind>("figure");
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("closeOnError");
    QTest::addColumn<bool>("canonical");
    QTest::addColumn<bool>("closeOnPublish");
    QTest::addColumn<int>("fillStyle");
    QTest::addColumn<int>("outlineStyle");
    for (const auto figure : {FigureKind::Ellipse, FigureKind::Rectangle, FigureKind::RoundedRectangle,
                              FigureKind::Polyline, FigureKind::Polygon}) {
        for (int mode : {0, 1, 2}) {
            if ((mode != 0) != gpu) continue;
            const auto name = QString::number(int(figure)) + (mode == 0 ? QStringLiteral("-ordinary") :
                              mode == 1 ? QStringLiteral("-gpu-input") : QStringLiteral("-gpu-canonical"));
            QTest::newRow(qPrintable(name)) << figure << (mode != 0) << false << (mode == 2) << false
                << int(figure == FigureKind::Ellipse ? KisFigurePaintingOptions::FillStyleForegroundColor :
                                                      KisFigurePaintingOptions::FillStyleBackgroundColor)
                << int(KisFigurePaintingOptions::StrokeStyleForeground);
        }
    }
    for (int mode : {0, 1, 2}) {
        if ((mode != 0) != gpu) continue;
        const auto name = QStringLiteral("fill-only-rectangle-") + QString::number(mode);
        QTest::newRow(qPrintable(name)) << FigureKind::Rectangle << (mode != 0) << false << (mode == 2) << false
            << int(KisFigurePaintingOptions::FillStyleBackgroundColor)
            << int(KisFigurePaintingOptions::StrokeStyleNone);
    }
}

void KisEllipseToolDocumentTest::drawingPreservesShapeHistoryAndSaving()
{
    exerciseFigure();
}

void KisEllipseToolDocumentTest::solidFillsPreserveShapeHistoryAndSaving()
{
    exerciseFigure();
}

void KisEllipseToolDocumentTest::ordinarySolidFillsPreserveShapeHistoryAndSaving()
{
    exerciseFigure();
}

void KisEllipseToolDocumentTest::fillOnlyUsesSelectedPreset_data()
{
    QTest::addColumn<FigureKind>("figure");
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("closeOnError");
    QTest::addColumn<bool>("canonical");
    QTest::addColumn<bool>("closeOnPublish");
    QTest::addColumn<int>("fillStyle");
    QTest::addColumn<int>("outlineStyle");
    for (const auto &entry : {std::pair{FigureKind::Ellipse, "ellipse"}, {FigureKind::Rectangle, "rectangle"},
                              {FigureKind::RoundedRectangle, "rounded-rectangle"}, {FigureKind::Polyline, "polyline"},
                              {FigureKind::Polygon, "polygon"}}) {
        QTest::newRow(entry.second) << entry.first << false << false << true << false
            << int(entry.first == FigureKind::Ellipse ? KisFigurePaintingOptions::FillStyleForegroundColor :
                                                       KisFigurePaintingOptions::FillStyleBackgroundColor)
            << int(KisFigurePaintingOptions::StrokeStyleNone);
    }
    for (const auto &entry : {std::pair{FigureKind::Rectangle, "rectangle-close-on-publish"},
                              std::pair{FigureKind::Polygon, "polygon-close-on-publish"}}) {
        QTest::newRow(entry.second) << entry.first << true << false << true << true
            << int(KisFigurePaintingOptions::FillStyleForegroundColor)
            << int(KisFigurePaintingOptions::StrokeStyleNone);
    }
}

void KisEllipseToolDocumentTest::fillOnlyUsesSelectedPreset()
{
    QFETCH(bool, closeOnPublish);
    if (closeOnPublish) {
        exerciseFigure(m_fillPreset, true);
        return;
    }
    QByteArray cpu, gpu;
    exerciseFigure(m_fillPreset, false, &cpu);
    if (QTest::currentTestFailed()) return;
    exerciseFigure(m_fillPreset, true, &gpu);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(gpu, cpu);
}

void KisEllipseToolDocumentTest::fractionalSolidFillsUseCapturedOpacity_data()
{
    QTest::addColumn<FigureKind>("figure");
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("closeOnError");
    QTest::addColumn<bool>("canonical");
    QTest::addColumn<bool>("closeOnPublish");
    QTest::addColumn<int>("fillStyle");
    QTest::addColumn<int>("outlineStyle");
    QTest::addColumn<qreal>("opacity");
    const auto row = [](const char *name, FigureKind figure, qreal opacity) {
        QTest::newRow(name) << figure << false << false << true << false
            << int(KisFigurePaintingOptions::FillStyleForegroundColor)
            << int(KisFigurePaintingOptions::StrokeStyleNone) << opacity;
    };
    row("ellipse-half", FigureKind::Ellipse, .5);
    row("rectangle-half", FigureKind::Rectangle, .5);
    row("rounded-rectangle-half", FigureKind::RoundedRectangle, .5);
    row("polyline-half", FigureKind::Polyline, .5);
    row("polygon-half", FigureKind::Polygon, .5);
    row("ellipse-byte128", FigureKind::Ellipse, qreal(128) / 255);
    row("ellipse-fraction", FigureKind::Ellipse, .37);
    row("ellipse-zero", FigureKind::Ellipse, 0);
    row("ellipse-tiny", FigureKind::Ellipse, 1e-15);
}

void KisEllipseToolDocumentTest::fractionalSolidFillsUseCapturedOpacity()
{
    QFETCH(qreal, opacity);
    QByteArray cpu, gpu;
    exerciseFigure(m_fillPreset, false, &cpu, opacity);
    if (QTest::currentTestFailed()) return;
    exerciseFigure(m_fillPreset, true, &gpu, opacity);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(gpu, cpu);
}

void KisEllipseToolDocumentTest::fractionalFillMatchesPainterWithProtectedChannels_data()
{
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<int>("writeMask");
    QTest::newRow("zero") << qreal(0) << 15;
    QTest::newRow("half") << qreal(.5) << 15;
    QTest::newRow("byte128") << qreal(128) / 255 << 15;
    QTest::newRow("fraction") << qreal(.37) << 15;
    QTest::newRow("tiny") << qreal(1e-15) << 15;
    QTest::newRow("subnormal") << qreal(std::numeric_limits<float>::denorm_min()) << 15;
    QTest::newRow("rounded-to-zero") << qreal(1e-46) << 15;
    QTest::newRow("alpha-protected") << qreal(.37) << 7;
    QTest::newRow("red-blue-protected") << qreal(.37) << 10;
    QTest::newRow("green-protected") << qreal(.5) << 13;
}

void KisEllipseToolDocumentTest::fractionalFillMatchesPainterWithProtectedChannels()
{
    QFETCH(qreal, opacity);
    QFETCH(int, writeMask);
    namespace Gpu = LibrePaint::Image::Gpu;
    const QRect region(0, 0, 64, 64);
    const auto rgba = [](QByteArray pixels) {
        for (qsizetype i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
        return pixels;
    };
    QByteArray initial(64 * 64 * 4, Qt::Uninitialized), alpha(64 * 64, Qt::Uninitialized);
    const quint8 destinationAlpha[] = {0, 1, 128, 255};
    const quint8 selectionAlpha[] = {0, 1, 3, 127, 128, 254, 255};
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            const auto offset = (y * 64 + x) * 4;
            initial[offset] = char(37 + (x * 3) % 173);
            initial[offset + 1] = char(17 + (y * 7) % 197);
            initial[offset + 2] = char(219 - (x + y) % 181);
            initial[offset + 3] = char(destinationAlpha[(x + y) % 4]);
            alpha[y * 64 + x] = char(selectionAlpha[(x + 3 * y) % 7]);
        }
    }
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP cpu(new KisPaintDevice(cs));
    cpu->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), 0, 0, 64, 64);
    KisSelectionSP selection(new KisSelection);
    selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), 0, 0, 64, 64);
    KisPainter painter(cpu, selection);
    painter.setPaintColor(KoColor(QColor(17, 129, 241), cs));
    painter.setFillStyle(KisPainter::FillStyleForegroundColor);
    painter.setOpacityF(opacity);
    QBitArray channels(4);
    for (int channel = 0; channel < 4; ++channel)
        channels.setBit(channel, writeMask & (1 << (channel == 0 ? 2 : channel == 2 ? 0 : channel)));
    painter.setChannelFlags(channels);
    QPainterPath path;
    path.addEllipse(QRectF(16, 16, 32, 32));
    painter.fillPainterPath(path);
    QByteArray expected(initial.size(), Qt::Uninitialized);
    cpu->readBytes(reinterpret_cast<quint8 *>(expected.data()), 0, 0, 64, 64);
    expected = rgba(expected);

    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = region.size();
    QByteArray tile(Gpu::RgbaTile::ByteCount, '\0');
    const auto initialRgba = rgba(initial);
    for (int y = 0; y < 64; ++y)
        std::memcpy(tile.data() + y * 256 * 4, initialRgba.constData() + y * 64 * 4, 64 * 4);
    raster.layers.append({QStringLiteral("Mixed alpha"), true,
        QUuid(QStringLiteral("{4ad3d278-ec82-4dd6-9a89-94e589c1f45b}")), {{{0, 0}, tile}}});
    auto owner = Gpu::PaintImageSP::create(execution, raster);
    const auto layer = owner->operations().activeLayer();
    const auto mask = Gpu::SelectionMask::fromAlphaCoverage(region, alpha, region);
    QVERIFY(mask);
    QCOMPARE(owner->setSelectionMask(mask), Gpu::Result::Success);
    Gpu::Brush brush;
    brush.channelWriteMask = quint8(writeMask);
    Gpu::FigureStyle style;
    style.outline = false;
    style.fill = LibrePaint::Image::Gpu::RgbaPixel{17, 129, 241, 255};
    brush.opacity = opacity;
    QCOMPARE(owner->paintFigure(layer, brush, {{{16, 16}, 1}, {{48, 48}, 1}},
        {Gpu::ShapeKind::Ellipse, {16, 16}, {48, 48}, 0}, style, 42), Gpu::Result::Success);
    const auto actual = owner->readRgbaPixels(layer, region, expected.size());
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        for (const auto &entry : {std::pair{QStringLiteral("expected"), expected},
                                  std::pair{QStringLiteral("actual"), actual.bytes}}) {
            QFile file(QDir(results).filePath(QStringLiteral("fractional-fill-%1-%2.rgba")
                .arg(QString::fromLatin1(QTest::currentDataTag()), entry.first)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), qint64(entry.second.size()));
        }
    }
    QCOMPARE(actual.bytes, expected);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, region, expected.size()).bytes, initialRgba);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, region, expected.size()).bytes, expected);

}

void KisEllipseToolDocumentTest::nativeSolidFillsKeepOrdinaryEditing_data()
{
    QTest::addColumn<FigureKind>("figure");
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("closeOnError");
    QTest::addColumn<bool>("canonical");
    QTest::addColumn<bool>("closeOnPublish");
    QTest::addColumn<int>("fillStyle");
    QTest::addColumn<int>("outlineStyle");
    QTest::addColumn<bool>("words");
    for (const auto figure : {FigureKind::Ellipse, FigureKind::Rectangle, FigureKind::RoundedRectangle, FigureKind::Polygon}) {
        for (const bool words : {false, true}) for (const bool outlined : {false, true}) {
            const auto name = QString::number(int(figure)) + (words ? QStringLiteral("-u16") : QStringLiteral("-u8")) +
                (outlined ? QStringLiteral("-outline") : QStringLiteral("-solid"));
            QTest::newRow(qPrintable(name)) << figure << true << false << false << false
                << int(figure == FigureKind::Ellipse ? KisFigurePaintingOptions::FillStyleForegroundColor :
                                                      KisFigurePaintingOptions::FillStyleBackgroundColor)
                << int(outlined ? KisFigurePaintingOptions::StrokeStyleForeground : KisFigurePaintingOptions::StrokeStyleNone) << words;
        }
    }
}

void KisEllipseToolDocumentTest::nativeSolidFillsKeepOrdinaryEditing()
{
    QFETCH(bool, words);
    QFETCH(int, outlineStyle);
    const auto preset = outlineStyle == KisFigurePaintingOptions::StrokeStyleNone ? m_fillPreset : m_preset;
    QByteArray cpu, gpu;
    exerciseFigure(preset, false, &cpu, 1, words);
    if (QTest::currentTestFailed()) return;
    exerciseFigure(preset, true, &gpu, 1, words);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(gpu, cpu);
}

void KisEllipseToolDocumentTest::standardBrushShapesKeepOrdinaryEditing_data()
{
    QTest::addColumn<FigureKind>("figure");
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("closeOnError");
    QTest::addColumn<bool>("canonical");
    QTest::addColumn<bool>("closeOnPublish");
    QTest::addColumn<int>("fillStyle");
    QTest::addColumn<int>("outlineStyle");
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("presetName");
    for (const auto figure : {FigureKind::Ellipse, FigureKind::Rectangle, FigureKind::RoundedRectangle, FigureKind::Polygon}) {
        for (const bool words : {false, true}) {
            const auto name = QString::number(int(figure)) + (words ? QStringLiteral("-u16") : QStringLiteral("-u8"));
            QTest::newRow(qPrintable(name)) << figure << true << false << false << false
                << int(KisFigurePaintingOptions::FillStyleNone)
                << int(KisFigurePaintingOptions::StrokeStyleForeground) << words << QString();
            QTest::newRow(qPrintable(QStringLiteral("paper-") + name)) << figure << true << false << false << false
                << int(KisFigurePaintingOptions::FillStyleNone)
                << int(KisFigurePaintingOptions::StrokeStyleForeground) << words << QStringLiteral("d)_Ink-7_Brush_Rough.kpp");
            QTest::newRow(qPrintable(QStringLiteral("screen-") + name)) << figure << true << false << false << false
                << int(KisFigurePaintingOptions::FillStyleNone)
                << int(KisFigurePaintingOptions::StrokeStyleForeground) << words << QStringLiteral("y)_Screentones_Regular.kpp");
            QTest::newRow(qPrintable(QStringLiteral("dither-") + name)) << figure << true << false << false << false
                << int(KisFigurePaintingOptions::FillStyleNone)
                << int(KisFigurePaintingOptions::StrokeStyleForeground) << words << QStringLiteral("u)_Pixel_Art_Dithering.kpp");
            const bool patternFill = figure == FigureKind::Rectangle || figure == FigureKind::Polygon;
            const QList<QPair<QString, QString>> fillPresets = patternFill ?
                QList<QPair<QString, QString>>{{QStringLiteral("screen"), QStringLiteral("y)_Screentones_Regular.kpp")},
                                               {QStringLiteral("dither"), QStringLiteral("u)_Pixel_Art_Dithering.kpp")}} :
                QList<QPair<QString, QString>>{{QStringLiteral("basic"), QString()},
                                               {QStringLiteral("paper"), QStringLiteral("d)_Ink-7_Brush_Rough.kpp")}};
            for (const auto &preset : fillPresets)
                QTest::newRow(qPrintable(QStringLiteral("generated-fill-") + preset.first + QLatin1Char('-') + name))
                    << figure << true << false << false << false
                    << int(patternFill ? KisFigurePaintingOptions::FillStylePattern : KisFigurePaintingOptions::FillStyleBackgroundColor)
                    << int(KisFigurePaintingOptions::StrokeStyleForeground) << words << preset.second;
            for (const auto *preset : {"u)_Pixel_Art.kpp", "u)_Pixel_Art_Fill.kpp", "h)_Charcoal_pencil_large.kpp",
                                       "h)_Charcoal_Pencil_Medium.kpp", "h)_Charcoal_Pencil_Thin.kpp"}) {
                if (figure != FigureKind::Ellipse && (QLatin1String(preset).contains(QLatin1String("Medium")) ||
                                                      QLatin1String(preset).contains(QLatin1String("Thin")))) continue;
                QTest::newRow(qPrintable(QString::fromLatin1(preset) + QLatin1Char('-') + name)) << figure << true << false << false << false
                    << int(KisFigurePaintingOptions::FillStyleNone)
                    << int(KisFigurePaintingOptions::StrokeStyleForeground) << words << QString::fromLatin1(preset);
            }
        }
    }
}

void KisEllipseToolDocumentTest::standardBrushShapesKeepOrdinaryEditing()
{
    QFETCH(bool, words);
    QFETCH(QString, presetName);
    const bool screentone = presetName == QLatin1String("y)_Screentones_Regular.kpp");
    const bool dither = presetName == QLatin1String("u)_Pixel_Art_Dithering.kpp");
    auto preset = m_fillPreset;
    if (!presetName.isEmpty()) {
        const auto bundle = KisResourceStorageSP::create(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle"));
        QVERIFY(bundle->valid());
        preset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
        QVERIFY(preset);
        if (preset->settings()->getBool("Texture/Pattern/Enabled")) {
            const auto file = QFileInfo(preset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
            const auto pattern = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
            QVERIFY(pattern);
            if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(pattern->md5Sum(), pattern->filename(), pattern->name())) {
                KisResourceModel patterns(ResourceType::Patterns); QVERIFY(patterns.addResource(pattern));
            }
        }
        preset->settings()->setProperty("Texture/Pattern/isRandomOffsetX", false);
        preset->settings()->setProperty("Texture/Pattern/isRandomOffsetY", false);
        preset->settings()->setProperty("Texture/Pattern/OffsetX", screentone || dither ? 0 : 7);
        preset->settings()->setProperty("Texture/Pattern/OffsetY", screentone || dither ? 0 : 11);
        preset->setFilename(QStringLiteral("paper-figure-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
        KisResourceModel presets(ResourceType::PaintOpPresets); QVERIFY(presets.addResource(preset));
    }
    QByteArray cpu, gpu;
    QHash<QString, QByteArray> nativeStates;
    exerciseFigure(preset, false, &cpu, 1, words, &nativeStates);
    if (QTest::currentTestFailed()) return;
    exerciseFigure(preset, true, &gpu, 1, words, &nativeStates);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(gpu, cpu);
}

void KisEllipseToolDocumentTest::selectedSolidFillsKeepOrdinaryEditing_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisEllipseToolDocumentTest::selectedSolidFillsKeepOrdinaryEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const bool hadEngine = qEnvironmentVariableIsSet("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (hadEngine) qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
        else qunsetenv("LIBREPAINT_GPU_ENGINE");
    });
    KisConfig(true).disableOpenGL();
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto foreground = QByteArray::fromHex(words ? "03f202820111ffff" : "f18111ff");
    const auto background = QByteArray::fromHex(words ? "042c051106ddffff" : "2c11ddff");
    const int pixelSize = words ? 8 : 4;
    QHash<QString, QByteArray> reference;
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto resultRoot = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto output = resultRoot.isEmpty() ? temporary.path() :
        QDir(resultRoot).filePath(QStringLiteral("selected-solid-") + QString::fromLatin1(QTest::currentDataTag()));
    QVERIFY(QDir().mkpath(output));
    for (const bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        QVERIFY(document->newImage(QStringLiteral("Selected solid editing"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        QCOMPARE(bool(document->image()->gpuImage()), gpu);
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
        window->resize(800, 600);
        auto *view = window->newView(document.get());
        QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        const auto closeWindow = qScopeGuard([&] {
            if (lifetime) {
                lifetime->setModified(false);
                window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            }
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            window.reset();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        window->viewManager()->setCurrentView(view);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
        const auto paintNode = [&]() -> KisNodeSP {
            for (auto node = lifetime->image()->root()->firstChild(); node; node = node->nextSibling())
                if (qobject_cast<KisPaintLayer *>(node.data())) return node;
            return {};
        };
        const auto pixels = [&] {
            lifetime->image()->waitForDone();
            QByteArray result(64 * 64 * pixelSize, Qt::Uninitialized);
            paintNode()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(result.data()), 0, 0, 64, 64);
            return result;
        };
        const auto compare = [&](const QString &stage) {
            const auto actual = pixels();
            QFile artifact(QDir(output).filePath((gpu ? QStringLiteral("gpu-") : QStringLiteral("cpu-")) + stage + QStringLiteral(".bgra")));
            if (!artifact.open(QIODevice::WriteOnly) || artifact.write(actual) != actual.size()) return false;
            if (!gpu) reference.insert(stage, actual);
            return !gpu || actual == reference.value(stage);
        };
        QByteArray coverage(64 * 64, '\0');
        const quint8 values[] = {0, 1, 127, 128, 254, 255};
        for (int y = 16; y < 48; ++y) for (int x = 16; x < 48; ++x)
            coverage[y * 64 + x] = char(values[(x / 4 + y / 4) % 6]);
        if (gpu) {
            const auto mask = Gpu::SelectionMask::fromAlphaCoverage(QRect(0, 0, 64, 64), coverage, QRect(0, 0, 64, 64));
            QVERIFY(mask);
            QVERIFY(lifetime->image()->setGpuSelectionMask(mask));
        } else {
            KisSelectionSP selection = new KisSelection;
            selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(coverage.constData()), 0, 0, 64, 64);
            KisSetGlobalSelectionCommand command(lifetime->image(), selection);
            command.redo();
        }
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setFGColor(KoColor(reinterpret_cast<const quint8 *>(foreground.constData()), cs));
        resources->setBGColor(KoColor(reinterpret_cast<const quint8 *>(background.constData()), cs));
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        resources->setEraserMode(false);
        resources->setGlobalAlphaLock(false);
        for (int step = 0; step < 5; ++step) {
            resources->setOpacity(step == 0 ? .7 : step == 2 ? .5 : 1);
            resources->setEraserMode(step == 2);
            const auto before = pixels();
            const auto index = lifetime->undoStack()->index();
            const auto name = step == 0 ? QStringLiteral("fill_selection_foreground_color_opacity") :
                              step == 1 || step == 4 ? QStringLiteral("fill_selection_background_color") :
                              step == 2 ? QStringLiteral("fill_selection_foreground_color_opacity") : QStringLiteral("clear");
            auto *action = window->viewManager()->actionManager()->actionByName(name);
            QVERIFY(action);
            QVERIFY(action->isEnabled());
            action->trigger();
            lifetime->image()->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), index + 1);
            const auto after = pixels();
            QVERIFY(after != before);
            QVERIFY2(compare(QString::number(step)), "Selected fill/erase must retain native pixels and channel precision");
            lifetime->undoStack()->undo();
            QCOMPARE(pixels(), before);
            lifetime->undoStack()->redo();
            QCOMPARE(pixels(), after);
            if (step != 3) continue;
            const auto path = QDir(output).filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("cpu.kra"));
            lifetime->setFileBatchMode(true);
            QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
            lifetime->waitForSavingToComplete();
            QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
            QCOMPARE(bool(lifetime->image()->gpuImage()), gpu);
            window->viewManager()->nodeManager()->slotUiActivatedNode(paintNode());
            QCOMPARE(pixels(), after);
        }
        if (gpu) {
            const auto current = lifetime->image()->gpuImage();
            LibrePaint::Canvas::Gpu::CanvasWindow *canvas = nullptr;
            for (auto *candidate : QGuiApplication::allWindows())
                if (auto *value = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate); value && value->isVisible()) canvas = value;
            QVERIFY(canvas);
            QSignalSpy frames(canvas, &LibrePaint::Canvas::Gpu::CanvasWindow::frameSubmitted);
            const auto visible = current->operations().snapshot();
            canvas->refresh();
            QTRY_VERIFY(std::any_of(frames.cbegin(), frames.cend(), [&](const auto &frame) {
                return frame[0].toULongLong() == current->handle() && frame[1].toULongLong() == visible->revision();
            }));
            QVERIFY(current->execution().device()->finish());
        }
        QVERIFY(window->screen()->grabWindow(window->winId()).save(QDir(output).filePath(
                    gpu ? QStringLiteral("gpu-window.png") : QStringLiteral("cpu-window.png"))));
    }
}

void KisEllipseToolDocumentTest::exerciseFigure(KisPaintOpPresetSP preset, bool gpuForPreset, QByteArray *firstPixels,
                                              qreal opacity, bool words, QHash<QString, QByteArray> *nativeStates)
{
    QFETCH(bool, gpu);
    const bool selectedPresetFill = bool(preset);
    if (selectedPresetFill) gpu = gpuForPreset;
    QFETCH(bool, closeOnError);
    QFETCH(bool, canonical);
    QFETCH(FigureKind, figure);
    QFETCH(bool, closeOnPublish);
    QFETCH(int, fillStyle);
    QFETCH(int, outlineStyle);
    const bool solid = fillStyle != KisFigurePaintingOptions::FillStyleNone;
    const auto foregroundPixel = QByteArray::fromHex(words ? "03f202820111ffff" : "f18111ff");
    const auto backgroundPixel = QByteArray::fromHex(words ? "042c051106ddffff" : "2c11ddff");
    const int pixelSize = words ? 8 : 4;
    const QByteArray zeroPixel(pixelSize, char(0));
    auto fillPixel = fillStyle == KisFigurePaintingOptions::FillStyleForegroundColor ? foregroundPixel : backgroundPixel;
    if (opacity == 0) fillPixel.fill('\0');
    else if (words) {
        const quint16 alpha = quint16(std::nearbyint(float(opacity) * 65535.0f));
        std::memcpy(fillPixel.data() + 6, &alpha, sizeof(alpha));
    } else fillPixel[3] = char(std::nearbyint(float(opacity) * 255.0f));
    const QPoint outlineProbe(32, solid ? 14 : 18);
    const bool hasOutline = outlineStyle != KisFigurePaintingOptions::StrokeStyleNone;
    auto firstOutlinePixel = hasOutline ? foregroundPixel : zeroPixel;
    const bool rounded = figure == FigureKind::RoundedRectangle;
    const bool rectangle = rounded || figure == FigureKind::Rectangle;
    const bool polygon = figure == FigureKind::Polygon;
    const bool pathFigure = polygon || figure == FigureKind::Polyline;
    namespace Gpu = LibrePaint::Image::Gpu;
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                       QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = settings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = settings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreRenderer = qScopeGuard([&] {
        if (hadRenderer) settings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else settings.remove(QStringLiteral("OpenGLRenderer"));
        settings.sync();
    });
    KisConfig(true).disableOpenGL();
    Gpu::DevicePtr device;
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        Gpu::RasterImage raster;
        raster.size = {64, 64};
        raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
        raster.colorProfile = cs->profile()->rawData();
        raster.layers.append({QStringLiteral("Outline"), true, QUuid(QStringLiteral("{9606c464-7658-4e24-bb48-0b621218ae53}")), {}});
        owner = Gpu::PaintImageSP::create(execution, raster);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Ellipse input"), cs));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Ellipse input"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<KoToolBase> tool;
    QPointer<QWindow> native;
    QPointer<QWidget> input(view->canvasBase()->canvasWidget());
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (tool) tool->requestStrokeCancellation();
        if (pressed && native) QTest::mouseRelease(native.data(), Qt::LeftButton);
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    KisImageWSP image = lifetime->image();
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset ? preset : m_preset);
    resources->setSize(8);
    resources->setOpacity(opacity);
    const KoColor foreground(reinterpret_cast<const quint8 *>(foregroundPixel.constData()), cs);
    const KoColor background(reinterpret_cast<const quint8 *>(backgroundPixel.constData()), cs);
    resources->setFGColor(foreground);
    resources->setBGColor(background);
    if (fillStyle == KisFigurePaintingOptions::FillStylePattern) {
        const auto pattern = KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns)
            .bestMatch({}, QStringLiteral("pattern.pat"), {});
        QVERIFY(pattern);
        resources->slotPatternActivated(pattern);
    }
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    const auto toolId = polygon ? QStringLiteral("KisToolPolygon") :
                        pathFigure ? QStringLiteral("KisToolPolyline") :
                        rectangle ? QStringLiteral("KritaShape/KisToolRectangle") : QStringLiteral("KritaShape/KisToolEllipse");
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = KoToolManager::instance()->toolById(view->canvasBase(), toolId);
    QVERIFY(tool);
    if (rectangle) {
        QVERIFY(QMetaObject::invokeMethod(tool, "roundCornersChanged", Qt::DirectConnection,
                                         Q_ARG(int, rounded ? 14 : 0), Q_ARG(int, rounded ? 14 : 0)));
    }
    QPointer<QComboBox> outline, fill;
    for (QWidget *panel : tool->optionWidgets()) {
        if (auto *value = panel->findChild<QComboBox*>(QStringLiteral("cmbOutline"))) outline = value;
        if (auto *value = panel->findChild<QComboBox*>(QStringLiteral("cmbFill"))) fill = value;
    }
    QVERIFY(outline);
    QVERIFY(fill);
    const int previousOutline = outline->currentIndex(), previousFill = fill->currentIndex();
    const auto restoreOptions = qScopeGuard([&] {
        if (outline) outline->setCurrentIndex(previousOutline);
        if (fill) fill->setCurrentIndex(previousFill);
    });
    fill->setCurrentIndex(fillStyle);
    outline->setCurrentIndex(outlineStyle);
    view->canvasBase()->snapGuide()->enableSnapping(false);
    input->setFocus();
    QTRY_VERIFY(input->width() >= 64 && input->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto size = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({size.width() / 2, size.height() / 2});
    if (gpu) {
        if (selectedPresetFill && QGuiApplication::platformName() == QLatin1String("offscreen")) {
            QCOMPARE(execution->beginReadbackFrame().code, Gpu::ExecutionCode::Success);
            auto cancelFrame = qScopeGuard([&] { execution->cancelFrame(); });
            QCOMPARE(execution->project(owner->handle(), QRect(0, 0, 64, 64)).code, Gpu::RenderOutcome::Recorded);
            const auto submitted = execution->submitFrame();
            cancelFrame.dismiss();
            QCOMPARE(submitted.code, Gpu::ExecutionCode::Success);
        }
        QTRY_COMPARE(execution->published(owner->handle()), owner->operations().snapshot());
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = candidate;
        QVERIFY(native);
    } else native = window->windowHandle();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
    QTest::qWait(100);
    const auto currentPresetCacheIsValid = [&] {
        const auto manager = view->canvasBase()->resourceManager();
        const auto cache = manager->resource(KoCanvasResource::CurrentPaintOpPresetCache).value<KoResourceCacheInterfaceSP>();
        const auto preset = manager->resource(KoCanvasResource::CurrentPaintOpPreset).value<KisPaintOpPresetSP>();
        return cache && preset && preset->sanityCheckResourceCacheIsValid(cache);
    };
    if (!gpu) QTRY_VERIFY(currentPresetCacheIsValid());
    const auto point = [&](QPointF imagePoint) {
        const auto p = view->canvasBase()->coordinatesConverter()->imageToWidget(imagePoint).toPoint();
        return gpu ? p : input->mapTo(window.get(), p);
    };
    const auto start = [&] {
        if (pathFigure) {
            const QVector<QPointF> vertices = canonical || solid ? QVector<QPointF>{{16, 16}, {48, 16}, {48, 48}} :
                                                        QVector<QPointF>{{16, 16}, {48, 16}};
            for (const auto &vertex : vertices) {
                if (canonical) {
                    const auto documentPoint = view->canvasBase()->coordinatesConverter()->imageToDocument(vertex);
                    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    KoPointerEvent first(&press, documentPoint);
                    qobject_cast<KisTool*>(tool.data())->beginPrimaryAction(&first);
                    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    KoPointerEvent last(&release, documentPoint);
                    qobject_cast<KisTool*>(tool.data())->endPrimaryAction(&last);
                } else {
                    QTest::mouseMove(native.data(), point(vertex));
                    QTest::qWait(10);
                    QEnterEvent entered(point(vertex), point(vertex), native->mapToGlobal(point(vertex)));
                    QApplication::sendEvent(native.data(), &entered);
                    QTest::mouseClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
                }
            }
            return;
        }
        if (canonical) {
            const auto converter = view->canvasBase()->coordinatesConverter();
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            KoPointerEvent first(&press, converter->imageToDocument(QPointF(16, 16)));
            qobject_cast<KisTool*>(tool.data())->beginPrimaryAction(&first);
            QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            KoPointerEvent last(&move, converter->imageToDocument(QPointF(48, 48)));
            qobject_cast<KisTool*>(tool.data())->continuePrimaryAction(&last);
            return;
        }
        QTest::mouseMove(native.data(), point({16, 16}));
        QTest::qWait(10);
        const auto p = point({16, 16});
        QEnterEvent entered(p, p, native->mapToGlobal(p));
        QApplication::sendEvent(native.data(), &entered);
        pressed = true;
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, p);
        QTest::mouseMove(native.data(), point({48, 48}));
        QTest::qWait(10);
    };
    const auto release = [&] {
        if (canonical) {
            tool->requestStrokeEnd();
            return;
        }
        if (pathFigure) {
            pressed = true;
            QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point({48, 48}));
            pressed = false;
            return;
        }
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({48, 48}));
        pressed = false;
    };
    const auto pixel = [&](QPoint p) {
        QByteArray bytes(pixelSize, '\0');
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8*>(bytes.data()), p.x(), p.y(), 1, 1);
        return bytes;
    };
    const auto compareNativeState = [&](const QString &stage) {
        if (!nativeStates) return true;
        QByteArray actual(64 * 64 * pixelSize, Qt::Uninitialized);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 0, 0, 64, 64);
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            const auto output = QDir(results).filePath(QStringLiteral("standard-brush-shape-") +
                                                     QString::fromLatin1(QTest::currentDataTag()));
            if (!QDir().mkpath(output)) return false;
            QFile artifact(QDir(output).filePath((gpu ? QStringLiteral("gpu-") : QStringLiteral("cpu-")) + stage + QStringLiteral(".bgra")));
            if (!artifact.open(QIODevice::WriteOnly) || artifact.write(actual) != actual.size()) return false;
        }
        if (!gpu) nativeStates->insert(stage, actual);
        return !gpu || actual == nativeStates->value(stage);
    };
    if (closeOnPublish) {
        const auto before = owner->operations().snapshot();
        bool closing = false;
        connect(owner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            if (closing) return;
            if (selectedPresetFill) {
                if (owner->operations().snapshot() == before) return;
            } else if (!owner->operations().hasActiveStroke()) return;
            closing = true;
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        start();
        release();
        QTRY_VERIFY(!lifetime);
        QTRY_VERIFY(!native);
        if (selectedPresetFill) {
            QVERIFY(owner->operations().snapshot() != before);
            QCOMPARE(owner->operations().undoCount(), 1);
            QVERIFY(!tool);
        } else QCOMPARE(owner->operations().snapshot(), before);
        QVERIFY(!owner->operations().hasActiveStroke());
        return;
    }
    if (closeOnError) {
        fill->setCurrentIndex(KisFigurePaintingOptions::FillStylePattern);
        resources->slotPatternActivated(KoPatternSP());
        start();
        const QString expected = i18n("No usable pattern is selected for this document.");
        QSignalSpy status(tool, &KoToolBase::statusTextChanged);
        connect(tool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
            if (message != expected) return;
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        release();
        QTRY_VERIFY(!lifetime);
        QVERIFY(!native);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QVERIFY(std::any_of(status.begin(), status.end(), [&](const QList<QVariant> &values) { return values.first().toString() == expected; }));
        return;
    }
    const auto selectedPreset = view->canvasBase()->resourceManager()->resource(KoCanvasResource::CurrentPaintOpPreset)
        .value<KisPaintOpPresetSP>();
    const auto presetSettings = selectedPreset->settings()->toXML();
    QSignalSpy diagnostics(tool, &KoToolBase::statusTextChanged);
    start();
    if (!gpu) QTRY_VERIFY(currentPresetCacheIsValid());
    QCOMPARE(lifetime->undoStack()->count(), 0);
    QCOMPARE(pixel({32, 32}), zeroPixel);
    try {
        release();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    if (selectedPresetFill && !diagnostics.isEmpty())
        qInfo() << "Fill-only tool diagnostic:" << diagnostics.last().first().toString();
    QTRY_COMPARE(lifetime->undoStack()->count(), 1);
    if (selectedPresetFill) {
        QCOMPARE(view->canvasBase()->resourceManager()->resource(KoCanvasResource::CurrentPaintOpPreset)
                     .value<KisPaintOpPresetSP>(), selectedPreset);
        QCOMPARE(selectedPreset->settings()->toXML(), presetSettings);
    }
    if (firstPixels) {
        firstPixels->resize(64 * 64 * pixelSize);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(firstPixels->data()), 0, 0, 64, 64);
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            const auto name = QStringLiteral("selected-preset-fill-%1-%2")
                .arg(gpu ? QStringLiteral("gpu") : QStringLiteral("ordinary"), QString::number(int(figure))) +
                (opacity == 1 && qstrcmp(QTest::currentTestFunction(), "nativeSolidFillsKeepOrdinaryEditing") != 0 ? QString() :
                 QLatin1Char('-') + QString::fromLatin1(QTest::currentDataTag()));
            QFile file(QDir(results).filePath(name + QStringLiteral(".pixels")));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(*firstPixels), qint64(firstPixels->size()));
        }
    }
    QVERIFY2(compareNativeState(QStringLiteral("painted")), "The selected brush must preserve every native figure pixel");
    if (nativeStates) firstOutlinePixel = pixel(outlineProbe);
    else QCOMPARE(pixel(outlineProbe), firstOutlinePixel);
    if (!nativeStates && solid) {
        QCOMPARE(pixel({40, 24}), fillPixel);
    } else if (!nativeStates) {
        QCOMPARE(pixel({32, 32}), polygon ? foregroundPixel : zeroPixel);
        QCOMPARE(pixel({16, 16}), (rectangle && !rounded) || pathFigure ? foregroundPixel : zeroPixel);
    }
    if (gpu) {
        const auto operation = std::get<Gpu::PaintSnapshot>(owner->operations().snapshot()->layers.first().operations.last());
        QCOMPARE(operation->shape.kind, polygon ? Gpu::ShapeKind::Polygon :
                                       pathFigure ? Gpu::ShapeKind::Polyline : rounded ? Gpu::ShapeKind::RoundedRectangle :
                                       rectangle ? Gpu::ShapeKind::Rectangle : Gpu::ShapeKind::Ellipse);
        QCOMPARE(operation->shape.cornerRadii, rounded ? QSizeF(14, 14) : QSizeF(0, 0));
        QCOMPARE(owner->operations().historyAction(1), polygon ? Gpu::HistoryAction::DrawPolygon :
                                                                  pathFigure ? Gpu::HistoryAction::DrawPolyline : rectangle ? Gpu::HistoryAction::DrawRectangle : Gpu::HistoryAction::DrawEllipse);
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(pixel(outlineProbe), zeroPixel);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(pixel(outlineProbe), firstOutlinePixel);
    if (firstPixels) {
        QByteArray redone(64 * 64 * pixelSize, Qt::Uninitialized);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(redone.data()), 0, 0, 64, 64);
        QCOMPARE(redone, *firstPixels);
    }
    resources->setOpacity(1);
    start();
    tool->requestStrokeCancellation();
    if (!pathFigure) release();
    QCOMPARE(lifetime->undoStack()->count(), 1);
    if (selectedPresetFill && solid) fill->setCurrentIndex(KisFigurePaintingOptions::FillStyleBackgroundColor);
    else outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleBackground);
    const QPoint editProbe = selectedPresetFill && solid ? QPoint(40, 24) : outlineProbe;
    start();
    release();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 2);
    QVERIFY2(compareNativeState(QStringLiteral("edited")), "Background-color editing must preserve every native figure pixel");
    const auto editedPixel = pixel(editProbe);
    if (!nativeStates) QCOMPARE(editedPixel, backgroundPixel);
    QCOMPARE(resources->fgColor(), foreground);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("ellipse.kra"));
    QSignalSpy saveFinished(lifetime.data(), &KisDocument::sigSavingFinished);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QTRY_VERIFY(!saveFinished.isEmpty());
    QTRY_VERIFY(!lifetime->isSaving());
    QTRY_VERIFY(!lifetime->isModified());
    QCOMPARE(lifetime->undoStack()->index(), 2);
    QCOMPARE(lifetime->undoStack()->cleanIndex(), 2);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto saved = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(saved.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(saved.diagnostic));
    QCOMPARE(saved.document.image.layers.size(), 1);
    const auto rgbaPixel = [&](QByteArray bgra) {
        for (int i = 0; i < (words ? 2 : 1); ++i) std::swap(bgra[i], bgra[(words ? 4 : 2) + i]);
        return bgra;
    };
    QCOMPARE(saved.document.image.layers.first().tiles.first().rgba.mid(
                 (editProbe.y() * 256 + editProbe.x()) * pixelSize, pixelSize), rgbaPixel(editedPixel));
    if (gpu && firstPixels) {
        const auto opened = LibrePaint::Document::Gpu::PaintDocument::openKra(execution, path);
        QVERIFY2(opened.code == LibrePaint::Document::Gpu::FileCode::Success, qPrintable(opened.diagnostic));
        QByteArray pixels(64 * 64 * pixelSize, Qt::Uninitialized);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), 0, 0, 64, 64);
        for (qsizetype i = 0; i < pixels.size(); i += pixelSize)
            for (int c = 0; c < (words ? 2 : 1); ++c) std::swap(pixels[i + c], pixels[i + (words ? 4 : 2) + c]);
        const auto reopened = opened.document->image();
        const auto read = reopened->readRgbaPixels(reopened->operations().activeLayer(), QRect(0, 0, 64, 64), pixels.size());
        QCOMPARE(read.code, Gpu::ExecutionCode::Success);
        QCOMPARE(read.bytes, pixels);
        const auto layer = reopened->operations().activeLayer();
        const Gpu::RgbaPixel color = words ? Gpu::RgbaPixel{4353, 33282, 61955, 65535} : Gpu::RgbaPixel{17, 129, 241, 255};
        QCOMPARE(reopened->fillLayerColor(layer, color), Gpu::Result::Success);
        const auto edited = reopened->readRgbaPixels(layer, QRect(0, 0, 64, 64), pixels.size());
        QCOMPARE(edited.code, Gpu::ExecutionCode::Success);
        QCOMPARE(edited.bytes.mid(0, pixelSize), rgbaPixel(foregroundPixel));
        QCOMPARE(reopened->undo(), Gpu::Result::Success);
        QCOMPARE(reopened->readRgbaPixels(layer, QRect(0, 0, 64, 64), pixels.size()).bytes, pixels);
        QCOMPARE(reopened->redo(), Gpu::Result::Success);
        QCOMPARE(reopened->readRgbaPixels(layer, QRect(0, 0, 64, 64), pixels.size()).bytes, edited.bytes);
    }
    resources->setEraserMode(true);
    start();
    release();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 3);
    auto erasedPixel = backgroundPixel;
    erasedPixel.replace(words ? 6 : 3, words ? 2 : 1, QByteArray(words ? 2 : 1, char(0)));
    QVERIFY2(compareNativeState(QStringLiteral("erased")), "Eraser mode must preserve every native figure pixel");
    if (!nativeStates) QCOMPARE(pixel(editProbe), erasedPixel);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(pixel(editProbe), editedPixel);
    QTRY_COMPARE(lifetime->undoStack()->index(), 2);
    QTRY_VERIFY2(!lifetime->isModified(), qPrintable(QStringLiteral("index=%1 count=%2 clean=%3 saving=%4")
                 .arg(lifetime->undoStack()->index()).arg(lifetime->undoStack()->count())
                 .arg(lifetime->undoStack()->cleanIndex()).arg(lifetime->isSaving())));
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty() && (nativeStates || qstrcmp(QTest::currentTestFunction(), "nativeSolidFillsKeepOrdinaryEditing") == 0)) {
        if (gpu) {
            auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(native.data());
            QVERIFY(canvas);
            QSignalSpy frames(canvas, &LibrePaint::Canvas::Gpu::CanvasWindow::frameSubmitted);
            const auto visible = owner->operations().snapshot();
            canvas->refresh();
            QTRY_VERIFY(std::any_of(frames.cbegin(), frames.cend(), [&](const auto &frame) {
                return frame[0].toULongLong() == owner->handle() && frame[1].toULongLong() == visible->revision();
            }));
            QVERIFY(execution->device()->finish());
        }
        const auto output = QDir(results).filePath(QString::fromLatin1(QTest::currentTestFunction()) + QLatin1Char('-') +
                                                 QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(output));
        QVERIFY(window->screen()->grabWindow(window->winId()).save(QDir(output).filePath(
                    gpu ? QStringLiteral("gpu-window.png") : QStringLiteral("cpu-window.png"))));
    }
    if (gpu && canonical && !results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const auto figurePrefix = polygon ? QStringLiteral("polygon-tool") : pathFigure ? QStringLiteral("polyline-tool") : rounded ? QStringLiteral("rounded-rectangle-tool") :
                                  rectangle ? QStringLiteral("rectangle-tool") : QStringLiteral("ellipse-tool");
        const auto prefix = (opacity == 1 ? QString() : QString::fromLatin1(QTest::currentDataTag()) + QLatin1Char('-')) + (solid ? (hasOutline ? QStringLiteral("filled-") : QStringLiteral("fill-only-")) : QString()) + figurePrefix;
        LibrePaint::Document::Gpu::PaintDocument fixed(owner);
        const auto saved = fixed.exportKra(QDir(results).filePath(prefix + QStringLiteral(".kra")));
        QVERIFY2(saved.code == LibrePaint::Document::Gpu::FileCode::Success, qPrintable(saved.diagnostic));
        KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
        configuration->setProperty("alpha", true);
        configuration->setProperty("interlaced", false);
        configuration->setProperty("compression", 3);
        configuration->setProperty("saveSRGBProfile", true);
        configuration->setProperty("forceSRGB", false);
        configuration->setProperty("storeAuthor", false);
        configuration->setProperty("storeMetaData", false);
        QVERIFY2(lifetime->exportDocumentSync(QDir(results).filePath(prefix + QStringLiteral(".png")),
                                             QByteArrayLiteral("image/png"), configuration),
                 qPrintable(lifetime->errorMessage()));
    }
}

KISTEST_MAIN(KisEllipseToolDocumentTest)
#include "KisEllipseToolDocumentTest.moc"
