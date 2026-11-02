/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QDockWidget>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QIcon>
#include <QMouseEvent>
#include <QPointer>
#include <QRadioButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QDomDocument>
#include <quazip.h>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoPointerEvent.h>
#include <KoSnapGuide.h>
#include <KoToolManager.h>
#include <actions/input/KisApplicationInputActions.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_preset.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <input/ui/kis_input_profile_manager.h>
#include <kis_canvas2.h>
#include <kis_config.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_adjustment_layer.h>
#include <kis_filter_mask.h>
#include <kis_transparency_mask.h>
#include <filter/kis_filter_registry.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <kis_node.h>
#include <commands/kis_node_property_list_command.h>
#include <commands/kis_set_global_selection_command.h>
#include <kis_undo_adapter.h>
#include <kis_layer_properties_icons.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_default_bounds.h>
#include <KisRegion.h>
#include <KoColorProfile.h>
#include <KoCompositeOpRegistry.h>
#include <cstring>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <nodes/kis_node_manager.h>
#include <kis_tool.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <algorithm>
#include <memory>

#include <kis_transform_worker.h>
#include "../../../plugins/tools/tool_transform2/kis_transform_utils.h"
#include "../../../plugins/tools/tool_transform2/kis_tool_transform.h"
namespace {
QByteArray exchangeRgbaBgra(QByteArray bytes, int pixelBytes)
{
    const int channelBytes = pixelBytes / 4;
    for (qsizetype pixel = 0; pixel < bytes.size(); pixel += pixelBytes)
        for (int byte = 0; byte < channelBytes; ++byte)
            std::swap(bytes[pixel + byte], bytes[pixel + 2 * channelBytes + byte]);
    return bytes;
}

LibrePaint::Image::Gpu::SelectionMaskSP generatedCoverage(const LibrePaint::Image::Gpu::PaintImageSP &owner,
    const LibrePaint::Image::Gpu::SelectionMaskSP &mask)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    Gpu::AffinePass pass; pass.kind = Gpu::AffinePass::Remap;
    pass.source = pass.destination = mask->bounds(); pass.mapWholeLayer = true;
    Gpu::AffinePlan plan; plan.passes = {pass};
    return owner->transformedSelectionMask(mask, plan);
}

}
class KisTransformToolDocumentTest : public QObject
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
        const QString path = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(path);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(path, bundle));
        m_preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(m_preset);
        m_preset->setName(QStringLiteral("Transform tool document fixture"));
        m_preset->setFilename(QStringLiteral("transform-tool-document-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        m_preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(m_preset));
    }
    void nativeAffineEditingKeepsPixelsHistoryAndKra_data();
    void nativeAffineEditingKeepsPixelsHistoryAndKra();
    void internalMasksMoveAndTransformThroughHistoryAndKra_data();
    void internalMasksMoveAndTransformThroughHistoryAndKra();
    void previewNotificationCanCancelOrCloseDocument_data();
    void previewNotificationCanCancelOrCloseDocument();
private:
    KisPaintOpPresetSP m_preset;
};

void KisTransformToolDocumentTest::nativeAffineEditingKeepsPixelsHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("filter");
    QTest::addColumn<int>("operation");
    QTest::addColumn<bool>("generated");
    QTest::addColumn<int>("maskDefault");
    QTest::addColumn<bool>("maskOnly");
    QTest::addColumn<bool>("groupOwner");
    for (const bool words : {false, true})
        for (const auto filter : {"NearestNeighbor", "Bilinear", "Bicubic", "Hermite", "Bell", "BSpline", "Lanczos3", "Mitchell"})
            for (int operation = 0; operation < 5; ++operation)
                QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3").arg(words ? "word" : "byte").arg(filter).arg(operation))) << words << QString::fromLatin1(filter) << operation << false << 0 << false << false;
    for (const bool words : {false, true}) for (int operation : {6, 7, 8, 9})
        QTest::newRow(qPrintable(QStringLiteral("%1-Bicubic-%2").arg(words ? "word" : "byte").arg(operation))) << words << QStringLiteral("Bicubic") << operation << false << 0 << false << false;
    for (const bool words : {false, true}) for (int operation : {0, 1, 2, 3, 4, 6, 7, 8, 9})
        QTest::newRow(qPrintable(QStringLiteral("%1-Bicubic-%2-generated").arg(words ? "word" : "byte").arg(operation)))
            << words << QStringLiteral("Bicubic") << operation << true << 0 << false << false;

    for (const bool words : {false, true}) for (int maskDefault : {17, 255}) for (int operation : {0, 1, 2, 3, 4, 6, 7, 8, 9})
        QTest::newRow(qPrintable(QStringLiteral("%1-Bicubic-%2-default-%3")
            .arg(words ? "word" : "byte").arg(operation).arg(maskDefault)))
            << words << QStringLiteral("Bicubic") << operation << false << maskDefault << false << false;

    for (const bool words : {false, true}) for (int maskDefault : {0, 255}) for (int operation : {0, 2, 3, 4, 6, 7, 8, 9, 10, 11})
        QTest::newRow(qPrintable(QStringLiteral("%1-Bicubic-%2-default-%3-mask-only")
            .arg(words ? "word" : "byte").arg(operation).arg(maskDefault)))
            << words << QStringLiteral("Bicubic") << operation << false << maskDefault << true << false;

    for (const bool words : {false, true}) for (int maskDefault : {0, 255}) for (int operation : {0, 2, 4, 11})
        QTest::newRow(qPrintable(QStringLiteral("%1-Bicubic-%2-default-%3-group-mask")
            .arg(words ? "word" : "byte").arg(operation).arg(maskDefault)))
            << words << QStringLiteral("Bicubic") << operation << false << maskDefault << true << true;

}

void KisTransformToolDocumentTest::nativeAffineEditingKeepsPixelsHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words); QFETCH(QString, filter); QFETCH(int, operation);
    QFETCH(bool, generated);
    QFETCH(int, maskDefault); QFETCH(bool, maskOnly); QFETCH(bool, groupOwner);
    const int pixelIndex = groupOwner ? 1 : 0;
    const int selectionOperation = operation == 10 ? 6 : operation == 11 ? 4 : operation;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto restoreEngine = qScopeGuard([&] { if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine); });
    KisConfig(true).disableOpenGL();
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto pixelBytes = Gpu::pixelByteCount(format);
    const QRect sourceRect(-3, 5, 14, 11), frame(-192, -192, 448, 448);
    QByteArray source(sourceRect.width() * sourceRect.height() * pixelBytes, '\0');
    const quint16 alphasWord[]{0, 1, 127, 128, 32768, 65535}, alphasByte[]{0, 1, 127, 128, 254, 255};
    for (int y = 0; y < sourceRect.height(); ++y) for (int x = 0; x < sourceRect.width(); ++x) {
        const int maximum = words ? 65536 : 256;
        const Gpu::RgbaPixel color{quint16((4353 + x * 131 + y * 17) % maximum), quint16((33282 + x * 37 + y * 259) % maximum),
            quint16((61955 + x * 311 + y * 13) % maximum), (words ? alphasWord : alphasByte)[(x + y) % 6]};
        const auto bytes = Gpu::rgbaBytes(color, format);
        std::memcpy(source.data() + (y * sourceRect.width() + x) * pixelBytes, bytes.constData(), pixelBytes);
    }
    Gpu::RasterImage raster; raster.size = {64, 64}; raster.format = format; raster.colorProfile = cs->profile()->rawData();
    raster.layers = {{QStringLiteral("Affine target"), true,
        QUuid(QStringLiteral("{a7e9c641-3f28-4b16-81d5-672098bcfa34}")), {}}};
    raster.layers.first().position = {7, -3};
    QByteArray maskPixels(sourceRect.width() * sourceRect.height(), '\0');
    for (int i = 0; i < maskPixels.size(); ++i) maskPixels[i] = char((i * 43 + 1) % 256);
    // Both extremes lie inside the soft selection: black gray still carries
    // independent source opacity when it is transformed over a white mask.
    maskPixels[(8 - sourceRect.y()) * sourceRect.width() + 3 - sourceRect.x()] = 0;
    maskPixels[(8 - sourceRect.y()) * sourceRect.width() + 2 - sourceRect.x()] = char(255);
    raster.layers.first().transparencyMask = Gpu::SelectionMask::fromAlphaCoverage(sourceRect, maskPixels, sourceRect);
    if (maskDefault) {
        Gpu::SelectionTileMap tiles;
        for (int y = sourceRect.top(); y <= sourceRect.bottom(); ++y) for (int x = sourceRect.left(); x <= sourceRect.right(); ++x) {
            const int tx = x < 0 ? (x - 255) / 256 : x / 256;
            const int ty = y < 0 ? (y - 255) / 256 : y / 256;
            auto &tile = tiles[{tx, ty}];
            if (tile.isEmpty()) tile.fill(char(maskDefault), Gpu::SelectionMask::AlphaTileBytes);
            tile[(y - ty * 256) * 256 + x - tx * 256] = maskPixels[(y - sourceRect.y()) * sourceRect.width() + x - sourceRect.x()];
        }
        raster.layers.first().transparencyMask = Gpu::SelectionMask::fromAlphaTiles(tiles, quint8(maskDefault));
    }
    if (groupOwner) {
        auto child = raster.layers.first();
        child.transparencyMask.clear();
        auto &group = raster.layers.first();
        group.kind = Gpu::LayerKind::Group;
        group.name = QStringLiteral("Masked group");
        group.position = {};
        group.uuid = QUuid::createUuid();
        child.parentUuid = group.uuid;
        raster.layers.append(child);
    }
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    const auto layerId = owner->operations().snapshot()->layers[pixelIndex].id;
    const auto maskId = owner->operations().snapshot()->layers.first().id;
    QCOMPARE(owner->writeRgbaPixels(layerId, sourceRect, source), Gpu::Result::Success);
    if (generated) {
        const auto mask = generatedCoverage(owner, raster.layers.first().transparencyMask);
        QVERIFY2(mask && mask->isGpuGenerated(), qPrintable(owner->lastRequestOutcome().diagnostic));
        QCOMPARE(owner->replaceTransparencyMask(maskId, mask), Gpu::Result::Success);
    }
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Transform editing"), cs));
    auto image = KisImageSP(document->image());
    const auto maskParent = image->root()->firstChild(); QVERIFY(maskParent);
    KisNodeSP maskNode;
    for (auto node = maskParent->firstChild(); node; node = node->nextSibling())
        if (node->inherits("KisTransparencyMask")) maskNode = node;
    QVERIFY(maskNode);
    KisNodeSP paintNode = maskParent;
    if (groupOwner) {
        for (auto node = maskParent->firstChild(); node; node = node->nextSibling())
            if (node->uuid() == raster.layers[pixelIndex].uuid) paintNode = node;
        QVERIFY(paintNode != maskParent);
    }

    if (selectionOperation == 6) QVERIFY(image->setGpuSelectionRect({-1, 6, 9, 8}, false));
    else if (selectionOperation == 7) QVERIFY(image->setGpuSelectionEllipse({-1, 6, 9, 8}, false));
    else if (selectionOperation == 4 || selectionOperation == 8) {
        const QRect selected(-1, 6, 9, 8);
        QByteArray alpha(selected.width() * selected.height(), '\0');
        for (int i = 0; i < alpha.size(); ++i) alpha[i] = char((i * 71) % 256);
        QVERIFY(image->setGpuSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(selected, alpha, selected), selectionOperation == 8));
    }
    if (generated && image->globalSelection()) {
        const auto bounds = owner->selectionRect();
        const auto source = owner->selectionKind() == Gpu::SelectionKind::Freehand ? owner->selectionMask() :
            owner->selectionKind() == Gpu::SelectionKind::Ellipse ? Gpu::SelectionMask::fromEllipse(bounds, bounds) :
            Gpu::SelectionMask::fromRectangle(bounds, bounds);
        QVERIFY(source);
        const auto mask = generatedCoverage(owner, source);
        QVERIFY2(mask && mask->isGpuGenerated(), qPrintable(owner->lastRequestOutcome().diagnostic));
        QVERIFY(image->setGpuSelectionMask(mask, owner->selectionInverted()));
    }
    KisPaintDeviceSP reference = new KisPaintDevice(cs);
    reference->setDefaultBounds(new KisDefaultBounds(image)); reference->moveTo({7, -3});
    const auto bgra = exchangeRgbaBgra(source, pixelBytes);
    reference->writeBytes(reinterpret_cast<const quint8*>(bgra.constData()), sourceRect);
    // Ordinary transparency masks use KisPixelSelection's GrayA8 composition
    // source, which keeps mask gray separate from selection opacity.
    KisPaintDeviceSP maskReference = new KisPixelSelection();
    maskReference->setDefaultBounds(new KisSelectionDefaultBounds(reference));
    KoColor maskDefaultPixel(maskReference->colorSpace()); maskDefaultPixel.data()[0] = quint8(maskDefault);
    maskReference->setDefaultPixel(maskDefaultPixel);
    maskReference->writeBytes(reinterpret_cast<const quint8 *>(maskPixels.constData()), sourceRect);
    KisSelectionSP nativeSelection;
    QByteArray originalSelectionPixels;
    if (selectionOperation == 4 || (selectionOperation >= 6 && selectionOperation <= 8)) {
        nativeSelection = new KisSelection();
        const QRect selected(-1, 6, 9, 8);
        if (selectionOperation == 6) nativeSelection->pixelSelection()->select(selected);
        else {
            if (selectionOperation == 8) nativeSelection->pixelSelection()->select(image->bounds());
            const auto area = selectionOperation == 8 ? selected.intersected(image->bounds()) : selected;
            QByteArray values(area.width() * area.height(), '\0');
            for (int y = area.top(); y <= area.bottom(); ++y) for (int x = area.left(); x <= area.right(); ++x) {
                const auto dx = (x + .5 - (selected.x() + selected.width() / 2.)) / (selected.width() / 2.);
                const auto dy = (y + .5 - (selected.y() + selected.height() / 2.)) / (selected.height() / 2.);
                const auto value = selectionOperation == 7 ? (dx * dx + dy * dy <= 1 ? 255 : 0) :
                    (((y - selected.y()) * selected.width() + x - selected.x()) * 71) % 256;
                values[(y - area.y()) * area.width() + x - area.x()] = char(selectionOperation == 8 ? 255 - value : value);
            }
            nativeSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), area);
        }
        nativeSelection->updateProjection();
        QVERIFY(image->globalSelection());
        QByteArray expectedInput(frame.width() * frame.height(), '\0'), actualInput(expectedInput.size(), '\0');
        nativeSelection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(expectedInput.data()), frame);
        image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actualInput.data()), frame);
        QCOMPARE(actualInput, expectedInput);
        originalSelectionPixels = expectedInput;
    }
    const auto originalPixels = owner->readRgbaPixels(layerId, frame, frame.width() * frame.height() * pixelBytes).bytes;
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow()); window->resize(800, 600);
    auto *view = window->newView(document.get()); QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release()), continued;
    QPointer<KisTool> tool;
    const auto cleanup = qScopeGuard([&] {
        if (tool) tool->requestStrokeCancellation();
        if (continued) continued->setModified(false);
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show(); window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    QTRY_VERIFY(!windowReady.isEmpty());
    if (maskOnly) {
        const auto child = maskNode;
        auto *manager = window->viewManager()->nodeManager();
        manager->slotUiActivatedNode(child); manager->slotSetSelectedNodes({child});
        QCOMPARE(manager->activeNode(), child);
        QCOMPARE(manager->selectedNodes(), KisNodeList{child});
    }
    const auto toolId = QStringLiteral("KisToolTransform");
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = qobject_cast<KisTool*>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
    QVERIFY(owner->operations().hasActiveStroke());
    view->canvasBase()->snapGuide()->enableSnapping(false);
    lifetime->setModified(false);
    const int initialHistory = owner->operations().undoCount();
    QCOMPARE(lifetime->undoStack()->count(), initialHistory);
    QWidget *panel = nullptr;
    for (auto widget : tool->optionWidgets()) if (widget->metaObject()->indexOfSlot("slotFilterChanged(KoID)") >= 0) panel = widget.data();
    QVERIFY(panel);
    QVERIFY(QMetaObject::invokeMethod(panel, "slotFilterChanged", Q_ARG(KoID, KoID(filter))));
    if (operation == 0 || operation == 4 || (operation >= 6 && operation <= 9) || operation == 11) { QVERIFY(tool->setProperty("scaleX", 137.)); QVERIFY(tool->setProperty("scaleY", 63.)); }
    if (operation == 1) { QVERIFY(tool->setProperty("shearX", 19.)); QVERIFY(tool->setProperty("shearY", -11.)); QVERIFY(tool->setProperty("rotateZ", 1.97)); }
    if ((operation >= 6 && operation <= 9) || operation == 11) QVERIFY(tool->setProperty("rotateZ", .47));
    if (operation == 2) QVERIFY(tool->setProperty("rotateZ", M_PI / 2));
    if (operation == 3) { QVERIFY(tool->setProperty("scaleX", -100.)); QVERIFY(tool->setProperty("scaleY", -100.)); }
    QVERIFY(tool->setProperty("translateX", tool->property("translateX").toDouble() + 3.25));
    QVERIFY(tool->setProperty("translateY", tool->property("translateY").toDouble() - 2.75));
    QVERIFY(QMetaObject::invokeMethod(tool, "requestImageRecalculation"));
    ToolTransformArgs args;
    const auto nativeMaskBounds = maskReference->exactBounds();
    args.setFilterId(filter); args.setOriginalCenter(QRectF(nativeSelection ? nativeSelection->selectedExactRect() : maskOnly ? nativeMaskBounds : reference->exactBounds().united(nativeMaskBounds)).center());
    args.setTransformedCenter({tool->property("translateX").toDouble(), tool->property("translateY").toDouble()});
    args.setScaleX(tool->property("scaleX").toDouble()); args.setScaleY(tool->property("scaleY").toDouble());
    args.setShearX(tool->property("shearX").toDouble()); args.setShearY(tool->property("shearY").toDouble()); args.setAZ(tool->property("rotateZ").toDouble());
    KisPaintDeviceSP transformed = reference;
    if (nativeSelection) {
        transformed = reference->createCompositionSourceDevice();
        KisPainter painter(transformed); painter.setSelection(nativeSelection);
        painter.bitBlt(nativeSelection->selectedExactRect().topLeft(), reference, nativeSelection->selectedExactRect());
    }
    KisPaintDeviceSP transformedMask = maskReference->createCompositionSourceDevice(maskReference);
    if (nativeSelection) {
        transformedMask = maskReference->createCompositionSourceDevice();
        KisPainter painter(transformedMask); painter.setSelection(nativeSelection);
        painter.bitBlt(nativeSelection->selectedExactRect().topLeft(), maskReference, nativeSelection->selectedExactRect());
    }
    if (!maskOnly) { auto worker = KisTransformUtils::createTransformWorker(args, transformed, {}); QVERIFY(worker.run()); }
    auto maskWorker = KisTransformUtils::createTransformWorker(args, transformedMask, {}); QVERIFY(maskWorker.run());
    if (!nativeSelection) {
        maskReference->clear();
        KisPainter painter(maskReference); const auto area = transformedMask->extent(); painter.bitBlt(area.topLeft(), transformedMask, area);
    }
    if (nativeSelection) {
        maskReference->clearSelection(nativeSelection);
        KisPainter painter(maskReference); const auto area = transformedMask->extent(); painter.bitBlt(area.topLeft(), transformedMask, area);
    }
    QByteArray expectedMask(frame.width() * frame.height(), '\0');
    maskReference->readBytes(reinterpret_cast<quint8 *>(expectedMask.data()), frame);
    if (nativeSelection && !maskOnly) {
        reference->clearSelection(nativeSelection);
        KisPainter painter(reference); const auto area = transformed->extent(); painter.bitBlt(area.topLeft(), transformed, area);
    }
    QByteArray expectedSelection;
    QRect selectionFrame = frame;
    if (nativeSelection) {
        auto selectionWorker = KisTransformUtils::createTransformWorker(args, nativeSelection->pixelSelection(), {}); QVERIFY(selectionWorker.run());
        selectionFrame |= nativeSelection->selectedExactRect();
        expectedSelection = QByteArray(selectionFrame.width() * selectionFrame.height(), '\0');
        nativeSelection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(expectedSelection.data()), selectionFrame);
    }
    QByteArray expected(frame.width() * frame.height() * pixelBytes, '\0');
    reference->readBytes(reinterpret_cast<quint8*>(expected.data()), frame); expected = exchangeRgbaBgra(expected, pixelBytes);
    const auto before = owner->operations().snapshot();
    const auto preview = owner->operations().preview();
    const auto actual = execution->readRgbaPixels(owner->handle(), preview, frame, layerId, expected.size());
    QVERIFY2(actual.code == Gpu::ExecutionCode::Success, qPrintable(actual.diagnostic));
    const auto output = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!output.isEmpty()) {
        const auto path = QDir(output).filePath(QString::fromLatin1(QTest::currentDataTag())); QVERIFY(QDir().mkpath(path));
        for (const auto &entry : {qMakePair(QStringLiteral("actual.rgba"), actual.bytes), qMakePair(QStringLiteral("expected.rgba"), expected)}) {
            QFile file(QDir(path).filePath(entry.first)); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(entry.second), entry.second.size());
        }
    }
    QCOMPARE(actual.bytes, expected);
    const auto verifyMask = [&](const Gpu::SelectionMaskSP &mask) {
        return execution->readSelectionPixels(mask, frame, expectedMask.size()).bytes;
    };
    QVERIFY(preview->layers.first().transparencyMask);
    const auto actualMask = verifyMask(preview->layers.first().transparencyMask);
    if (actualMask != expectedMask && !output.isEmpty()) {
        const auto path = QDir(output).filePath(QString::fromLatin1(QTest::currentDataTag()));
        for (const auto &entry : {qMakePair(QStringLiteral("actual.mask"), actualMask), qMakePair(QStringLiteral("expected.mask"), expectedMask)}) {
            QFile file(QDir(path).filePath(entry.first)); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(entry.second), entry.second.size());
        }
        for (qsizetype i = 0; i < qMin(actualMask.size(), expectedMask.size()); ++i) if (actualMask[i] != expectedMask[i]) {
            qWarning() << "Mask mismatch" << QPoint(frame.x() + i % frame.width(), frame.y() + i / frame.width())
                       << "actual" << quint8(actualMask[i]) << "expected" << quint8(expectedMask[i]);
            break;
        }
    }
    QCOMPARE(actualMask, expectedMask);
    QCOMPARE(preview->layers.first().transparencyMask->defaultCoverage(), *maskReference->defaultPixel().data());
    QByteArray previewMask(expectedMask.size(), '\0');
    maskNode->paintDevice()->readBytes(reinterpret_cast<quint8 *>(previewMask.data()), frame);
    QCOMPARE(previewMask, expectedMask);
    if (nativeSelection) {
        QVERIFY(image->globalSelection());
        QByteArray actualSelection(expectedSelection.size(), '\0');
        QCOMPARE(image->globalSelection()->selectedExactRect(), nativeSelection->selectedExactRect());
        image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actualSelection.data()), selectionFrame);
        QCOMPARE(actualSelection, expectedSelection);
    }
    QCOMPARE(preview->layers[pixelIndex].position, reference->offset());
    QCOMPARE(paintNode->paintDevice()->exactBounds(), reference->exactBounds());
    QCOMPARE(paintNode->paintDevice()->extent(), reference->extent());
    QCOMPARE(owner->operations().snapshot(), before);
    if (operation == 9 || operation == 11) {
        tool->requestStrokeCancellation();
        QCOMPARE(owner->operations().preview(), before);
        QByteArray cancelledMask(expectedMask.size(), '\0');
        maskNode->paintDevice()->readBytes(reinterpret_cast<quint8 *>(cancelledMask.data()), frame);
        QCOMPARE(cancelledMask, verifyMask(raster.layers.first().transparencyMask));
        QCOMPARE(owner->readRgbaPixels(layerId, frame, originalPixels.size()).bytes, originalPixels);
        QCOMPARE(owner->operations().undoCount(), initialHistory);
        QCOMPARE(lifetime->undoStack()->count(), initialHistory);
        if (nativeSelection) {
            QByteArray restored(originalSelectionPixels.size(), '\0');
            QVERIFY(image->globalSelection());
            image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(restored.data()), frame);
            QCOMPARE(restored, originalSelectionPixels);
        }
        tool->requestStrokeCancellation();
        QVERIFY(!owner->operations().hasActiveStroke());
        return;
    }
    tool->requestStrokeEnd();
    QCOMPARE(lifetime->undoStack()->count(), initialHistory + 1);
    QCOMPARE(owner->operations().undoCount(), initialHistory + 1);
    QCOMPARE(owner->operations().historyAction(initialHistory + 1), Gpu::HistoryAction::TransformLayer);
    QCOMPARE(owner->readRgbaPixels(layerId, frame, expected.size()).bytes, expected);
    lifetime->undoStack()->undo(); QCOMPARE(owner->readRgbaPixels(layerId, frame, originalPixels.size()).bytes, originalPixels);
    lifetime->undoStack()->redo(); QCOMPARE(owner->readRgbaPixels(layerId, frame, expected.size()).bytes, expected);
    QTemporaryDir files; QVERIFY(files.isValid());
    const auto path = files.filePath(QStringLiteral("transformed.kra")); lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage())); lifetime->waitForSavingToComplete();
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    const auto next = reopened->image()->gpuImage(); QVERIFY(next);
    const auto reloaded = next->operations().snapshot();
    QCOMPARE(reloaded->format, format);
    const auto nextLayer = [&](const QUuid &uuid) -> const Gpu::Layer * {
        for (const auto &value : reloaded->layers) if (value.uuid == uuid) return &value;
        return nullptr;
    };
    const auto nextPixels = nextLayer(raster.layers[pixelIndex].uuid);
    const auto nextMask = nextLayer(raster.layers.first().uuid);
    QVERIFY(nextPixels && nextMask && nextMask->transparencyMask);
    QCOMPARE(nextPixels->kind, Gpu::LayerKind::Paint);
    QCOMPARE(nextPixels->parentUuid, raster.layers[pixelIndex].parentUuid);
    QCOMPARE(nextMask->kind, groupOwner ? Gpu::LayerKind::Group : Gpu::LayerKind::Paint);
    QCOMPARE(nextMask->transparencyMaskUuid, before->layers.first().transparencyMaskUuid);
    QCOMPARE(verifyMask(nextMask->transparencyMask), expectedMask);
    const auto nextPixelId = nextPixels->id, nextMaskId = nextMask->id;
    if (nativeSelection) {
        QVERIFY(reopened->image()->globalSelection());
        QCOMPARE(reopened->image()->globalSelection()->selectedExactRect(), nativeSelection->selectedExactRect());
        QByteArray savedSelection(expectedSelection.size(), '\0');
        reopened->image()->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(savedSelection.data()), selectionFrame);
        QCOMPARE(savedSelection, expectedSelection);
    }
    QCOMPARE(next->readRgbaPixels(nextPixelId, frame, expected.size()).bytes, expected);
    if (maskOnly) {
        const auto layer = nextMaskId;
        QCOMPARE(next->fillTransparencyMask(layer, {128, 128, 128}, .5), Gpu::Result::Success);
        const auto edited = verifyMask(next->layerTransparencyMask(layer));
        QVERIFY(edited != expectedMask);
        QCOMPARE(next->undo(), Gpu::Result::Success); QCOMPARE(verifyMask(next->layerTransparencyMask(layer)), expectedMask);
        QCOMPARE(next->redo(), Gpu::Result::Success); QCOMPARE(verifyMask(next->layerTransparencyMask(layer)), edited);
        QCOMPARE(next->readRgbaPixels(nextPixelId, frame, expected.size()).bytes, expected);
        return;
    }
    QCOMPARE(next->fillLayer(next->operations().activeLayer(), {0x12, 0x34, 0x56}, .5), Gpu::Result::Success);
    const auto edited = next->readRgbaPixels(nextPixelId, frame, expected.size()).bytes;
    QVERIFY(edited != expected);
    QCOMPARE(next->undo(), Gpu::Result::Success); QCOMPARE(next->readRgbaPixels(nextPixelId, frame, expected.size()).bytes, expected);
    QCOMPARE(next->redo(), Gpu::Result::Success); QCOMPARE(next->readRgbaPixels(nextPixelId, frame, expected.size()).bytes, edited);
}
void KisTransformToolDocumentTest::previewNotificationCanCancelOrCloseDocument_data()
{
    QTest::addColumn<bool>("words"); QTest::addColumn<bool>("close");
    for (bool words : {false, true}) for (bool close : {false, true})
        QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(words ? "word" : "byte").arg(close ? "close" : "cancel"))) << words << close;
}
void KisTransformToolDocumentTest::previewNotificationCanCancelOrCloseDocument()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words); QFETCH(bool, close);
    KisConfig(true).disableOpenGL();
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    QString diagnostic; const auto execution = KisPart::instance()->gpuExecution(diagnostic); QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster; raster.size = {64, 64}; raster.format = format; raster.colorProfile = cs->profile()->rawData();
    raster.layers = {{QStringLiteral("Transform lifetime"), true, {}, {}}};
    const auto owner = Gpu::PaintImageSP::create(execution, raster); const auto layer = owner->operations().activeLayer();
    const auto pixel = Gpu::rgbaBytes(words ? Gpu::RgbaPixel{1, 257, 65534, 65535} : Gpu::RgbaPixel{17, 34, 51, 255}, format);
    QCOMPARE(owner->writeRgbaPixels(layer, {16, 16, 8, 8}, pixel.repeated(64)), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Transform lifetime"), cs));
    QSignalSpy ready(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get()); QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto cleanup = qScopeGuard([&] { if (lifetime) lifetime->setModified(false); window.reset(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); });
    window->viewManager()->setCurrentView(view); window->show();
    window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    QTRY_VERIFY(!ready.isEmpty());
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolTransform"));
    QPointer<KisTool> tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KisToolTransform"))); QVERIFY(tool);
    QVERIFY(owner->operations().hasActiveStroke());
    lifetime->undoStack()->clear(); lifetime->setModified(false);
    const auto before = owner->operations().snapshot();
    bool interrupted = false;
    connect(owner.data(), &Gpu::PaintImage::changed, tool.data(), [&] {
        if (interrupted) return;
        interrupted = true;
        if (close) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        } else tool->requestStrokeCancellation();
    });
    tool->setProperty("scaleX", 150.);
    QVERIFY(interrupted);
    if (close) QTRY_VERIFY(!tool);
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->operations().undoCount(), 0);
    if (!close) QCOMPARE(owner->readRgbaPixels(layer, {16, 16, 8, 8}, pixel.size() * 64).bytes, pixel.repeated(64));
}

void KisTransformToolDocumentTest::internalMasksMoveAndTransformThroughHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("adjustment");
    QTest::addColumn<bool>("selected");
    QTest::addColumn<int>("maskDefault");
    for (const bool words : {false, true}) for (const bool adjustment : {false, true})
        for (const bool selected : {false, true}) for (const int maskDefault : {0, 255})
            QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3-default%4").arg(words ? "word" : "byte")
                .arg(adjustment ? "adjustment" : "filter").arg(selected ? "selected" : "whole").arg(maskDefault)))
                << words << adjustment << selected << maskDefault;
}

void KisTransformToolDocumentTest::internalMasksMoveAndTransformThroughHistoryAndKra()
{
    QFETCH(bool, words); QFETCH(bool, adjustment); QFETCH(bool, selected); QFETCH(int, maskDefault);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    KisConfig(true).disableOpenGL();
    QTemporaryDir files; QVERIFY(files.isValid());
    std::unique_ptr<KisDocument> source(KisPart::instance()->createDocument());
    source->setFileBatchMode(true);
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP native = new KisImage(source->createUndoStore(), 64, 48, cs, "Mask tools");
    source->setCurrentImage(native);
    KisPaintLayerSP paint = new KisPaintLayer(native, "Source", 255); native->addNode(paint, native->root());
    const KoColor color(QColor(61, 137, 219, 197), cs);
    paint->paintDevice()->fill(0, 0, 64, 48, color.data());
    const auto filter = KisFilterRegistry::instance()->value("maximize"); QVERIFY(filter);
    const auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance())->cloneWithResourcesSnapshot();
    KisSelectionSP coverage = new KisSelection;
    coverage->pixelSelection()->moveTo(-65, 73);
    const quint8 defaultValue = quint8(maskDefault);
    coverage->pixelSelection()->setDefaultPixel(KoColor(&defaultValue, coverage->pixelSelection()->colorSpace()));
    QByteArray values(17 * 13, Qt::Uninitialized);
    for (int i = 0; i < values.size(); ++i) values[i] = char((i * 43 + 1) % 256);
    coverage->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), 8, 6, 17, 13);
    KisNodeSP target;
    if (adjustment) {
        target = new KisAdjustmentLayer(native, "Adjustment", config, coverage);
        native->addNode(target, native->root(), paint);
    } else {
        KisFilterMaskSP mask = new KisFilterMask(native, "Filter");
        mask->initSelection(coverage, paint); mask->setFilter(config); native->addNode(mask, paint); target = mask;
    }
    KisTransparencyMaskSP child;
    if (adjustment) {
        child = new KisTransparencyMask(native, "Independent child");
        KisSelectionSP childCoverage = new KisSelection;
        const quint8 white = 255;
        childCoverage->pixelSelection()->setDefaultPixel(KoColor(&white, childCoverage->pixelSelection()->colorSpace()));
        childCoverage->pixelSelection()->moveTo(77, -69);
        childCoverage->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), 24, 20, 17, 13);
        child->setSelection(childCoverage); native->addNode(child, target);
    }
    native->refreshGraphAsync(); native->waitForDone();
    const auto input = files.filePath("native-mask-tools.kra");
    QVERIFY(source->saveAs(input, KisDocument::nativeFormatMimeType(), false)); source->waitForSavingToComplete();
    const QUuid targetUuid = target->uuid(), childUuid = child ? child->uuid() : QUuid{}, paintUuid = paint->uuid();
    const QRect frame(-128, -128, 320, 320);
    const auto find = [](KisImageSP image, const QUuid &uuid) -> KisNodeSP {
        KisNodeList nodes{image->root()};
        for (qsizetype i = 0; i < nodes.size(); ++i) {
            const auto node = nodes[i]; if (node->uuid() == uuid) return node;
            for (auto child = node->firstChild(); child; child = child->nextSibling()) nodes.append(child);
        }
        return {};
    };
    QList<QList<QByteArray>> expected;
    QList<QList<QPoint>> expectedPositions;
    for (const bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument()); document->setFileBatchMode(true);
        QVERIFY2(document->openPath(input, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
        KisImageSP image = document->image();
        const auto node = find(image, targetUuid); QVERIFY(node);
        const auto snapshot = [&](KisImageSP view) {
            QList<QByteArray> output;
            for (const auto &uuid : {targetUuid, childUuid, paintUuid}) {
                const auto current = find(view, uuid);
                if (!current) { output.append(QByteArray{}); continue; }
                QByteArray bytes(frame.width() * frame.height() * current->paintDevice()->pixelSize(), Qt::Uninitialized);
                current->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), frame);
                output.append(bytes);
            }
            QByteArray projection(64 * 48 * cs->pixelSize(), Qt::Uninitialized);
            view->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), 0, 0, 64, 48);
            output.append(projection); return output;
        };
        const auto positions = [&](KisImageSP view) {
            QList<QPoint> result;
            for (const auto &uuid : {targetUuid, childUuid}) {
                const auto current = find(view, uuid);
                result.append(current ? current->paintDevice()->offset() : QPoint{});
            }
            return result;
        };
        int stage = 0;
        const auto compare = [&](KisImageSP view) {
            const auto bytes = snapshot(view); const auto coordinates = positions(view);
            const int index = stage++;
            if (!gpu) { expected.append(bytes); expectedPositions.append(coordinates); }
            else { QCOMPARE(bytes, expected.at(index)); QCOMPARE(coordinates, expectedPositions.at(index)); }
        };
        QSignalSpy ready(KisPart::instance(), &KisPart::sigMainWindowCreated);
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow()); window->resize(800, 600);
        auto *view = window->newView(document.get()); QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release()); QPointer<KisTool> tool;
        const auto cleanup = qScopeGuard([&] {
            if (tool) tool->requestStrokeCancellation();
            if (lifetime) lifetime->setModified(false);
            window.reset(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        window->viewManager()->setCurrentView(view); window->show();
        window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset); QTRY_VERIFY(!ready.isEmpty());
        auto *manager = window->viewManager()->nodeManager();
        manager->slotUiActivatedNode(node); manager->slotSetSelectedNodes({node});
        compare(image);
        const int initialHistory = lifetime->undoStack()->count();
        const auto moveId = QStringLiteral("KritaTransform/KisToolMove");
        KoToolManager::instance()->switchToolRequested(moveId);
        tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), moveId)); QVERIFY(tool);
        QVERIFY(QMetaObject::invokeMethod(tool, "slotMoveDiscreteRight"));
        tool->requestStrokeEnd(); image->waitForDone(); QApplication::processEvents();
        QCOMPARE(lifetime->undoStack()->count(), initialHistory + 1); compare(image);
        lifetime->undoStack()->undo(); image->waitForDone(); compare(image);
        lifetime->undoStack()->redo(); image->waitForDone(); compare(image);
        if (selected) {
            KisSelectionSP selection = new KisSelection;
            selection->pixelSelection()->select(QRect(7, 5, 28, 25));
            if (gpu) QVERIFY(image->setGpuSelectionRect(QRect(7, 5, 28, 25)));
            else image->undoAdapter()->addCommand(new KisSetGlobalSelectionCommand(image, selection));
        }
        const auto beforeTransform = snapshot(image); const auto coordinates = positions(image);
        const int beforeHistory = lifetime->undoStack()->count();
        const auto transformId = QStringLiteral("KisToolTransform");
        KoToolManager::instance()->switchToolRequested(transformId);
        tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), transformId)); QVERIFY(tool);
        // waitForDone finalizes active input; the generated transaction's
        // center reports transform readiness without ending the gesture.
        QApplication::processEvents();
        QTRY_VERIFY(tool->property("translateX").toDouble() != 0 || tool->property("translateY").toDouble() != 0);
        QWidget *panel = nullptr;
        for (auto widget : tool->optionWidgets()) if (widget->metaObject()->indexOfSlot("slotFilterChanged(KoID)") >= 0) panel = widget.data();
        QVERIFY(panel); QVERIFY(QMetaObject::invokeMethod(panel, "slotFilterChanged", Q_ARG(KoID, KoID("Bicubic"))));
        QVERIFY(tool->setProperty("scaleX", 137.)); QVERIFY(tool->setProperty("scaleY", 63.));
        QVERIFY(tool->setProperty("rotateZ", .47));
        QVERIFY(QMetaObject::invokeMethod(tool, "requestImageRecalculation"));
        tool->requestStrokeCancellation(); image->waitForDone();
        QCOMPARE(snapshot(image), beforeTransform); QCOMPARE(positions(image), coordinates);
        QCOMPARE(lifetime->undoStack()->count(), beforeHistory);
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        KoToolManager::instance()->switchToolRequested(transformId);
        tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), transformId)); QVERIFY(tool);
        QApplication::processEvents();
        QTRY_VERIFY(tool->property("translateX").toDouble() != 0 || tool->property("translateY").toDouble() != 0);
        QVERIFY(tool->setProperty("scaleX", 137.)); QVERIFY(tool->setProperty("scaleY", 63.));
        QVERIFY(tool->setProperty("rotateZ", .47));
        QCOMPARE(tool->property("scaleX").toDouble(), 1.37);
        QCOMPARE(tool->property("scaleY").toDouble(), .63);
        QVERIFY(QMetaObject::invokeMethod(tool, "requestImageRecalculation"));
        tool->requestStrokeEnd(); image->waitForDone(); QApplication::processEvents();
        QVERIFY(snapshot(image) != beforeTransform);
        QCOMPARE(lifetime->undoStack()->count(), beforeHistory + 1); compare(image);
        lifetime->undoStack()->undo(); image->waitForDone(); compare(image);
        lifetime->undoStack()->redo(); image->waitForDone(); compare(image);
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush")); tool.clear();
        const auto path = files.filePath(gpu ? "gpu-transformed.kra" : "cpu-transformed.kra");
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        compare(reopened->image());
        const auto continued = find(reopened->image(), targetUuid); QVERIFY(continued);
        continued->paintDevice()->moveTo(79, 89); reopened->image()->refreshGraphAsync(); reopened->image()->waitForDone();
        compare(reopened->image());
        QCOMPARE(stage, expected.size());
    }
}

KISTEST_MAIN(KisTransformToolDocumentTest)
#include "KisTransformToolDocumentTest.moc"
