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
#include <QTemporaryDir>
#include <KLocalizedString>
#include <KisFigurePaintingOptions.h>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoToolBase.h>
#include <KoSnapGuide.h>
#include <KoResourceCacheInterface.h>
#include <KoCanvasResourceProvider.h>
#include <KoCanvasResourcesIds.h>
#include <KoPointerEvent.h>
#include <KoToolManager.h>
#include <KoColorSpaceRegistry.h>
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
#include <kis_canvas2.h>
#include <kis_image.h>
#include <kis_tool.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
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
private:
    void addSolidFigureRows(bool gpu);
    void exerciseFigure(KisPaintOpPresetSP preset = {}, bool gpuForPreset = false, QByteArray *firstPixels = nullptr, qreal opacity = 1);
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
    row("rectangle-gpu-rounded-close-on-pattern-fill-rejection", FigureKind::RoundedRectangle, true, true);
    row("rectangle-gpu-rounded-canonical", FigureKind::RoundedRectangle, true, false, true);
    row("rectangle-gpu-canonical", FigureKind::Rectangle, true, false, true);
    row("rectangle-gpu-close-on-publish", FigureKind::Rectangle, true, false, false, true);
    row("polyline-ordinary", FigureKind::Polyline, false);
    row("polyline-gpu", FigureKind::Polyline, true);
    row("polyline-gpu-canonical", FigureKind::Polyline, true, false, true);
    row("polyline-gpu-close-on-pattern-fill-rejection", FigureKind::Polyline, true, true);
    row("polyline-gpu-close-on-publish", FigureKind::Polyline, true, false, false, true);
    row("polygon-ordinary", FigureKind::Polygon, false);
    row("polygon-gpu", FigureKind::Polygon, true);
    row("polygon-gpu-canonical", FigureKind::Polygon, true, false, true);
    row("polygon-gpu-close-on-pattern-fill-rejection", FigureKind::Polygon, true, true);
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

void KisEllipseToolDocumentTest::exerciseFigure(KisPaintOpPresetSP preset, bool gpuForPreset, QByteArray *firstPixels, qreal opacity)
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
    auto fillPixel = fillStyle == KisFigurePaintingOptions::FillStyleForegroundColor ?
        QByteArray::fromHex("f18111ff") : QByteArray::fromHex("2c11ddff");
    if (opacity == 0) fillPixel.fill('\0');
    else fillPixel[3] = char(std::nearbyint(float(opacity) * 255.0f));
    const QPoint outlineProbe(32, solid ? 14 : 18);
    const bool hasOutline = outlineStyle != KisFigurePaintingOptions::StrokeStyleNone;
    const auto firstOutlinePixel = hasOutline ? QByteArray::fromHex("f18111ff") : QByteArray(4, char(0));
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
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        Gpu::RasterImage raster;
        raster.size = {64, 64};
        raster.layers.append({QStringLiteral("Outline"), true, QUuid(QStringLiteral("{9606c464-7658-4e24-bb48-0b621218ae53}")), {}});
        owner = Gpu::PaintImageSP::create(execution, raster);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Ellipse input")));
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
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    resources->setBGColor(KoColor(QColor(221, 17, 44), cs));
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
        QByteArray bytes(4, '\0');
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8*>(bytes.data()), p.x(), p.y(), 1, 1);
        return bytes;
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
        start();
        const QString expected = i18n("Pattern fills are not yet supported for this GPU document.");
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
    QCOMPARE(pixel({32, 32}), QByteArray(4, '\0'));
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
        firstPixels->resize(64 * 64 * 4);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(firstPixels->data()), 0, 0, 64, 64);
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            const auto name = QStringLiteral("selected-preset-fill-%1-%2")
                .arg(gpu ? QStringLiteral("gpu") : QStringLiteral("ordinary"), QString::number(int(figure))) +
                (opacity == 1 ? QString() : QLatin1Char('-') + QString::fromLatin1(QTest::currentDataTag()));
            QFile file(QDir(results).filePath(name + QStringLiteral(".pixels")));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(*firstPixels), qint64(firstPixels->size()));
        }
    }
    QCOMPARE(pixel(outlineProbe), firstOutlinePixel);
    if (solid) {
        QCOMPARE(pixel({40, 24}), fillPixel);
    } else {
        QCOMPARE(pixel({32, 32}), polygon ? QByteArray::fromHex("f18111ff") : QByteArray(4, '\0'));
        QCOMPARE(pixel({16, 16}), (rectangle && !rounded) || pathFigure ? QByteArray::fromHex("f18111ff") : QByteArray(4, '\0'));
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
    QCOMPARE(pixel(outlineProbe), QByteArray(4, '\0'));
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(pixel(outlineProbe), firstOutlinePixel);
    if (firstPixels) {
        QByteArray redone(64 * 64 * 4, Qt::Uninitialized);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(redone.data()), 0, 0, 64, 64);
        QCOMPARE(redone, *firstPixels);
    }
    resources->setOpacity(1);
    start();
    tool->requestStrokeCancellation();
    if (!pathFigure) release();
    QCOMPARE(lifetime->undoStack()->count(), 1);
    if (selectedPresetFill) fill->setCurrentIndex(KisFigurePaintingOptions::FillStyleBackgroundColor);
    else outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleBackground);
    const QPoint editProbe = selectedPresetFill ? QPoint(40, 24) : outlineProbe;
    start();
    release();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 2);
    QCOMPARE(pixel(editProbe), QByteArray::fromHex("2c11ddff"));
    QCOMPARE(resources->fgColor().toQColor(), QColor(17, 129, 241));
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
    QCOMPARE(saved.document.image.layers.first().tiles.first().rgba.mid((editProbe.y() * 256 + editProbe.x()) * 4, 4), QByteArray::fromHex("dd112cff"));
    if (gpu && firstPixels) {
        const auto opened = LibrePaint::Document::Gpu::PaintDocument::openKra(execution, path);
        QVERIFY2(opened.code == LibrePaint::Document::Gpu::FileCode::Success, qPrintable(opened.diagnostic));
        QByteArray pixels(64 * 64 * 4, Qt::Uninitialized);
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), 0, 0, 64, 64);
        for (qsizetype i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
        const auto reopened = opened.document->image();
        const auto read = reopened->readRgbaPixels(reopened->operations().activeLayer(), QRect(0, 0, 64, 64), pixels.size());
        QCOMPARE(read.code, Gpu::ExecutionCode::Success);
        QCOMPARE(read.bytes, pixels);
    }
    resources->setEraserMode(true);
    start();
    release();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 3);
    QCOMPARE(pixel(editProbe), QByteArray::fromHex("2c11dd00"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(pixel(editProbe), QByteArray::fromHex("2c11ddff"));
    QTRY_COMPARE(lifetime->undoStack()->index(), 2);
    QTRY_VERIFY2(!lifetime->isModified(), qPrintable(QStringLiteral("index=%1 count=%2 clean=%3 saving=%4")
                 .arg(lifetime->undoStack()->index()).arg(lifetime->undoStack()->count())
                 .arg(lifetime->undoStack()->cleanIndex()).arg(lifetime->isSaving())));
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
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
