/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testui.h>
#include <testutil.h>
#include <KisPaintingEngineTestScope.h>
#include <KisGpuPixelTolerance.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <metadata/KoDocumentInfo.h>
#include <generator/kis_generator.h>
#include <generator/kis_generator_layer.h>
#include <generator/kis_generator_registry.h>
#include <filter/kis_filter_configuration.h>
#include <KisGlobalResourcesInterface.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <gpu/KisGpuPaintImage.h>
#include <QTemporaryDir>
#include <memory>

namespace {
std::unique_ptr<KisDocument> document()
{
    auto result = std::unique_ptr<KisDocument>(KisPart::instance()->createDocument());
    result->setFileBatchMode(true);
    return result;
}

QByteArray projection(KisImageSP image)
{
    image->waitForDone();
    QByteArray result(image->width() * image->height() * 4, Qt::Uninitialized);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(result.data()), image->bounds());
    return result;
}

KisGeneratorLayerSP fillLayer(KisImageSP image)
{
    for (auto node = image->root()->firstChild(); node; node = node->nextSibling())
        if (auto *layer = qobject_cast<KisGeneratorLayer *>(node.data())) return layer;
    return {};
}

LibrePaint::Image::Gpu::LayerId layerId(const LibrePaint::Image::Gpu::PaintImageSP &owner, const QUuid &uuid)
{
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.uuid == uuid) return layer.id;
    return 0;
}

QByteArray selectedPixels(KisGeneratorLayerSP layer)
{
    const auto image = layer->image();
    QByteArray result(image->width() * image->height(), Qt::Uninitialized);
    layer->internalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(result.data()), image->bounds());
    return result;
}

std::unique_ptr<KisDocument> sourceDocument(bool emptySelection)
{
    const TestUtil::PaintingEngineScope cpu(false);
    auto result = document();
    TestUtil::MaskParent fixture(QRect(0, 0, 289, 275));
    result->setCurrentImage(fixture.image);
    result->documentInfo()->setAboutInfo("title", fixture.image->objectName());
    const auto generator = KisGeneratorRegistry::instance()->value("color");
    auto configuration = generator->defaultConfiguration(KisGlobalResourcesInterface::instance());
    configuration->setProperty("color", QVariant::fromValue(KoColor(QColor(53, 137, 211), fixture.image->colorSpace())));
    KisSelectionSP selection = new KisSelection();
    if (!emptySelection) selection->pixelSelection()->select(QRect(7, 9, 276, 259), 181);
    KisGeneratorLayerSP layer = new KisGeneratorLayer(fixture.image, "editable fill", configuration->cloneWithResourcesSnapshot(), selection);
    layer->setOpacity(173);
    layer->setColorLabelIndex(3);
    layer->setPinnedToTimeline(true);
    fixture.image->addNode(layer);
    layer->update();
    fixture.image->waitForDone();
    return result;
}
}

class KisGpuKraGeneratorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void generatedLayerSurvivesEditingAndResaving();
    void emptySelectionAndRemovalStayEmpty();
};

void KisGpuKraGeneratorTest::generatedLayerSurvivesEditingAndResaving()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto source = sourceDocument(false);
    auto original = fillLayer(source->image());
    QVERIFY(original);
    const auto uuid = original->uuid();
    auto canonical = original->filter()->clone();
    QVERIFY(canonical->fromXML(original->filter()->toXML(), true));
    const auto originalConfiguration = canonical->toXML();
    const auto expected = projection(source->image());
    const auto selection = selectedPixels(original);
    const auto sourcePath = directory.filePath("source.kra");
    QVERIFY(source->exportDocumentSync(sourcePath, source->mimeType()));
    const TestUtil::PaintingEngineScope gpu(true);
    auto loaded = document();
    QVERIFY2(loaded->openPath(sourcePath, KisDocument::DontAddToRecent), qPrintable(loaded->errorMessage()));
    const auto owner = loaded->image()->gpuImage();
    QVERIFY(owner);
    auto layer = fillLayer(loaded->image());
    QVERIFY(layer);
    QCOMPARE(layer->uuid(), uuid);
    QCOMPARE(layer->filter()->toXML(), originalConfiguration);
    const auto selectionDifference = pixelStepDifference(selectedPixels(layer), selection, 1, 1);
    QVERIFY2(selectionDifference.isEmpty(), qPrintable(selectionDifference));
    QCOMPARE_PIXELS_WITHIN_STEP(projection(loaded->image()), expected);
    QCOMPARE(owner->operations().undoCount(), 0);
    auto changed = layer->filter()->cloneWithResourcesSnapshot();
    changed->setProperty("color", QVariant::fromValue(KoColor(QColor(211, 83, 17), loaded->image()->colorSpace())));
    const auto edit = owner->beginCpuLayerEdit(layerId(owner, uuid));
    QVERIFY(!edit.isNull());
    layer->setFilter(changed);
    QCOMPARE(owner->operations().undoCount(), 0);
    const auto preview = projection(loaded->image());
    QVERIFY(preview != expected);
    QCOMPARE(owner->commitCpuLayerEdit(edit, layer->name()), LibrePaint::Image::Gpu::Result::Success);
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto edited = projection(loaded->image());
    QCOMPARE_PIXELS_WITHIN_STEP(edited, preview);
    QVERIFY(edited != expected);
    const auto editedConfiguration = layer->filter()->toXML();
    QCOMPARE(owner->undo(), LibrePaint::Image::Gpu::Result::Success);
    QCOMPARE(layer->filter()->toXML(), originalConfiguration);
    QCOMPARE_PIXELS_WITHIN_STEP(projection(loaded->image()), expected);
    QCOMPARE(owner->redo(), LibrePaint::Image::Gpu::Result::Success);
    QCOMPARE(layer->filter()->toXML(), editedConfiguration);
    QCOMPARE_PIXELS_WITHIN_STEP(projection(loaded->image()), edited);
    const auto savedPath = directory.filePath("saved.kra");
    QVERIFY(loaded->exportDocumentSync(savedPath, loaded->mimeType()));
    for (const bool useGpu : {true, false}) {
        const TestUtil::PaintingEngineScope engine(useGpu);
        auto reopened = document();
        QVERIFY2(reopened->openPath(savedPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        QCOMPARE(bool(reopened->image()->gpuImage()), useGpu);
        auto restored = fillLayer(reopened->image());
        QVERIFY(restored);
        QCOMPARE(restored->uuid(), uuid);
        QCOMPARE(restored->name(), layer->name());
        QCOMPARE(restored->opacity(), layer->opacity());
        QCOMPARE(restored->colorLabelIndex(), layer->colorLabelIndex());
        QCOMPARE(restored->isPinnedToTimeline(), layer->isPinnedToTimeline());
        QCOMPARE(restored->filter()->toXML(), editedConfiguration);
        const auto selectionDifference = pixelStepDifference(selectedPixels(restored), selection, 1, 1);
        QVERIFY2(selectionDifference.isEmpty(), qPrintable(selectionDifference));
        QCOMPARE_PIXELS_WITHIN_STEP(projection(reopened->image()), edited);
    }
}

void KisGpuKraGeneratorTest::emptySelectionAndRemovalStayEmpty()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto source = sourceDocument(true);
    const auto expected = projection(source->image());
    const auto path = directory.filePath("empty.kra");
    QVERIFY(source->exportDocumentSync(path, source->mimeType()));
    const TestUtil::PaintingEngineScope gpu(true);
    auto loaded = document();
    QVERIFY2(loaded->openPath(path, KisDocument::DontAddToRecent), qPrintable(loaded->errorMessage()));
    auto layer = fillLayer(loaded->image());
    QVERIFY(layer);
    const auto owner = loaded->image()->gpuImage();
    QVERIFY(owner);
    const auto edit = owner->beginCpuLayerEdit(layerId(owner, layer->uuid()));
    QVERIFY(!edit.isNull());
    auto changed = layer->filter()->cloneWithResourcesSnapshot();
    changed->setProperty("color", QVariant::fromValue(KoColor(Qt::green, loaded->image()->colorSpace())));
    layer->setFilter(changed);
    QCOMPARE(owner->cancelCpuLayerEdit(edit), LibrePaint::Image::Gpu::Result::Success);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE_PIXELS_WITHIN_STEP(projection(loaded->image()), expected);
    QVERIFY(layer->internalSelection()->selectedExactRect().isEmpty());
    const auto id = layerId(owner, layer->uuid());
    QCOMPARE(owner->removeLayer(id), LibrePaint::Image::Gpu::Result::Success);
    const auto removed = directory.filePath("removed.kra");
    QVERIFY(loaded->exportDocumentSync(removed, loaded->mimeType()));
    for (const bool useGpu : {true, false}) {
        const TestUtil::PaintingEngineScope engine(useGpu);
        auto reopened = document();
        QVERIFY(reopened->openPath(removed, KisDocument::DontAddToRecent));
        QVERIFY(!fillLayer(reopened->image()));
        QCOMPARE_PIXELS_WITHIN_STEP(projection(reopened->image()), expected);
    }
}

KISTEST_MAIN(KisGpuKraGeneratorTest)
#include "KisGpuKraGeneratorTest.moc"
