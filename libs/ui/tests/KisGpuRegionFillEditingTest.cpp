/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuRegionFillEditingTest.h"
#include <KisGpuPixelTolerance.h>
#include <QApplication>
#include <QBitArray>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QImage>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPointer>
#include <QScopeGuard>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KisAngleSelector.h>
#include <KisFigurePaintingOptions.h>
#include <KisGpuBlendMode.h>
#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoPattern.h>
#include <KoPointerEvent.h>
#include <KoSnapGuide.h>
#include <KoToolManager.h>
#include <application/kis_config.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <brushengine/kis_paintop_preset.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_controller.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_zoom_manager.h>
#include <commands_new/KisMergeLabeledLayersCommand.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kra/KisKraRasterCodec.h>
#include <nodes/kis_node_manager.h>
#include <kis_default_bounds.h>
#include <kis_figure_painting_stroke.h>
#include <kis_fill_painter.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_layer_utils.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_selection_filters.h>
#include <kis_slider_spin_box.h>
#include <kis_tool.h>
#include <kis_undo_stores.h>
#include <algorithm>
#include <cstring>
#include <memory>

namespace {
void exchangeRgbaBgra(QByteArray &bytes, int pixelBytes)
{
    const int channelBytes = pixelBytes / 4;
    for (qsizetype pixel = 0; pixel < bytes.size(); pixel += pixelBytes)
        for (int byte = 0; byte < channelBytes; ++byte)
            std::swap(bytes[pixel + byte], bytes[pixel + 2 * channelBytes + byte]);
}
}

void RegionFillEditingTest::addData()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("scenario");
    QTest::newRow("byte-foreground") << false << QStringLiteral("foreground");
    QTest::newRow("word-similar-colour-feather-soft") << true << QStringLiteral("similar-colour-feather-soft");
}

void RegionFillEditingTest::run(const KisPaintOpPresetSP &preset)
{
    QFETCH(bool, words);
    QFETCH(QString, scenario);
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const bool similar = scenario.startsWith(QLatin1String("similar"));
    const bool colourRegion = scenario.contains(QLatin1String("colour"));
    const bool labeled = scenario.contains(QLatin1String("labeled"));
    const bool referenceGroup = labeled && scenario.contains(QLatin1String("group"));
    const bool selectedGroup = scenario.contains(QLatin1String("group-selected")) || scenario.contains(QLatin1String("group-pass"));
    const QList<int> colorLabels = scenario.endsWith(QLatin1String("empty")) ? QList<int>{} :
        scenario.endsWith(QLatin1String("nomatch")) ? QList<int>{8} :
        scenario.endsWith(QLatin1String("unlabeled")) ? QList<int>{0} : selectedGroup ? QList<int>{1} : QList<int>{2};
    const bool includeActive = scenario.endsWith(QLatin1String("active"));
    const bool merged = scenario.contains(QLatin1String("merged"));
    const bool boundary = scenario.contains(QLatin1String("boundary"));
    const bool antiAlias = scenario.contains(QLatin1String("-aa"));
    const bool edgeCanvas = scenario.contains(QLatin1String("-edge-"));
    const bool grow = scenario.contains(QLatin1String("-grow"));
    const bool shrink = scenario.contains(QLatin1String("-shrink"));
    const int radius = grow || shrink ? (scenario.endsWith(QLatin1String("limit")) ? 400 : scenario.endsWith(QLatin1String("wide")) ? 40 : scenario.contains(QLatin1String("soft")) ? 7 : 3) : 0;
    const int sizeMod = shrink ? -radius : radius;
    const int feather = scenario.contains(QLatin1String("feather")) ?
        (scenario.endsWith(QLatin1String("small")) ? 1 : scenario.contains(QLatin1String("soft")) ? 7 : 3) : 0;
    const int margin = (qMax(0, sizeMod) + 63) / 64 * 64 + (feather + 63) / 64 * 64;
    const bool softRegion = colourRegion && scenario.contains(QLatin1String("soft"));
    const bool excludedSeed = scenario.contains(QLatin1String("excluded-seed"));
    const bool contiguous = scenario.startsWith(QLatin1String("contiguous"));
    const bool defaultThreshold = scenario == QLatin1String("contiguous-default");
    const bool opaqueRegion = scenario == QLatin1String("contiguous-opaque") || colourRegion;
    const int threshold = colourRegion ? 23 : defaultThreshold ? 8 : 1;
    const int spread = softRegion ? 35 : 100;
    const bool offcanvas = scenario.startsWith(QLatin1String("offcanvas"));
    const QRect region(offcanvas ? QPoint(-12, -9) : QPoint(), edgeCanvas ? QSize(103, 79) : QSize(64, 64));
    const QRect inspected = radius || feather ? region.adjusted(-margin, -margin, margin, margin) : edgeCanvas ? QRect(-2, -2, 136, 136) : region;
    const int stride = cs->pixelSize();
    const bool api = scenario.startsWith(QLatin1String("api"));
    const bool figure = api || scenario.startsWith(QLatin1String("rectangle")) || scenario.startsWith(QLatin1String("ellipse"));
    const bool unselected = api || scenario.startsWith(QLatin1String("unselected")) || ((contiguous || similar) && !scenario.contains(QLatin1String("masked")) && !excludedSeed);
    const bool pattern = scenario.contains(QLatin1String("pattern"));
    const bool background = scenario.contains(QLatin1String("background"));
    const bool custom = background || scenario.endsWith(QLatin1String("copy"));
    const bool action = scenario.startsWith(QLatin1String("action"));
    const bool alphaLock = scenario.contains(QLatin1String("alpha-lock"));
    const bool protect = scenario.contains(QLatin1String("protected"));
    const bool erase = scenario.endsWith(QLatin1String("erase"));
    const bool softlight = scenario.contains(QLatin1String("softlight"));
    const bool alternate = scenario == QLatin1String("alt-selection");
    const auto composite = custom ? QStringLiteral("copy") : softlight ? QStringLiteral("soft_light") :
        erase ? QStringLiteral("erase") : QStringLiteral("normal");
    const qreal opacity = custom ? .63 : .37;
    const qreal scale = scenario == QLatin1String("pattern-small-scale") ? .01 : scenario == QLatin1String("pattern-zero-scale") ? 0 : scenario.contains(QLatin1String("rotated")) || protect || figure ? 137.5 : 100;
    const qreal rotation = scenario.contains(QLatin1String("rotated")) || protect || figure ? 27 : 0;
    KConfigGroup configuration = KSharedConfig::openConfig()->group(QStringLiteral("KritaFill/KisToolFill"));
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfig = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("whatToFill", similar ? "fillSimilarRegions" : alternate || contiguous ? "fillContiguousRegion" : "fillSelection");
    if (contiguous || similar) {
        configuration.writeEntry("thresholdAmount", threshold);
        configuration.writeEntry("opacitySpread", spread);
        configuration.writeEntry("antiAlias", antiAlias);
        configuration.writeEntry("closeGapAmount", 0);
        configuration.writeEntry("growSelection", sizeMod);
        configuration.writeEntry("stopGrowingAtDarkestPixel", false);
        configuration.writeEntry("featherAmount", feather);
        configuration.writeEntry("sampleLayersMode", labeled ? "colorLabeledLayers" : merged ? "allLayers" : "currentLayer");
        if (labeled) {
            QStringList labels;
            for (int label : colorLabels) labels.append(QString::number(label));
            configuration.writeEntry("colorLabels", labels.join(','));
            configuration.writeEntry("useActiveLayer", includeActive);
        }
        configuration.writeEntry("contiguousFillMode", boundary ? "boundaryFill" : "floodFill");
        if (boundary) {
            const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
            auto bytes = Gpu::rgbaBytes({Gpu::maximumComponent(format), 177, quint16(Gpu::maximumComponent(format) / 3), Gpu::maximumComponent(format)}, format);
            exchangeRgbaBgra(bytes, stride);
            configuration.writeEntry("contiguousFillBoundaryColor", KoColor(reinterpret_cast<const quint8 *>(bytes.constData()), cs).toXML());
        }
        configuration.writeEntry("useSelectionAsBoundary", !similar);
    }
    configuration.writeEntry("fillWith", pattern ? "pattern" : background ? "backgroundColor" : "foregroundColor");
    configuration.writeEntry("patternScale", scale);
    configuration.writeEntry("patternRotate", rotation);
    configuration.writeEntry("useCustomBlendingOptions", custom);
    configuration.writeEntry("customOpacity", 63);
    configuration.writeEntry("customCompositeOp", "copy");
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const auto oldDisplay = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const bool hadDisplay = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto restoreDisplay = qScopeGuard([&] {
        if (hadDisplay) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), oldDisplay);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = region.size(); raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.layers = {{QStringLiteral("Fill layer"), true, {}, {}}};
    if (merged) raster.layers.prepend({QStringLiteral("Reference layer"), true, {}, {}});
    if (labeled) {
        Gpu::RasterLayer reference{QStringLiteral("Labelled reference"), true, QUuid::createUuid(), {}};
        reference.colorLabelIndex = 2;
        reference.visible = !scenario.endsWith(QLatin1String("hidden")) || referenceGroup;
        reference.opacity = 197;
        reference.blendMode = Gpu::LayerBlendMode::Multiply;
        reference.projectionChannelMask = 9;
        Gpu::RasterLayer decoy{QStringLiteral("Other label"), true, QUuid::createUuid(), {}};
        decoy.colorLabelIndex = 3;
        raster.layers.prepend(decoy);
        raster.layers.prepend(reference);
        if (referenceGroup) {
            Gpu::RasterLayer group{QStringLiteral("Reference group"), true, QUuid::createUuid(), {}};
            group.kind = Gpu::LayerKind::Group;
            group.colorLabelIndex = selectedGroup ? 1 : 0;
            group.opacity = 113;
            group.blendMode = Gpu::LayerBlendMode::Multiply;
            group.projectionChannelMask = 10;
            group.passThrough = scenario.endsWith(QLatin1String("pass"));
            group.visible = !scenario.endsWith(QLatin1String("hidden"));
            raster.layers[0].parentUuid = group.uuid;
            raster.layers.prepend(group);
            if (scenario.endsWith(QLatin1String("group-order"))) {
                Gpu::RasterLayer second = group;
                second.uuid = QUuid::createUuid();
                second.name = QStringLiteral("Second reference group");
                raster.layers[2].colorLabelIndex = 2;
                raster.layers[2].parentUuid = second.uuid;
                raster.layers.insert(1, second);
                std::swap(raster.layers[2], raster.layers[3]);
            }
        }
    }
    raster.colorProfile = cs->profile()->rawData();
    auto owner = Gpu::PaintImageSP::create(execution, raster);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Ordinary selection fill"), cs));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
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
    for (auto *dock : window->findChildren<QDockWidget *>()) dock->hide();
    window->resize(800, 600);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->canvasBase()->snapGuide()->enableSnapping(false);
    view->zoomManager()->zoomTo100();
    view->canvasController()->setPreferredCenter({32, 32});
    QTRY_VERIFY(view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().width() >= 64);
    auto image = KisImageSP(lifetime->image());
    KisNodeSP node = KisLayerUtils::findNodeByUuid(image->root(), owner->operations().snapshot()->layers.last().uuid);
    QVERIFY(node);
    const auto fillUuid = node->uuid();
    window->viewManager()->nodeManager()->slotUiActivatedNode(node);
    QByteArray initial(region.width() * region.height() * stride, '\0');
    for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x) {
        Gpu::RgbaPixel pixel = words ? Gpu::RgbaPixel{quint16(119 + x * 813), quint16(931 + y * 577), 39113,
            quint16((x + y) % 7 == 0 ? 0 : (x + y) % 5 == 0 ? 1 : 41513)} :
            Gpu::RgbaPixel{quint16(1 + x * 3), quint16(5 + y * 2), 183, quint16((x + y) % 7 == 0 ? 0 : 161)};
        if (contiguous || similar) {
            const bool interior = edgeCanvas ? x >= 7 && y >= 7 && x + y < 164 :
                y >= 4 && y < 60 && ((x >= 4 && x < 30) || (x >= 34 && x < 60));
            const quint16 maximum = Gpu::maximumComponent(raster.format);
            pixel = {quint16(maximum / 7), 177, quint16(maximum / 3), quint16(interior && !opaqueRegion ? 0 : maximum)};
            if (colourRegion && interior) {
                pixel[0] += quint16((x % 13) * (words ? 317 : 2));
                pixel[1] += quint16((y % 11) * (words ? 411 : 3));
                pixel[2] += quint16(((x + y) % 7) * (words ? 659 : 2));
            }
            if (boundary && softRegion && interior && x < 12) {
                pixel[0] = quint16(maximum - (12 - x) * (words ? 2501 : 11));
                pixel[1] = quint16(177 + (x - 4) * (words ? 213 : 3));
            }
            if (!interior) pixel[0] = maximum;
        }
        const auto bytes = Gpu::rgbaBytes(pixel, raster.format);
        std::memcpy(initial.data() + (y * region.width() + x) * stride, bytes.constData(), stride);
    }
    auto bgra = initial;
    exchangeRgbaBgra(bgra, stride);
    KisPaintDeviceSP referenceLayer;
    QHash<QUuid, KisPaintDeviceSP> labeledPixels;
    if (merged) {
        const auto referenceNode = KisLayerUtils::findNodeByUuid(image->root(), owner->operations().snapshot()->layers.first().uuid);
        QVERIFY(referenceNode && referenceNode != node);
        referenceNode->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
        referenceLayer = new KisPaintDevice(cs);
        referenceLayer->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
        initial.fill('\0'); bgra.fill('\0');
    }
    if (labeled) {
        for (const auto &candidate : owner->operations().snapshot()->layers) {
            if (candidate.kind != Gpu::LayerKind::Paint || candidate.uuid == fillUuid) continue;
            auto pixels = bgra;
            if (candidate.name == QLatin1String("Other label")) {
                pixels.clear();
                const auto blue = Gpu::rgbaBytes({0, 0, Gpu::maximumComponent(raster.format), Gpu::maximumComponent(raster.format)}, raster.format);
                for (int i = 0; i < region.width() * region.height(); ++i) pixels += blue;
                exchangeRgbaBgra(pixels, stride);
            }
            const auto referenceNode = KisLayerUtils::findNodeByUuid(image->root(), candidate.uuid);
            QVERIFY(referenceNode && referenceNode->paintDevice());
            referenceNode->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), region);
            auto source = KisPaintDeviceSP(new KisPaintDevice(cs));
            source->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), region);
            labeledPixels.insert(candidate.uuid, source);
        }
        initial.fill('\0'); bgra.fill('\0');
    }
    node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
    KisImageSP nativeReferenceBounds;
    if (feather) nativeReferenceBounds = new KisImage(nullptr, region.width(), region.height(), cs, QStringLiteral("Native fill bounds"));
    KisPaintDeviceSP expectedDevice = new KisPaintDevice(cs);
    if (nativeReferenceBounds) expectedDevice->setDefaultBounds(new KisDefaultBounds(nativeReferenceBounds));
    if (edgeCanvas) {
        const auto maximum = Gpu::maximumComponent(raster.format);
        const auto old = Gpu::rgbaBytes({quint16(maximum / 2), quint16(maximum / 4), quint16(maximum / 8), quint16(maximum / 2)}, raster.format);
        QByteArray outside;
        for (int i = 0; i < inspected.width() * inspected.height(); ++i) outside += old;
        exchangeRgbaBgra(outside, stride);
        node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(outside.constData()), inspected);
        expectedDevice->writeBytes(reinterpret_cast<const quint8 *>(outside.constData()), inspected);
        node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
    }
    expectedDevice->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
    QByteArray alpha(region.width() * region.height(), '\0');
    for (int y = 4; y < 60; ++y) for (int x = 3; x < 61; ++x)
        alpha[y * region.width() + x] = char((x * 47 + y * 13) % 256);
    if (excludedSeed) alpha[24 * region.width() + 24] = 0;
    const auto mask = Gpu::SelectionMask::fromAlphaCoverage(region, alpha, region);
    QVERIFY(mask);
    KisSelectionSP selection;
    if (!unselected) {
        QVERIFY(image->setGpuSelectionMask(mask));
        selection = new KisSelection;
        selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), region);
    }
    QBitArray flags(4, true);
    if (protect) { flags.setBit(0, false); flags.setBit(2, false); }
    qobject_cast<KisPaintLayer *>(node.data())->setChannelLockFlags(flags);
    if (alphaLock) flags.setBit(3, false);
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset);
    resources->setFGColor(KoColor(QColor(17, 129, 241, 19), cs));
    resources->setBGColor(KoColor(QColor(221, 17, 44, 80), cs));
    resources->setOpacity(custom ? .11 : opacity);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setGlobalAlphaLock(alphaLock);
    resources->setEraserMode(erase || custom);
    resources->setCurrentCompositeOp(custom ? QStringLiteral("normal") : composite);
    QImage patternImage(5, 3, QImage::Format_ARGB32);
    for (int y = 0; y < patternImage.height(); ++y) for (int x = 0; x < patternImage.width(); ++x)
        patternImage.setPixelColor(x, y, QColor(11 + x * 59, 27 + y * 83, 197 - x * 31, x == 0 ? 0 : 21 + y * 83));
    const KoPatternSP patternResource(new KoPattern(patternImage, QStringLiteral("Native pattern"), QString()));
    resources->slotPatternActivated(patternResource);
    QTransform transform;
    transform.rotate(rotation); transform.scale(scale * .01, scale * .01);
    int regionEdits = 0;
    const auto regionSeed = [&] { return defaultThreshold && regionEdits > 0 ? QPoint(40, 24) : QPoint(24, 24); };
    bool emptyNativeRegion = false;
    const auto applyExpected = [&](bool smooth) {
        emptyNativeRegion = false;
        KisPaintDeviceSP reference = expectedDevice;
        if (labeled) {
            // The real native labelled-layer command owns root selection,
            // visibility, detached opacity and selected-group composition.
            KisImageSP source = new KisImage(new KisSurrogateUndoStore(), region.width(), region.height(), cs, QStringLiteral("Native labelled reference"));
            QHash<QUuid, KisNodeSP> nodes;
            KisNodeSP active;
            const auto version = owner->operations().snapshot();
            for (const auto &layer : version->layers) {
                KisLayerSP copy;
                if (layer.kind == Gpu::LayerKind::Group) {
                    auto group = KisGroupLayerSP(new KisGroupLayer(source, layer.name, layer.opacity));
                    group->setPassThroughMode(layer.passThrough);
                    copy = group;
                } else {
                    const auto pixels = layer.uuid == fillUuid ? expectedDevice : labeledPixels.value(layer.uuid);
                    copy = new KisPaintLayer(source, layer.name, layer.opacity, new KisPaintDevice(*pixels));
                }
                copy->setVisible(layer.visible);
                copy->setColorLabelIndex(layer.colorLabelIndex);
                copy->setCompositeOpId(Gpu::compositeOpFromLayerBlendMode(layer.blendMode));
                QBitArray channels(4, false);
                for (int c = 0; c < 4; ++c) channels.setBit(c == 0 ? 2 : c == 2 ? 0 : c, layer.projectionChannelMask & (1 << c));
                copy->setChannelFlags(channels);
                nodes.insert(layer.uuid, copy);
                if (layer.uuid == fillUuid) active = copy;
            }
            for (const auto &layer : version->layers)
                source->addNode(nodes.value(layer.uuid), layer.parentUuid.isNull() ? source->root() : nodes.value(layer.parentUuid));
            reference = KisMergeLabeledLayersCommand::createRefPaintDevice(source, QStringLiteral("Native labelled pixels"));
            auto previous = KisMergeLabeledLayersCommand::createRefPaintDevice(source, QStringLiteral("Previous labelled pixels"));
            KisMergeLabeledLayersCommand command(source,
                KisMergeLabeledLayersCommand::ReferenceNodeInfoListSP::create(),
                KisMergeLabeledLayersCommand::ReferenceNodeInfoListSP::create(), previous, reference, colorLabels,
                KisMergeLabeledLayersCommand::GroupSelectionPolicy_SelectIfColorLabeled, true, includeActive ? active : KisNodeSP());
            command.redo();
        }
        if (merged) {
            reference = new KisPaintDevice(*referenceLayer);
            KisPainter composite(reference);
            composite.bitBlt(region.topLeft(), expectedDevice, region);
        }
        if (similar) {
            KisSelectionSP selected = new KisSelection;
            if (nativeReferenceBounds) selected->setDefaultBounds(new KisDefaultBounds(nativeReferenceBounds));
            KisFillPainter classifier;
            classifier.setFillThreshold(threshold); classifier.setOpacitySpread(spread);
            classifier.createSimilarColorsSelection(selected->pixelSelection(), reference->pixel(regionSeed()), reference,
                region, selection ? selection->pixelSelection() : KisPixelSelectionSP());
            if (sizeMod > 0) {
                KisGrowSelectionFilter filter(sizeMod, sizeMod);
                filter.process(selected->pixelSelection(), selected->pixelSelection()->selectedRect().adjusted(-sizeMod, -sizeMod, sizeMod, sizeMod));
            } else if (sizeMod < 0) {
                KisShrinkSelectionFilter filter(-sizeMod, -sizeMod, false);
                filter.process(selected->pixelSelection(), selected->pixelSelection()->selectedRect());
            }
            if (feather) {
                KisFeatherSelectionFilter filter(feather);
                filter.process(selected->pixelSelection(), selected->pixelSelection()->selectedRect().adjusted(-feather, -feather, feather, feather));
            } else if (smooth) {
                KisAntiAliasSelectionFilter filter;
                filter.process(selected->pixelSelection(), selected->pixelSelection()->selectedRect());
            }
            if (selection) selected->pixelSelection()->applySelection(selection->pixelSelection(), SELECTION_INTERSECT);
            if (radius || feather) emptyNativeRegion = selected->pixelSelection()->selectedExactRect().isEmpty();
            KisPaintDeviceSP source = new KisPaintDevice(cs);
            KisFillPainter fill(source);
            if (pattern) fill.fillRectNoCompose(region, patternResource, transform);
            else fill.fillRect(region, background ? resources->bgColor() : resources->fgColor());
            KisPainter painter(expectedDevice, selected);
            painter.setOpacityF(opacity); painter.setCompositeOpId(composite); painter.setChannelFlags(flags);
            painter.bitBlt(region.topLeft(), source, region);
            return;
        }
        if (contiguous) {
            KisFillPainter fill(expectedDevice, selection);
            fill.setWidth(region.width()); fill.setHeight(region.height());
            fill.setFillThreshold(threshold); fill.setOpacitySpread(spread);
            if (boundary) {
                auto bytes = Gpu::rgbaBytes({Gpu::maximumComponent(raster.format), 177, quint16(Gpu::maximumComponent(raster.format) / 3), Gpu::maximumComponent(raster.format)}, raster.format);
                exchangeRgbaBgra(bytes, stride);
                fill.setRegionFillingMode(KisFillPainter::RegionFillingMode_BoundaryFill);
                fill.setRegionFillingBoundaryColor(KoColor(reinterpret_cast<const quint8 *>(bytes.constData()), cs));
            }
            fill.setUseCompositing(true); fill.setCareForSelection(bool(selection));
            fill.setUseSelectionAsBoundary(bool(selection)); fill.setAntiAlias(smooth); fill.setSizemod(sizeMod); fill.setFeather(feather);
            fill.setOpacityF(opacity); fill.setCompositeOpId(composite); fill.setChannelFlags(flags);
            fill.setPaintColor(background ? resources->bgColor() : resources->fgColor());
            fill.setPattern(patternResource);
            const auto seed = regionSeed();
            if (radius || feather) {
                auto nativeMask = fill.createFloodSelection(seed.x(), seed.y(), reference, selection ? selection->pixelSelection() : KisPixelSelectionSP());
                if (selection) nativeMask->applySelection(selection->pixelSelection(), SELECTION_INTERSECT);
                emptyNativeRegion = nativeMask->selectedExactRect().isEmpty();
            }
            if (pattern) fill.fillPattern(seed.x(), seed.y(), reference, transform);
            else fill.fillColor(seed.x(), seed.y(), reference);
            return;
        }
        if (figure) {
            KisPainter painter(expectedDevice, selection);
            painter.setPattern(patternResource); painter.setPatternTransform(transform);
            painter.setFillStyle(KisPainter::FillStylePattern);
            painter.setOpacityF(opacity); painter.setCompositeOpId(composite); painter.setChannelFlags(flags);
            QPainterPath path;
            if (scenario.startsWith(QLatin1String("ellipse"))) path.addEllipse(QRectF(24, 24, 24, 24));
            else path.addRect(QRectF(24, 24, 24, 24));
            painter.fillPainterPath(path);
            return;
        }
        KisPaintDeviceSP source = new KisPaintDevice(cs);
        KisFillPainter fill(source);
        if (pattern) fill.fillRectNoCompose(region, patternResource, transform);
        else fill.fillRect(region, background ? resources->bgColor() : resources->fgColor(), OPACITY_OPAQUE_U8);
        KisPainter painter(expectedDevice, selection);
        painter.setOpacityF(opacity); painter.setCompositeOpId(composite); painter.setChannelFlags(flags);
        painter.bitBlt(region.topLeft(), source, region);
    };
    const auto expectedPixels = [&] {
        QByteArray result(inspected.width() * inspected.height() * stride, '\0');
        expectedDevice->readBytes(reinterpret_cast<quint8 *>(result.data()), inspected);
        exchangeRgbaBgra(result, stride);
        return result;
    };
    const auto saveRegionPixels = [&](const QString &stage, const QByteArray &actual, const QByteArray &expected) {
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!feather || results.isEmpty()) return true;
        if (!QDir().mkpath(results)) return false;
        const auto prefix = QStringLiteral("region-fill-%1-%2").arg(QString::fromLatin1(QTest::currentDataTag()), stage);
        for (const bool native : {false, true}) {
            QFile file(QDir(results).filePath(prefix + (native ? QStringLiteral("-native.rgba") : QStringLiteral("-gpu.rgba"))));
            const auto &bytes = native ? expected : actual;
            if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) return false;
        }
        const auto metadata = QStringLiteral("rect=%1,%2,%3,%4\ncomponentBits=%5\nfeather=%6\ngrow=%7\n")
            .arg(inspected.x()).arg(inspected.y()).arg(inspected.width()).arg(inspected.height())
            .arg(words ? 16 : 8).arg(feather).arg(sizeMod).toUtf8();
        QFile file(QDir(results).filePath(prefix + QStringLiteral(".txt")));
        return file.open(QIODevice::WriteOnly) && file.write(metadata) == metadata.size();
    };
    const auto click = [&] {
        if (action) {
            resources->setOpacity(opacity);
            auto *fill = window->viewManager()->actionManager()->actionByName(pattern ?
                QStringLiteral("fill_selection_pattern_opacity") : QStringLiteral("fill_selection_foreground_color_opacity"));
            if (!fill || !QTest::qWaitFor([&] { return fill->isEnabled(); })) return false;
            resources->setFGColor(KoColor(QColor(17, 129, 241, 19), cs));
            resources->setBGColor(KoColor(QColor(221, 17, 44, 80), cs));
            resources->slotPatternActivated(patternResource);
            resources->setOpacity(opacity);
            fill->trigger();
        } else if (api) {
            resources->setOpacity(opacity);
            KisFigurePaintingStroke stroke(kundo2_i18n("Pattern figure"), image,
                window->viewManager()->nodeManager()->activeNode(), resources->resourceManager(),
                KisFigurePaintingOptions::StrokeStyleNone, KisFigurePaintingOptions::FillStylePattern, transform);
            stroke.paintRect(QRectF(24, 24, 24, 24));
        } else {
            const auto toolId = !figure ? QStringLiteral("KritaFill/KisToolFill") :
                scenario.startsWith(QLatin1String("ellipse")) ? QStringLiteral("KritaShape/KisToolEllipse") : QStringLiteral("KritaShape/KisToolRectangle");
            KoToolManager::instance()->switchToolRequested(toolId);
            auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId));
            if (!tool) return false;
            resources->setEraserMode(erase || custom);
            resources->setCurrentCompositeOp(custom ? QStringLiteral("normal") : composite);
            resources->setMirrorHorizontal(false);
            resources->setMirrorVertical(false);
            resources->setGlobalAlphaLock(alphaLock);
            resources->setOpacity(custom ? .11 : opacity);
            if (figure) {
                for (QWidget *widget : tool->optionWidgets()) {
                    if (auto *outline = widget->findChild<QComboBox *>(QStringLiteral("cmbOutline"))) outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleNone);
                    if (auto *fill = widget->findChild<QComboBox *>(QStringLiteral("cmbFill"))) fill->setCurrentIndex(KisFigurePaintingOptions::FillStylePattern);
                }
                if (!QMetaObject::invokeMethod(tool, "patternRotationSettingChanged", Qt::DirectConnection, Q_ARG(qreal, rotation)) ||
                    !QMetaObject::invokeMethod(tool, "patternScaleSettingChanged", Qt::DirectConnection, Q_ARG(qreal, scale))) return false;
                // The controls own the transform used by the ordinary tool.
                for (QWidget *widget : tool->optionWidgets()) {
                    if (auto *angle = widget->findChild<KisAngleSelector *>(QStringLiteral("angleSelectorRotation"))) angle->setAngle(rotation);
                    if (auto *size = widget->findChild<KisDoubleSliderSpinBox *>(QStringLiteral("sldScale"))) size->setValue(scale);
                }
            }
            const auto point = view->canvasBase()->coordinatesConverter()->imageToDocument(contiguous || similar ? QPointF(regionSeed()) : QPointF(24, 24));
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton,
                              alternate ? Qt::AltModifier : Qt::NoModifier);
            KoPointerEvent first(&press, point); tool->beginPrimaryAction(&first);
            if (figure) {
                QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                KoPointerEvent update(&move, view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(48, 48)));
                tool->continuePrimaryAction(&update);
            }
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            KoPointerEvent last(&release, point);
            // Synthetic releases intentionally cancel ordinary rectangle tools.
            // Commit through the same explicit end request used by the native
            // figure fixtures; standalone acceptance uses physical releases.
            if (figure) tool->requestStrokeEnd();
            else tool->endPrimaryAction(&last);
        }
        return true;
    };
    if (edgeCanvas || radius || feather) initial = expectedPixels();
    const auto layer = owner->operations().activeLayer();
    const auto count = owner->operations().undoCount();
    QByteArray unfiltered;
    if (edgeCanvas && antiAlias) {
        const auto original = expectedDevice;
        expectedDevice = new KisPaintDevice(*original);
        applyExpected(false);
        unfiltered = expectedPixels();
        expectedDevice = original;
    }
    applyExpected(antiAlias);
    if (edgeCanvas && antiAlias) QVERIFY2(expectedPixels() != unfiltered, "The diagonal boundary must exercise native anti-aliasing");
    QVERIFY(click());
    if ((radius || feather) && emptyNativeRegion) {
        QCOMPARE(expectedPixels(), initial);
        QCOMPARE(owner->readRgbaPixels(layer, inspected, initial.size()).bytes, initial);
        QCOMPARE(owner->operations().undoCount(), count);
        return;
    }
    QTRY_COMPARE(owner->operations().undoCount(), count + 1);
    ++regionEdits;
    const auto expected = expectedPixels();
    auto actual = owner->readRgbaPixels(layer, inspected, initial.size());
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    QVERIFY(saveRegionPixels(QStringLiteral("first"), actual.bytes, expected));
    if (actual.bytes != expected) {
        for (int i = 0; i < actual.bytes.size(); ++i) if (actual.bytes[i] != expected[i]) {
            qWarning() << "fill mismatch at" << (i / stride) % inspected.width() << (i / stride) / inspected.width() << "byte" << i % stride
                       << "actual" << actual.bytes.mid((i / stride) * stride, stride).toHex()
                       << "native" << expected.mid((i / stride) * stride, stride).toHex(); break;
        }
    }
    { const QString difference = pixelStepDifference(actual.bytes, expected, words ? 2 : 1); QVERIFY2(difference.isEmpty(), qPrintable(difference)); }
    resources->setFGColor(KoColor(Qt::green, cs));
    QImage changedPattern(2, 2, QImage::Format_ARGB32);
    changedPattern.fill(Qt::yellow);
    resources->slotPatternActivated(KoPatternSP(new KoPattern(changedPattern, QStringLiteral("Changed resource"), QString())));
    auto *undo = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_undo"));
    auto *redo = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_redo"));
    QVERIFY(undo && redo);
    undo->trigger();
    QCOMPARE(owner->readRgbaPixels(layer, inspected, initial.size()).bytes, initial);
    redo->trigger();
    { const QString difference = pixelStepDifference(owner->readRgbaPixels(layer, inspected, initial.size()).bytes, expected, words ? 2 : 1); QVERIFY2(difference.isEmpty(), qPrintable(difference)); }
    QTemporaryDir directory;
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("fill.kra"));
    const auto savedLayers = owner->operations().snapshot()->layers;
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const auto decoded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(decoded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(decoded.diagnostic));
    if (labeled) for (const auto &before : savedLayers) {
        const auto after = std::find_if(decoded.document.image.layers.cbegin(), decoded.document.image.layers.cend(),
            [&](const auto &layer) { return layer.uuid == before.uuid; });
        QVERIFY(after != decoded.document.image.layers.cend());
        QCOMPARE(after->colorLabelIndex, before.colorLabelIndex);
        QCOMPARE(after->parentUuid, before.parentUuid);
        QCOMPARE(after->visible, before.visible);
        QCOMPARE(after->opacity, before.opacity);
        QCOMPARE(after->projectionChannelMask, before.projectionChannelMask);
        QCOMPARE(after->blendMode, before.blendMode);
        QCOMPARE(after->passThrough, before.passThrough);
    }
    auto reopened = Gpu::PaintImageSP::create(execution, decoded.document.image);
    Gpu::LayerId reopenedLayer = 0;
    for (const auto &candidate : reopened->operations().snapshot()->layers)
        if (candidate.uuid == fillUuid) reopenedLayer = candidate.id;
    QVERIFY(reopenedLayer);
    { const QString difference = pixelStepDifference(reopened->readRgbaPixels(reopenedLayer, inspected, initial.size()).bytes, expected, words ? 2 : 1); QVERIFY2(difference.isEmpty(), qPrintable(difference)); }
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened fill"), cs));
    owner = reopened;
    image = lifetime->image();
    QTRY_COMPARE(view->image().data(), image.data());
    QTRY_COMPARE(view->canvasBase()->image().data(), image.data());
    KisNodeSP reopenedNode = KisLayerUtils::findNodeByUuid(image->root(), fillUuid);
    QVERIFY(reopenedNode);
    window->viewManager()->nodeManager()->slotUiActivatedNode(reopenedNode);
    resources->setFGColor(KoColor(QColor(17, 129, 241, 19), cs));
    resources->slotPatternActivated(patternResource);
    resources->setEraserMode(erase || custom);
    resources->setCurrentCompositeOp(custom ? QStringLiteral("normal") : composite);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setGlobalAlphaLock(alphaLock);
    qobject_cast<KisPaintLayer *>(reopenedNode.data())->setChannelLockFlags(protect ? QBitArray{flags} : QBitArray());
    applyExpected(antiAlias);
    QVERIFY(click());
    const auto editedAgain = reopened->readRgbaPixels(reopenedLayer, inspected, initial.size()).bytes;
    QVERIFY(saveRegionPixels(QStringLiteral("reedit"), editedAgain, expectedPixels()));
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(editedAgain, expectedPixels(), words ? 2 : 1);
    undo->trigger();
    { const QString difference = pixelStepDifference(reopened->readRgbaPixels(reopenedLayer, inspected, initial.size()).bytes, expected, words ? 2 : 1); QVERIFY2(difference.isEmpty(), qPrintable(difference)); }
    redo->trigger();
    { const QString difference = pixelStepDifference(reopened->readRgbaPixels(reopenedLayer, inspected, initial.size()).bytes, expectedPixels(), words ? 2 : 1); QVERIFY2(difference.isEmpty(), qPrintable(difference)); }
}

