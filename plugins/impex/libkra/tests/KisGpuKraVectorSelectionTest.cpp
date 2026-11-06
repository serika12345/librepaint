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
#include <KoPathShape.h>
#include <KoStore.h>
#include <KisImageResolutionProxy.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_shape_selection.h>
#include <kis_pixel_selection.h>
#include <gpu/KisGpuPaintImage.h>
#include <QTemporaryDir>
#include <QVariantMap>
#include <memory>
#include <QDomDocument>

namespace {
std::unique_ptr<KisDocument> sourceDocument()
{
    const TestUtil::PaintingEngineScope cpu(false);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    TestUtil::MaskParent fixture(QRect(0, 0, 289, 275));
    fixture.image->setResolution(1.5, 1.5);
    document->setCurrentImage(fixture.image);
    document->documentInfo()->setAboutInfo("title", fixture.image->objectName());
    fixture.layer->paintDevice()->fill(QRect(-4, -3, 296, 281),
        KoColor(QColor(53, 137, 211, 107), fixture.layer->colorSpace()));
    KisSelectionSP selection = new KisSelection(fixture.layer->paintDevice()->defaultBounds(),
        toQShared(new KisImageResolutionProxy(fixture.image)));
    auto *shapes = new KisShapeSelection(document->shapeController(), selection);
    selection->convertToVectorSelectionNoUndo(shapes);
    auto *path = new KoPathShape();
    path->setShapeId(KoPathShapeId);
    path->setName("editable_selection");
    path->moveTo({4.25, 7.75});
    path->lineTo({187.5, 12.125});
    path->lineTo({179.125, 177.75});
    path->lineTo({9.75, 171.25});
    path->close();
    path->normalize();
    shapes->addShape(path);
    KisTransparencyMaskSP mask = new KisTransparencyMask(fixture.image, "vector mask");
    mask->setColorLabelIndex(3);
    mask->setPinnedToTimeline(true);
    mask->setSelection(selection);
    fixture.image->addNode(mask, fixture.layer);
    selection->updateProjection();
    mask->setDirty(fixture.image->bounds());
    fixture.image->waitForDone();
    return document;
}

KisTransparencyMaskSP vectorMask(KisImageSP image)
{
    return qobject_cast<KisTransparencyMask *>(image->root()->firstChild()->firstChild().data());
}

QByteArray projection(KisImageSP image)
{
    QByteArray pixels(image->width() * image->height() * 4, Qt::Uninitialized);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
    return pixels;
}

QVariantList vectorDescription(KisTransparencyMaskSP mask)
{
    const auto *shapes = dynamic_cast<const KisShapeSelection *>(mask->selection()->shapeSelection());
    if (!shapes) return {};
    QVariantList description;
    for (const auto *shape : shapes->shapes()) {
        QVariantMap value;
        value.insert("name", shape->name());
        value.insert("z-index", shape->zIndex());
        value.insert("outline", QVariant::fromValue(shape->absoluteTransformation().map(shape->outline())));
        description.append(value);
    }
    return description;
}

QString vectorEntry(const QString &path)
{
    QScopedPointer<KoStore> store(KoStore::createStore(path, KoStore::Read));
    if (!store || !store->open("maindoc.xml")) return {};
    QDomDocument xml;
    xml.setContent(store->read(store->size()));
    store->close();
    const auto image = xml.documentElement().firstChildElement("IMAGE");
    const auto mask = image.firstChildElement("layers").firstChildElement("layer").firstChildElement("masks").firstChildElement("mask");
    return image.attribute("name") + "/layers/" + mask.attribute("filename") + ".shapeselection/content.svg";
}
}

class KisGpuKraVectorSelectionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void vectorSelectionSurvivesEditingAndResaving();
    void rasterReplacementAndRemovalDiscardVectorData();
    void vectorSelectionSurvivesMigration();
};

void KisGpuKraVectorSelectionTest::vectorSelectionSurvivesEditingAndResaving()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto source = sourceDocument();
    const auto originalMask = vectorMask(source->image());
    const auto uuid = originalMask->uuid();
    const auto sourcePath = directory.filePath("source.kra");
    QVERIFY(source->exportDocumentSync(sourcePath, source->mimeType()));
    const auto expected = projection(source->image());
    const TestUtil::PaintingEngineScope gpu(true);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY2(document->openPath(sourcePath, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    QVERIFY(document->image()->gpuImage());
    const auto owner = document->image()->gpuImage();
    QCOMPARE(owner->operations().undoCount(), 0);
    auto mask = vectorMask(document->image());
    QCOMPARE(mask->uuid(), uuid);
    QCOMPARE(mask->colorLabelIndex(), 3);
    QVERIFY(mask->isPinnedToTimeline());
    QCOMPARE(vectorDescription(mask), vectorDescription(originalMask));
    QCOMPARE_PIXELS_WITHIN_STEP(projection(document->image()), expected);
    auto *shapes = dynamic_cast<KisShapeSelection *>(mask->selection()->shapeSelection());
    QVERIFY(shapes);
    QCOMPARE(shapes->shapes().size(), 1);
    const auto oldPosition = shapes->shapes().first()->position();
    shapes->shapes().first()->setPosition(oldPosition + QPointF(2.5, -1.25));
    shapes->shapes().first()->setName("edited_selection");
    QTRY_COMPARE(owner->operations().undoCount(), 1);
    const auto editedShapes = vectorDescription(mask);
    const auto editedPixels = projection(document->image());
    QVERIFY(editedPixels != expected);
    QCOMPARE(owner->undo(), LibrePaint::Image::Gpu::Result::Success);
    QCOMPARE_PIXELS_WITHIN_STEP(projection(document->image()), expected);
    QCOMPARE(owner->redo(), LibrePaint::Image::Gpu::Result::Success);
    QCOMPARE(vectorDescription(mask), editedShapes);
    shapes = dynamic_cast<KisShapeSelection *>(mask->selection()->shapeSelection());
    QVERIFY(shapes);
    shapes->shapes().first()->setPosition(shapes->shapes().first()->position() + QPointF(0.125, 0.25));
    const auto savedShapes = vectorDescription(mask);
    const auto savedPath = directory.filePath("saved.kra");
    QVERIFY(document->exportDocumentSync(savedPath, document->mimeType()));
    const auto savedPixels = projection(document->image());
    QVERIFY(savedPixels != editedPixels);
    for (const bool useGpu : {true, false}) {
        const TestUtil::PaintingEngineScope engine(useGpu);
        std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(savedPath, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        QCOMPARE(bool(reopened->image()->gpuImage()), useGpu);
        const auto restored = vectorMask(reopened->image());
        QVERIFY(restored);
        QCOMPARE(restored->uuid(), uuid);
        QCOMPARE(restored->name(), mask->name());
        QCOMPARE(restored->colorLabelIndex(), mask->colorLabelIndex());
        QCOMPARE(restored->isPinnedToTimeline(), mask->isPinnedToTimeline());
        QCOMPARE(reopened->image()->xRes(), 1.5);
        QCOMPARE(reopened->image()->yRes(), 1.5);
        QCOMPARE(vectorDescription(restored), savedShapes);
        QCOMPARE_PIXELS_WITHIN_STEP(projection(reopened->image()), savedPixels);
    }
}

void KisGpuKraVectorSelectionTest::rasterReplacementAndRemovalDiscardVectorData()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto source = sourceDocument();
    const auto sourcePath = directory.filePath("source.kra");
    QVERIFY(source->exportDocumentSync(sourcePath, source->mimeType()));
    const auto entry = vectorEntry(sourcePath);
    QVERIFY(!entry.isEmpty());
    const TestUtil::PaintingEngineScope gpu(true);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY2(document->openPath(sourcePath, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    auto image = document->image();
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    auto mask = vectorMask(image);
    QVERIFY(mask->selection()->hasShapeSelection());
    const auto layer = owner->operations().snapshot()->layers.first().id;
    QCOMPARE(owner->clearMaskPixels(layer, false), LibrePaint::Image::Gpu::Result::Success);
    QVERIFY(!mask->selection()->hasShapeSelection());
    QCOMPARE(owner->undo(), LibrePaint::Image::Gpu::Result::Success);
    QVERIFY(mask->selection()->hasNonEmptyShapeSelection());
    QVERIFY(image->removeGpuTransparencyMask(mask->parent()));
    const auto savedPath = directory.filePath("removed.kra");
    QVERIFY(document->exportDocumentSync(savedPath, document->mimeType()));
    QScopedPointer<KoStore> store(KoStore::createStore(savedPath, KoStore::Read));
    QVERIFY(store);
    QVERIFY(!store->hasFile(entry));
    for (const bool useGpu : {true, false}) {
        const TestUtil::PaintingEngineScope engine(useGpu);
        std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY(reopened->openPath(savedPath, KisDocument::DontAddToRecent));
        QVERIFY(!reopened->image()->root()->firstChild()->firstChild());
        QCOMPARE_PIXELS_WITHIN_STEP(projection(reopened->image()), projection(image));
    }
}

void KisGpuKraVectorSelectionTest::vectorSelectionSurvivesMigration()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto source = sourceDocument();
    const auto sourcePath = directory.filePath("source.kra");
    QVERIFY(source->exportDocumentSync(sourcePath, source->mimeType()));

    const TestUtil::PaintingEngineScope gpu(true);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY2(document->openPath(sourcePath, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    auto image = document->image();
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    auto mask = vectorMask(image);
    QVERIFY(mask);
    auto selection = mask->selection();
    QVERIFY(selection);
    const auto edit = [&](const QString &name, const QPointF &offset) {
        auto *shapes = dynamic_cast<KisShapeSelection *>(selection->shapeSelection());
        QVERIFY(shapes);
        QCOMPARE(shapes->shapes().size(), 1);
        auto *shape = shapes->shapes().first();
        shape->setPosition(shape->position() + offset);
        shape->setName(name);
    };

    edit(QStringLiteral("first_edit"), QPointF(3.5, -1.25));
    QTRY_COMPARE(owner->operations().undoCount(), 1);
    edit(QStringLiteral("second_edit"), QPointF(0.125, 0.25));
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const auto secondDescription = vectorDescription(mask);
    const auto secondPixels = projection(image);

    QVERIFY(document->migrateToCpu(64 * 1024 * 1024));
    QVERIFY(!image->gpuImage());
    QCOMPARE(vectorMask(image), mask);
    QCOMPARE(mask->selection(), selection);
    auto *stack = document->undoStack();
    QCOMPARE(stack->index(), 0);
    QCOMPARE(stack->count(), 0);
    QVERIFY(!stack->canUndo());
    QVERIFY(!stack->canRedo());
    QCOMPARE(vectorDescription(mask), secondDescription);
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_STEP(projection(image), secondPixels);

    // The current shape survives adoption; rasterising it starts a new stable
    // native history on the same selection object.
    KUndo2Command *flatten = selection->flatten();
    QVERIFY(flatten);
    stack->push(flatten);
    image->waitForDone();
    QVERIFY(!selection->hasShapeSelection());
    QCOMPARE_PIXELS_WITHIN_STEP(projection(image), secondPixels);
    QCOMPARE(stack->count(), 1);
    stack->undo();
    image->waitForDone();
    QVERIFY(selection->hasShapeSelection());
    QCOMPARE(vectorDescription(mask), secondDescription);
    stack->redo();
    image->waitForDone();
    QVERIFY(!selection->hasShapeSelection());
    stack->undo();
    image->waitForDone();
    QCOMPARE(vectorDescription(mask), secondDescription);
}

KISTEST_MAIN(KisGpuKraVectorSelectionTest)
#include "KisGpuKraVectorSelectionTest.moc"
