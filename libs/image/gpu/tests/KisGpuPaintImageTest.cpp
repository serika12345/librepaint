/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QTest>
#include <limits>
#include <stdexcept>

using namespace LibrePaint::Image::Gpu;

class KisGpuPaintImageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void retainedLayerPixelsRemainEditableThroughRemovalAndDiscardedHistory();
    void rejectedRetainedLayerWritesPreserveVersionsAndHistory();
    void retainedImagesAndClonesKeepTheirExecutionAlive();
    void clonesKeepIndependentCurrentStateAndOperationIdentities();
    void retainedLayerCopiesKeepIndependentPixelsAndExecutionLifetime();
    void sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment();
    void recoveryPreservesPendingInputAndRedoOnTheSameAttachment();
    void directPixelsReachGpuWithoutEditingNotificationsOrHistory();
    void rectangularWritesPreserveOtherRegionsAcrossHistoryAndRecovery();
    void rectangularWritesValidateBeforeMutationAndCrossTileBoundaries();
    void rectangularHistoryEditsPreserveIndependentUntrackedPixels();
    void damagedPixelSharingRecoveryPreservesTheLiveImage();
    void historyNavigationPublishesOneVersionAndPreservesSavedState();
};

void KisGpuPaintImageTest::retainedLayerPixelsRemainEditableThroughRemovalAndDiscardedHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(16, 16));
    const auto bottom = image.operations().activeLayer();
    const auto layer = image.addLayer(QStringLiteral("Retained"));
    const auto pixels = image.retainLayerPixels(layer);
    QVERIFY(pixels);
    QCOMPARE(image.retainLayerPixels(layer), pixels);
    const auto first = QByteArray::fromHex("112233ff");
    const auto second = QByteArray::fromHex("445566aa");
    const QRect rect(5, 5, 1, 1);
    QCOMPARE(image.writeRgbaPixels(layer, rect, first), Result::Success);
    QCOMPARE(pixels->version(), image.operations().snapshot());
    const auto pinned = pixels->version();
    const auto before = image.operations().undoCount();
    QCOMPARE(image.removeLayer(layer), Result::Success);
    const auto removed = image.operations().snapshot();
    QCOMPARE(image.markSaved(removed), Result::Success);
    QCOMPARE(pixels->version(), pinned);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.writeRgbaPixels(pixels, rect, second), Result::Success);
    QCOMPARE(image.operations().snapshot(), removed);
    QCOMPARE(image.operations().undoCount(), before + 1);
    QVERIFY(!image.operations().isModified());
    QVERIFY(changed.isEmpty());
    QCOMPARE(execution.readRgbaPixels(image.handle(), pixels->version(), rect, layer, 4).bytes, second);
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, rect, layer, 4).bytes, first);
    PaintImage recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(image.operations().recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.readRgbaPixels(layer, rect, 4).bytes, second);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, rect, 4).bytes, second);
    QCOMPARE(pixels->version(), image.operations().snapshot());
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.renameLayer(bottom, QStringLiteral("Discarded layer branch")), Result::Success);
    QCOMPARE(image.operations().redoCount(), 0);
    const auto branch = image.operations().snapshot();
    const auto history = image.operations().undoCount();
    const auto lastPixels = pixels->version();
    QCOMPARE(image.writeRgbaPixels(pixels, rect, first), Result::Success);
    QCOMPARE(image.operations().snapshot(), branch);
    QCOMPARE(image.operations().undoCount(), history);
    QCOMPARE(image.readRgbaPixels(bottom, rect, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), pixels->version(), rect, layer, 4).bytes, first);
    QCOMPARE(execution.readRgbaPixels(image.handle(), lastPixels, rect, layer, 4).bytes, second);
    QVERIFY(!image.retainLayerPixels(layer));
}

void KisGpuPaintImageTest::rejectedRetainedLayerWritesPreserveVersionsAndHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    Limits limits;
    limits.rasterBytes = 2 * RgbaTile::ByteCount;
    PaintImage image(execution, QSize(16, 16), limits);
    PaintImage other(execution, QSize(16, 16));
    const auto layer = image.operations().activeLayer();
    const auto pixels = image.retainLayerPixels(layer);
    const auto foreign = other.retainLayerPixels(other.operations().activeLayer());
    QVERIFY(pixels && foreign);
    const QRect rect(5, 5, 1, 1);
    const auto bytes = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, rect, bytes), Result::Success);
    QCOMPARE(image.removeLayer(layer), Result::Success);
    QCOMPARE(image.writeRgbaPixels(pixels, rect, bytes), Result::Success);
    const auto pinned = pixels->version();
    const auto before = image.operations().recoveryData();
    QCOMPARE(image.writeRgbaPixels(pixels, rect, bytes), Result::CapacityExceeded);
    QCOMPARE(image.writeRgbaPixels(foreign, rect, bytes), Result::InvalidInput);
    QCOMPARE(pixels->version(), pinned);
    QCOMPARE(image.operations().recoveryData(), before);
}

void KisGpuPaintImageTest::retainedImagesAndClonesKeepTheirExecutionAlive()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    std::weak_ptr<Execution> lifetime = execution;
    auto image = PaintImageSP::create(execution, QSize(32, 32));
    const auto layer = image->operations().activeLayer();
    QCOMPARE(image->writeRgbaPixels(layer, QRect(4, 5, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    auto clone = image->clone(true);
    execution.reset();
    device.reset();
    image.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(clone->readRgbaPixels(layer, QRect(4, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));

    auto destinationExecution = std::make_shared<Execution>(Device::createNative(diagnostic));
    std::weak_ptr<Execution> destinationLifetime = destinationExecution;
    auto destination = PaintImageSP::create(destinationExecution, QSize(32, 32));
    auto replacement = clone->cloneTo(*destination, true);
    destinationExecution.reset();
    destination.reset();
    clone.reset();
    QVERIFY(lifetime.expired());
    QVERIFY(!destinationLifetime.expired());
    QCOMPARE(replacement->readRgbaPixels(layer, QRect(4, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    replacement.reset();
    QVERIFY(destinationLifetime.expired());
    QVERIFY_EXCEPTION_THROWN(PaintImage(std::shared_ptr<Execution>{}, QSize(32, 32)), std::invalid_argument);
}

void KisGpuPaintImageTest::clonesKeepIndependentCurrentStateAndOperationIdentities()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    auto source = PaintImageSP::create(execution, QSize(64, 64));
    const auto layer = source->operations().activeLayer();
    const auto stroke = source->beginStroke(layer, Brush{}, 17);
    QCOMPARE(source->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source->commitStroke(stroke), Result::Success);
    const auto hidden = source->addLayer(QStringLiteral("Hidden"));
    QVERIFY(hidden);
    QCOMPARE(source->writeRgbaPixels(hidden, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Result::Success);
    QCOMPARE(source->setLayerVisible(hidden, false), Result::Success);
    const auto before = source->operations().snapshot();
    const auto history = source->operations().undoCount();
    auto exact = source->clone(true);
    QVERIFY(exact);
    QVERIFY(exact->handle() != source->handle());
    QVERIFY(exact->operations().snapshot()->documentId() != before->documentId());
    QCOMPARE(exact->operations().undoCount(), 0);
    QCOMPARE(exact->operations().redoCount(), 0);
    QVERIFY(!exact->operations().isModified());
    QCOMPARE(exact->operations().snapshot()->layers[0].uuid, before->layers[0].uuid);
    QCOMPARE(exact->readRgbaPixels(layer, QRect(12, 12, 1, 1), 4).bytes, source->readRgbaPixels(layer, QRect(12, 12, 1, 1), 4).bytes);
    QCOMPARE(exact->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(exact->restoreRecovery(exact->operations().recoveryData()), Result::Success);
    QCOMPARE(exact->replaceRgbaPixels(layer, QRect(12, 12, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(source->operations().snapshot(), before);
    QCOMPARE(source->operations().undoCount(), history);
    auto renamed = source->clone(false);
    QVERIFY(renamed);
    for (int index = 0; index < before->layers.size(); ++index)
        QVERIFY(renamed->operations().snapshot()->layers[index].uuid != before->layers[index].uuid);
    Execution destination(device);
    auto transferred = source->cloneTo(destination, true);
    QVERIFY(transferred);
    QCOMPARE(&transferred->execution(), &destination);
    QCOMPARE(destination.requested(transferred->handle()), transferred->operations().snapshot());
    QCOMPARE(transferred->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    Execution full(device, 128 * 1024 * 1024, 1);
    PaintImage oneSlot(full, QSize(64, 64));
    const auto retained = oneSlot.operations().snapshot();
    QVERIFY_EXCEPTION_THROWN(oneSlot.clone(true), std::length_error);
    QVERIFY_EXCEPTION_THROWN(oneSlot.cloneLayerPixels(oneSlot.retainLayerPixels(oneSlot.operations().activeLayer())), std::length_error);
    QCOMPARE(oneSlot.operations().snapshot(), retained);
    QCOMPARE(full.requested(oneSlot.handle()), retained);
}

void KisGpuPaintImageTest::retainedLayerCopiesKeepIndependentPixelsAndExecutionLifetime()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    std::weak_ptr<Execution> lifetime = execution;
    auto source = PaintImageSP::create(execution, QSize(32, 32));
    const auto layer = source->addLayer(QStringLiteral("Retained copy"));
    const auto stroke = source->beginStroke(layer, Brush{}, 17);
    QCOMPARE(source->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source->commitStroke(stroke), Result::Success);
    QCOMPARE(source->writeRgbaPixels(layer, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Result::Success);
    const auto pixels = source->retainLayerPixels(layer);
    const auto retained = pixels->version();
    const auto expected = execution->readRgbaPixels(source->handle(), retained, QRect(0, 0, 32, 32), layer, 4096).bytes;
    QCOMPARE(source->removeLayer(layer), Result::Success);
    const auto before = source->operations().snapshot();
    QVERIFY(!source->cloneLayerPixels({}));
    QVERIFY(!source->cloneLayerPixels(PaintImageSP::create(execution, QSize(1, 1))->retainLayerPixels(1)));
    auto copy = source->cloneLayerPixels(pixels);
    QVERIFY(copy);
    const auto version = copy->operations().snapshot();
    QCOMPARE(version->layers.size(), 1);
    QCOMPARE(version->activeLayer, layer);
    QCOMPARE(version->selectedLayers, QList<LayerId>{layer});
    QCOMPARE(version->sampleCount, retained->sampleCount);
    QCOMPARE(version->rasterBytes, retained->rasterBytes);
    QVERIFY(version->documentId() != retained->documentId());
    QCOMPARE(copy->operations().undoCount(), 0);
    QVERIFY(!copy->operations().isModified());
    QCOMPARE(copy->readRgbaPixels(layer, QRect(0, 0, 32, 32), 4096).bytes, expected);
    QCOMPARE(copy->readRgbaPixels(layer, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(copy->restoreRecovery(copy->operations().recoveryData()), Result::Success);
    QCOMPARE(copy->writeRgbaPixels(layer, QRect(-5, 7, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(execution->readRgbaPixels(source->handle(), retained, QRect(-5, 7, 1, 1), layer, 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(source->operations().snapshot(), before);
    source.reset();
    execution.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(copy->readRgbaPixels(layer, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    copy.reset();
    QVERIFY(lifetime.expired());
}

void KisGpuPaintImageTest::historyNavigationPublishesOneVersionAndPreservesSavedState()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.addLayer(QStringLiteral("Second layer"));
    QVERIFY(layer);
    QCOMPARE(image.renameLayer(layer, QStringLiteral("Saved layer")), Result::Success);
    QSignalSpy saved(&image, &PaintImage::savedStateChanged);
    QCOMPARE(image.markSaved(image.operations().snapshot()), Result::Success);
    QCOMPARE(saved.size(), 1);
    QCOMPARE(image.renameLayer(layer, QStringLiteral("Later layer")), Result::Success);
    const auto versions = image.operations().historyVersions();
    QCOMPARE(versions.size(), 4);
    QCOMPARE(image.operations().savedHistoryIndex(), 2);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.setHistoryIndex(0), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(execution.requested(image.handle()), versions[0]);
    QCOMPARE(image.operations().redoCount(), 3);
    QVERIFY(image.operations().isModified());
    QCOMPARE(image.setHistoryIndex(-1), Result::InvalidInput);
    QCOMPARE(image.setHistoryIndex(4), Result::InvalidInput);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.setHistoryIndex(2), Result::Success);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(image.operations().historyVersions(), versions);
    const auto stroke = image.beginStroke(layer, {}, 17);
    QCOMPARE(image.appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    const auto pending = image.operations().preview();
    const int notifications = changed.size();
    QCOMPARE(image.setHistoryIndex(0), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), pending);
    QCOMPARE(changed.size(), notifications);
    QCOMPARE(image.cancelStroke(stroke), Result::Success);
    QCOMPARE(image.setHistoryIndex(3), Result::Success);
    QVERIFY(image.operations().isModified());
}

void KisGpuPaintImageTest::sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    auto owner = PaintImageSP::create(execution, QSize(64, 64));
    auto reader = owner;
    const auto handle = owner->handle();
    QVERIFY(handle);
    QSignalSpy changed(owner.data(), &PaintImage::changed);
    bool currentAtNotification = true;
    connect(owner.data(), &PaintImage::changed, this, [&] {
        currentAtNotification &= execution.requested(handle) == reader->operations().preview();
    });
    owner.clear();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    const auto stroke = reader->beginStroke(reader->operations().activeLayer(), brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(reader->appendSamples(stroke, {{{12, 12}, 1}, {{32, 32}, 0.5}}), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(reader->commitStroke(stroke), Result::Success);
    QCOMPARE(reader->operations().undoCount(), 1);
    QVERIFY(reader->operations().isModified());
    const auto pixels = execution.readPixels(handle, reader->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QCOMPARE(pixels.tiles.size(), 1);
    QCOMPARE(pixels.tiles.front().rgba.mid((12 * 256 + 12) * 4, 4), QByteArray::fromHex("1181f1ff"));
    const auto count = changed.size();
    QCOMPARE(reader->markSaved(reader->operations().snapshot()), Result::Success);
    QVERIFY(!reader->operations().isModified());
    QCOMPARE(changed.size(), count);
    QCOMPARE(reader->undo(), Result::Success);
    QCOMPARE(reader->redo(), Result::Success);
    QVERIFY(currentAtNotification);
    reader.clear();
    QVERIFY(!execution.requested(handle));
    QCOMPARE(execution.pendingDocuments(), qsizetype(0));
}

void KisGpuPaintImageTest::recoveryPreservesPendingInputAndRedoOnTheSameAttachment()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 128 * 1024 * 1024, 1);
    PaintImage image(execution, QSize(64, 64));
    const auto handle = image.handle();
    const auto previous = image.operations().snapshot();
    QSignalSpy changed(&image, &PaintImage::changed);
    OperationDocument source({64, 64});
    Brush red;
    red.rgba = {255, 0, 0, 255};
    const auto committed = source.beginStroke(source.activeLayer(), red, 1);
    QCOMPARE(source.appendSamples(committed, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source.commitStroke(committed), Result::Success);
    QCOMPARE(source.undo(), Result::Success);
    Brush blue;
    blue.rgba = {0, 0, 255, 255};
    const auto pending = source.beginStroke(source.activeLayer(), blue, 2);
    QCOMPARE(source.appendSamples(pending, {{{32, 32}, 1}}), Result::Success);
    const auto record = source.recoveryData();
    auto corrupt = record;
    corrupt[corrupt.size() - 1] ^= 1;
    QCOMPARE(image.restoreRecovery(corrupt), Result::InvalidRecovery);
    QCOMPARE(image.operations().snapshot(), previous);
    QCOMPARE(execution.requested(handle), previous);
    QVERIFY(changed.isEmpty());
    QCOMPARE(image.restoreRecovery(record), Result::Success);
    QCOMPARE(image.handle(), handle);
    QVERIFY(image.operations().snapshot()->documentId() != previous->documentId());
    QCOMPARE(changed.size(), 1);
    QCOMPARE(execution.requested(handle), image.operations().preview());
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 1);
    const auto preview = execution.readPixels(handle, image.operations().preview(), {0, 0, 64, 64});
    QCOMPARE(preview.code, ExecutionCode::Success);
    QCOMPARE(preview.tiles.size(), 1);
    QCOMPARE(preview.tiles.front().rgba.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(image.cancelStroke(image.operations().activeOperation()), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    image.close();
    QCOMPARE(image.handle(), quint64(0));
    QVERIFY(!execution.requested(handle));
    QCOMPARE(image.undo(), Result::Closed);
}

void KisGpuPaintImageTest::directPixelsReachGpuWithoutEditingNotificationsOrHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto layer = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    const RgbaTile pixels{{-1, -1}, QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(image.writeRgbaPixels(layer, {-256, -256, 256, 256}, pixels.rgba), Result::Success);
    QVERIFY(changed.isEmpty());
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 0);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.requested(image.handle()), image.operations().snapshot());
    const auto read = execution.readPixels(image.handle(), image.operations().snapshot(), {-1, -1, 1, 1},
                                           {layer}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.tiles.size(), 1);
    QCOMPARE(read.tiles.first().rgba, pixels.rgba);
    const auto old = execution.readPixels(image.handle(), before, {-1, -1, 1, 1},
                                          {layer}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(old.code, ExecutionCode::Success);
    QVERIFY(old.tiles.isEmpty());
    image.close();
    QCOMPARE(image.writeRgbaPixels(layer, {-256, -256, 256, 256}, pixels.rgba), Result::Closed);
}

void KisGpuPaintImageTest::rectangularWritesPreserveOtherRegionsAcrossHistoryAndRecovery()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage definition;
    definition.size = {128, 128};
    definition.layers = {{QStringLiteral("Edited"), true, {},
                          {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
                         {QStringLiteral("Imported"), true, {}, {}}};
    PaintImage image(execution, definition);
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto pinned = image.operations().snapshot();
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    brush.diameter = 6;
    const auto stroke = image.beginStroke(1, brush, 0);
    QCOMPARE(image.appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    const auto notifications = changed.size();
    const auto raw = QByteArray::fromHex("120a2700");
    const auto green = QByteArray::fromHex("00ff00ff");
    QCOMPARE(image.writeRgbaPixels(2, {-1, -1, 1, 1}, raw), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {8, 12, 1, 1}, raw), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, raw), Result::Success);
    QCOMPARE(changed.size(), notifications);
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 1);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, {8, 12, 1, 1}, 1, 4).bytes, QByteArray::fromHex("ff0000ff"));
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, green), Result::Success);
    PaintImage restored(execution, QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(image.operations().recoveryData()), Result::Success);
    for (auto *owner : {&image, &restored}) {
        QCOMPARE(owner->redo(), Result::Success);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes,
                 QByteArray::fromHex("0000ffff"));
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, green);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {-1, -1, 1, 1}, 2, 4).bytes, raw);
        QCOMPARE(owner->undo(), Result::Success);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes, raw);
        const auto yellow = QByteArray::fromHex("ffff00ff");
        QCOMPARE(owner->writeRgbaPixels(1, {96, 96, 1, 1}, yellow), Result::Success);
        QCOMPARE(owner->redo(), Result::Success);
        QCOMPARE(execution.readRgbaPixels(owner->handle(), owner->operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, yellow);
        QCOMPARE(owner->undo(), Result::Success);
        QVERIFY(!owner->operations().isModified());
    }
}

void KisGpuPaintImageTest::rectangularWritesValidateBeforeMutationAndCrossTileBoundaries()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 3 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    PaintImage image(execution, QSize(64, 64));
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto layer = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    const QRect region(-1, -1, 2, 2);
    const auto pixels = QByteArray::fromHex("120a2700197fafe00000ffffff000080");
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels.first(15)), Result::InvalidInput);
    QCOMPARE(image.writeRgbaPixels(layer, {0, 0, -1, 2}, pixels), Result::InvalidInput);
    QCOMPARE(image.writeRgbaPixels(99, region, pixels), Result::LayerUnavailable);
    QCOMPARE(image.writeRgbaPixels(layer, {0, 0, 0, 0}, {}), Result::Success);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(execution.requested(image.handle()), before);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels), Result::Busy);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels + QByteArrayLiteral("ignored suffix")), Result::Success);
    const auto read = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), region, layer, pixels.size());
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.bytes, pixels);
    QCOMPARE(image.operations().undoCount(), 0);
    QVERIFY(!image.operations().isModified());
    QVERIFY(changed.isEmpty());
    image.close();
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels), Result::Closed);
}

void KisGpuPaintImageTest::rectangularHistoryEditsPreserveIndependentUntrackedPixels()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage definition;
    definition.size = {128, 128};
    definition.layers = {{QStringLiteral("Edited"), true, {},
                          {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}}};
    PaintImage image(execution, definition);
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto blue = QByteArray::fromHex("0000ffff");
    const auto raw = QByteArray::fromHex("120a2700");
    QCOMPARE(image.replaceRgbaPixels(1, {8, 12, 1, 1}, blue), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(image.operations().isModified());
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, raw), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes, blue);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, raw);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes,
             QByteArray::fromHex("ff0000ff"));
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, raw);
    QVERIFY(!image.operations().isModified());
}

void KisGpuPaintImageTest::damagedPixelSharingRecoveryPreservesTheLiveImage()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    QCOMPARE(image.writeRgbaPixels(1, {8, 12, 1, 1}, QByteArray::fromHex("120a2700")), Result::Success);
    const auto before = image.operations().snapshot();
    const auto record = image.operations().recoveryData();
    QVERIFY(!record.isEmpty());
    QBuffer input;
    input.setData(record);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QVERIFY(input.seek(46));
    QDataStream reader(&input);
    reader.setVersion(QDataStream::Qt_5_15);
    reader.setByteOrder(QDataStream::BigEndian);
    quint32 tileCount = 0;
    reader >> tileCount;
    for (quint32 i = 0; i < tileCount; ++i) {
        qint32 x = 0, y = 0;
        quint32 size = 0;
        reader >> x >> y >> size;
        QVERIFY(input.seek(input.pos() + size));
    }
    quint32 operationCount = 0, count = 0;
    quint8 kind = 0;
    quint64 sequence = 0, version = 0, layer = 0;
    reader >> operationCount >> kind >> sequence >> version >> layer >> count;
    QCOMPARE(operationCount, quint32(1));
    QCOMPARE(kind, quint8(1));
    for (quint32 i = 0; i < count; ++i) { quint32 index = 0; reader >> index; }
    const auto groupPosition = input.pos();
    quint64 group = 0;
    quint32 areas = 0;
    reader >> group >> areas;
    QCOMPARE(areas, quint32(1));
    const auto areaPosition = input.pos();
    QSignalSpy changed(&image, &PaintImage::changed);
    for (int damage = 0; damage < 5; ++damage) {
        auto payload = record.first(record.size() - 32);
        QBuffer output(&payload);
        QVERIFY(output.open(QIODevice::ReadWrite));
        QVERIFY(output.seek(damage == 0 ? groupPosition : damage == 4 ? groupPosition + 8 : areaPosition));
        QDataStream writer(&output);
        writer.setVersion(QDataStream::Qt_5_15);
        writer.setByteOrder(QDataStream::BigEndian);
        if (damage == 0) writer << quint64(sequence + 1);
        if (damage == 1) writer << qint32(63) << qint32(12) << qint32(2) << qint32(1);
        if (damage == 2) writer << std::numeric_limits<qint32>::max() << std::numeric_limits<qint32>::max()
                                << std::numeric_limits<qint32>::max() << std::numeric_limits<qint32>::max();
        if (damage == 3) writer << qint32(256) << qint32(0) << qint32(1) << qint32(1);
        if (damage == 4) writer << std::numeric_limits<quint32>::max();
        output.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(image.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(image.operations().snapshot(), before);
        QCOMPARE(execution.requested(image.handle()), before);
        QCOMPARE(image.operations().recoveryData(), record);
        QVERIFY(changed.isEmpty());
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuPaintImageTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPaintImageTest.moc"
