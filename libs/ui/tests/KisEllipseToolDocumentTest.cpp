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
#include <kis_properties_configuration.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <algorithm>
#include <memory>

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
    }
    void drawingPreservesShapeHistoryAndSaving_data();
    void drawingPreservesShapeHistoryAndSaving();
    void solidFillsPreserveShapeHistoryAndSaving_data();
    void solidFillsPreserveShapeHistoryAndSaving();
    void ordinarySolidFillsPreserveShapeHistoryAndSaving_data();
    void ordinarySolidFillsPreserveShapeHistoryAndSaving();
private:
    void addSolidFigureRows(bool gpu);
    void exerciseFigure();
    KisPaintOpPresetSP m_preset;
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

void KisEllipseToolDocumentTest::exerciseFigure()
{
    QFETCH(bool, gpu);
    QFETCH(bool, closeOnError);
    QFETCH(bool, canonical);
    QFETCH(FigureKind, figure);
    QFETCH(bool, closeOnPublish);
    QFETCH(int, fillStyle);
    QFETCH(int, outlineStyle);
    const bool solid = fillStyle != KisFigurePaintingOptions::FillStyleNone;
    const auto fillPixel = fillStyle == KisFigurePaintingOptions::FillStyleForegroundColor ?
        QByteArray::fromHex("f18111ff") : QByteArray::fromHex("2c11ddff");
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
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
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
            if (closing || !owner->operations().hasActiveStroke()) return;
            closing = true;
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        start();
        release();
        QTRY_VERIFY(!lifetime);
        QTRY_VERIFY(!native);
        QCOMPARE(owner->operations().snapshot(), before);
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
    QTRY_COMPARE(lifetime->undoStack()->count(), 1);
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
    start();
    tool->requestStrokeCancellation();
    if (!pathFigure) release();
    QCOMPARE(lifetime->undoStack()->count(), 1);
    outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleBackground);
    start();
    release();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 2);
    QCOMPARE(pixel(outlineProbe), QByteArray::fromHex("2c11ddff"));
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
    QCOMPARE(saved.document.image.layers.first().tiles.first().rgba.mid((outlineProbe.y() * 256 + outlineProbe.x()) * 4, 4), QByteArray::fromHex("dd112cff"));
    resources->setEraserMode(true);
    start();
    release();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 3);
    QCOMPARE(pixel(outlineProbe), QByteArray::fromHex("2c11dd00"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(pixel(outlineProbe), QByteArray::fromHex("2c11ddff"));
    QTRY_COMPARE(lifetime->undoStack()->index(), 2);
    QTRY_VERIFY2(!lifetime->isModified(), qPrintable(QStringLiteral("index=%1 count=%2 clean=%3 saving=%4")
                 .arg(lifetime->undoStack()->index()).arg(lifetime->undoStack()->count())
                 .arg(lifetime->undoStack()->cleanIndex()).arg(lifetime->isSaving())));
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (canonical && !results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const auto figurePrefix = polygon ? QStringLiteral("polygon-tool") : pathFigure ? QStringLiteral("polyline-tool") : rounded ? QStringLiteral("rounded-rectangle-tool") :
                                  rectangle ? QStringLiteral("rectangle-tool") : QStringLiteral("ellipse-tool");
        const auto prefix = (solid ? (hasOutline ? QStringLiteral("filled-") : QStringLiteral("fill-only-")) : QString()) + figurePrefix;
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
