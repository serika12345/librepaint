/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <QCryptographicHash>
#include <QBuffer>
#include <QDataStream>
#include <limits>
#include <stdexcept>

#include <gpu/KisGpuOperationDocument.h>

using namespace LibrePaint::Image::Gpu;

namespace {
InputSample sample(qreal x, qreal y, qreal pressure = 1, quint64 time = 0)
{
    return {{x, y}, pressure, 0, 0, 0, time};
}

OperationId paint(OperationDocument &document, const QList<InputSample> &samples)
{
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 42);
    if (id.isNull() || document.appendSamples(id, samples) != Result::Success ||
        document.commitStroke(id) != Result::Success) {
        return {};
    }
    return id;
}

RasterImage rasterImage()
{
    RasterImage image;
    image.size = {320, 180};
    image.activeLayer = 1;
    image.layers = {
        {QStringLiteral("Red"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")),
         {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Hidden blue"), false, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")),
         {{{1, -1}, QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4)}}}
    };
    image.layers[0].defaultPixel = {12, 34, 56, 78};
    return image;
}
}

class KisGpuOperationDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void directLayerPropertiesPreserveOtherPropertyHistoryAndFixedVersions();
    void propertyRecoveryPreservesRecordedValuesAndRejectsDamagedChanges();
    void failedHistoryMovesPreserveEveryVersionAndCounter();
    void layerOpacityAndLockShareHistoryAndRecovery();
    void inputChunksRetainTheSameOperation();
    void shapePreviewCommitsAsOneHistoryEntry();
    void cancelAndRejectedInputPreserveRedo();
    void documentChangesShareOneHistory();
    void savesPinTheirVersionWhileEditingContinues();
    void historyRetains250Operations();
    void staleAndForeignEditsCannotChangeADocument();
    void frozenOperationsOutliveTheDocument();
    void limitsRejectWholeRequests();
    void closedDocumentPreservesSnapshots();
    void cancelledPreviewsCannotBecomeSavedVersions();
    void emptyAndInvalidRequestsLeaveHistoryUnchanged();
    void recoveryPreservesHistoryAndUncommittedIntent();
    void invalidRecoveryLeavesTheLiveDocumentUntouched();
    void checkedRecoveryRejectsInvalidCountsAndValues();
    void importedPixelsPreserveLayersAndStartWithoutUndo();
    void tileReplacementSharesHistoryAndRejectsWholeRequests();
    void rasterRecoveryPreservesPixelsIdentityAndPendingInput();
    void damagedRasterRecoveryPreservesTheLiveDocument();
    void layerStructureSharesPaintingHistoryAndPreservesPins();
    void rejectedLayerChangesKeepRedoAndPendingInput();
    void removingTheLastLayerPreservesAnEmptyRecoverableDocument();
    void layerRemovalReusesCapacityWithoutLosingRecoveryHistory_data();
    void layerRemovalReusesCapacityWithoutLosingRecoveryHistory();
};

void KisGpuOperationDocumentTest::directLayerPropertiesPreserveOtherPropertyHistoryAndFixedVersions()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(first, 128), Result::Success);
    const auto second = document.addLayer(QStringLiteral("Other"));
    QVERIFY(second);
    const auto pinned = document.snapshot();
    QCOMPARE(document.writeLayerName(first, {}), Result::Success);
    QCOMPARE(document.writeLayerVisible(first, false), Result::Success);
    QCOMPARE(document.writeLayerLocked(first, true), Result::Success);
    QCOMPARE(document.writeLayerPinnedToTimeline(first, true), Result::Success);
    QCOMPARE(document.writeLayerOpacity(second, 64), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(pinned->layers.first().name, QStringLiteral("Paint Layer 1"));
    QVERIFY(pinned->layers.first().visible);
    QVERIFY(!pinned->layers.first().userLocked);
    QVERIFY(!pinned->layers.first().pinnedToTimeline);
    QCOMPARE(pinned->layers.last().opacity, quint8(255));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(255));
    QCOMPARE(document.snapshot()->layers.first().name, QString());
    QVERIFY(!document.snapshot()->layers.first().visible);
    QVERIFY(document.snapshot()->layers.first().userLocked);
    QVERIFY(document.snapshot()->layers.first().pinnedToTimeline);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.last().opacity, quint8(64));
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().name, QString());
    QVERIFY(!restored.snapshot()->layers.first().visible);
    QVERIFY(restored.snapshot()->layers.first().userLocked);
    QVERIFY(restored.snapshot()->layers.first().pinnedToTimeline);
    QCOMPARE(restored.snapshot()->layers.last().opacity, quint8(64));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(255));
}

void KisGpuOperationDocumentTest::propertyRecoveryPreservesRecordedValuesAndRejectsDamagedChanges()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(layer, 128), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 64), Result::Success);
    const auto bytes = document.recoveryData();
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(255));
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(128));
    QByteArray record;
    QDataStream encoded(&record, QIODevice::WriteOnly);
    encoded.setByteOrder(QDataStream::BigEndian);
    encoded << quint8(2) << layer << quint8(255) << quint8(128);
    const auto payload = bytes.first(bytes.size() - 32);
    const auto offset = payload.lastIndexOf(record);
    QVERIFY(offset >= 0);
    const auto retained = restored.snapshot();
    const auto retainedHistory = restored.recoveryData();
    for (int damage = 0; damage < 3; ++damage) {
        auto invalid = payload;
        if (damage == 0) invalid[offset] = char(255);
        if (damage == 1) invalid[offset + 8] = char(2);
        if (damage == 2) invalid[offset + 10] = char(255);
        invalid.append(QCryptographicHash::hash(invalid, QCryptographicHash::Sha256));
        QCOMPARE(restored.restoreRecovery(invalid), Result::InvalidRecovery);
        QCOMPARE(restored.snapshot(), retained);
        QCOMPARE(restored.recoveryData(), retainedHistory);
    }
}

void KisGpuOperationDocumentTest::failedHistoryMovesPreserveEveryVersionAndCounter()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(layer, 128), Result::Success);
    QCOMPARE(document.setLayerOpacity(layer, 32), Result::Success);
    QCOMPARE(document.setHistoryIndex(0), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 16), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 64), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    auto bytes = document.recoveryData();
    bytes.chop(32);
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    QVERIFY(buffer.seek(14)); // LPOR dimensions precede the next content revision.
    QDataStream encoded(&buffer);
    encoded.setByteOrder(QDataStream::BigEndian);
    encoded << quint64(std::numeric_limits<quint64>::max() - 1);
    buffer.close();
    bytes.append(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    const auto retained = restored.snapshot();
    const auto history = restored.recoveryData();
    QCOMPARE(restored.setHistoryIndex(0), Result::CapacityExceeded);
    QCOMPARE(restored.undoCount(), 2);
    QCOMPARE(restored.snapshot(), retained);
    QCOMPARE(restored.recoveryData(), history);
}

void KisGpuOperationDocumentTest::layerOpacityAndLockShareHistoryAndRecovery()
{
    auto source = rasterImage();
    source.activeLayer = 0;
    source.layers[0].opacity = 128;
    source.layers[0].userLocked = true;
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
    QVERIFY(document.snapshot()->layers.first().userLocked);
    QVERIFY(document.beginStroke(layer, Brush{}, 1).isNull());
    QCOMPARE(document.undoCount(), 0);
    QVERIFY(!document.isModified());
    QCOMPARE(document.setLayerOpacity(layer, 64), Result::Success);
    QCOMPARE(document.setLayerLocked(layer, false), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    const auto committed = document.snapshot();
    const auto stroke = document.beginStroke(layer, Brush{}, 1);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.setLayerOpacity(layer, 32), Result::EditInProgress);
    QCOMPARE(document.setLayerLocked(layer, true), Result::EditInProgress);
    QCOMPARE(document.snapshot(), committed);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.first().userLocked);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undoCount(), 2);
    QCOMPARE(recovered.snapshot()->layers.first().opacity, quint8(64));
    QVERIFY(!recovered.snapshot()->layers.first().userLocked);
    QCOMPARE(recovered.undo(), Result::Success);
    QVERIFY(recovered.snapshot()->layers.first().userLocked);
    auto invalid = document.recoveryData();
    invalid.chop(32);
    const auto identity = source.layers[0].uuid.toRfc4122();
    const auto offset = invalid.indexOf(identity);
    QVERIFY(offset >= 0);
    const auto lockOffset = offset + identity.size() + 4 + 1;
    QCOMPARE(invalid[lockOffset], char(1));
    invalid[lockOffset] = char(2);
    invalid.append(QCryptographicHash::hash(invalid, QCryptographicHash::Sha256));
    const auto retained = recovered.snapshot();
    QCOMPARE(recovered.restoreRecovery(invalid), Result::InvalidRecovery);
    QCOMPARE(recovered.snapshot(), retained);
}

void KisGpuOperationDocumentTest::layerRemovalReusesCapacityWithoutLosingRecoveryHistory_data()
{
    QTest::addColumn<bool>("raster");
    QTest::newRow("compressed pixels") << true;
    QTest::newRow("stroke input") << false;
}

void KisGpuOperationDocumentTest::layerRemovalReusesCapacityWithoutLosingRecoveryHistory()
{
    QFETCH(bool, raster);
    Limits limits;
    limits.samplesPerStroke = 1;
    limits.samplesPerDocument = 1;
    limits.rasterBytes = RgbaTile::ByteCount;
    OperationDocument document(QSize(64, 64), limits);
    RgbaTile pixels{{0, 0}, QByteArray(RgbaTile::ByteCount, char(0))};
    for (int cycle = 0; cycle < 3; ++cycle) {
        if (cycle) {
            QCOMPARE(document.removeLayer(document.activeLayer()), Result::Success);
            QVERIFY(document.addLayer(QString::number(cycle)));
        }
        if (raster) {
            pixels.rgba.fill(char(cycle + 1));
            QCOMPARE(document.replaceTiles(document.activeLayer(), {pixels}), Result::Success);
        } else {
            QVERIFY(!paint(document, {sample(8 + cycle, 12)}).isNull());
        }
    }
    const auto bytes = document.recoveryData();
    QVERIFY(!bytes.isEmpty());
    OperationDocument restored(QSize(1, 1), limits);
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    QCOMPARE(restored.undoCount(), document.undoCount());
    if (raster) {
        QCOMPARE(restored.replaceTiles(restored.activeLayer(), {pixels}), Result::CapacityExceeded);
    } else {
        const auto stroke = restored.beginStroke(restored.activeLayer(), Brush{}, 0);
        QCOMPARE(restored.appendSamples(stroke, {sample(20, 20)}), Result::CapacityExceeded);
        QCOMPARE(restored.cancelStroke(stroke), Result::Success);
    }
    for (;;) {
        const auto actual = restored.snapshot();
        const auto expected = document.snapshot();
        QCOMPARE(actual->revision(), expected->revision());
        QCOMPARE(actual->activeLayer, expected->activeLayer);
        QCOMPARE(actual->sampleCount, expected->sampleCount);
        QCOMPARE(actual->rasterBytes, expected->rasterBytes);
        QCOMPARE(actual->layers.size(), expected->layers.size());
        if (!actual->layers.isEmpty()) {
            QCOMPARE(actual->layers[0].uuid, expected->layers[0].uuid);
            QCOMPARE(actual->layers[0].name, expected->layers[0].name);
            if (!actual->layers[0].operations.isEmpty()) {
                if (raster) {
                    QCOMPARE(std::get<TileWriteSnapshot>(actual->layers[0].operations[0])->tiles[0]->rgba(),
                             std::get<TileWriteSnapshot>(expected->layers[0].operations[0])->tiles[0]->rgba());
                } else {
                    QCOMPARE(std::get<PaintSnapshot>(actual->layers[0].operations[0])->inputSamples(),
                             std::get<PaintSnapshot>(expected->layers[0].operations[0])->inputSamples());
                }
            }
        }
        if (!document.undoCount()) break;
        QCOMPARE(restored.undo(), document.undo());
    }
    QCOMPARE(restored.undoCount(), 0);
    QCOMPARE(restored.redoCount(), document.redoCount());
}

void KisGpuOperationDocumentTest::layerStructureSharesPaintingHistoryAndPreservesPins()
{
    OperationDocument document(rasterImage());
    const auto initial = document.snapshot();
    const auto lower = initial->layers[0].id;
    const auto upper = initial->layers[1].id;
    QCOMPARE(document.renameLayer(lower, QStringLiteral("Renamed 下地")), Result::Success);
    const auto renamed = document.snapshot();
    QCOMPARE(renamed->layers[0].name, QStringLiteral("Renamed 下地"));
    QCOMPARE(renamed->layers[0].uuid, initial->layers[0].uuid);
    QCOMPARE(renamed->layers[0].operations, initial->layers[0].operations);
    QCOMPARE(document.moveLayer(lower, 1), Result::Success);
    const auto moved = document.snapshot();
    QCOMPARE(moved->layers[1].id, lower);
    QCOMPARE(document.activeLayer(), upper);
    QCOMPARE(document.removeLayer(upper), Result::Success);
    QCOMPARE(document.activeLayer(), lower);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.snapshot()->rasterBytes, RgbaTile::ByteCount);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), moved);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), renamed);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(initial->layers.size(), 2);
    QCOMPARE(initial->layers[0].name, QStringLiteral("Red"));
}

void KisGpuOperationDocumentTest::rejectedLayerChangesKeepRedoAndPendingInput()
{
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Second"));
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.renameLayer(second, QStringLiteral("Gone")), Result::LayerUnavailable);
    QCOMPARE(document.moveLayer(layer, -1), Result::InvalidInput);
    QCOMPARE(document.moveLayer(layer, 1), Result::InvalidInput);
    QCOMPARE(document.removeLayer(second), Result::LayerUnavailable);
    QCOMPARE(document.renameLayer(layer, QString(65537, QLatin1Char('a'))), Result::InvalidInput);
    QCOMPARE(document.renameLayer(layer, before->layers[0].name), Result::Success);
    QCOMPARE(document.moveLayer(layer, 0), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.redoCount(), 1);
    const auto stroke = document.beginStroke(layer, Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {sample(8, 12)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.renameLayer(layer, QStringLiteral("Busy")), Result::EditInProgress);
    QCOMPARE(document.moveLayer(layer, 0), Result::EditInProgress);
    QCOMPARE(document.removeLayer(layer), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.redoCount(), 1);
    document.close();
    QCOMPARE(document.renameLayer(layer, {}), Result::Closed);
    QCOMPARE(document.moveLayer(layer, 0), Result::Closed);
    QCOMPARE(document.removeLayer(layer), Result::Closed);
}

void KisGpuOperationDocumentTest::removingTheLastLayerPreservesAnEmptyRecoverableDocument()
{
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.removeLayer(layer), Result::Success);
    const auto empty = document.snapshot();
    QVERIFY(empty->layers.isEmpty());
    QCOMPARE(empty->activeLayer, LayerId(0));
    QCOMPARE(empty->sampleCount, 0);
    QCOMPARE(empty->rasterBytes, 0);
    QVERIFY(document.beginStroke(layer, Brush{}, 0).isNull());
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QVERIFY(recovered.snapshot()->layers.isEmpty());
    QCOMPARE(recovered.activeLayer(), LayerId(0));
    QCOMPARE(recovered.undoCount(), 2);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[0].uuid, painted->layers[0].uuid);
    QCOMPARE(recovered.snapshot()->sampleCount, painted->sampleCount);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), empty);
    const auto added = document.addLayer(QStringLiteral("New"));
    QVERIFY(added > layer);
    QCOMPARE(document.activeLayer(), added);
    OperationDocument imported(RasterImage{QSize(64, 64), {}, 0});
    QVERIFY(imported.snapshot()->layers.isEmpty());
    QCOMPARE(imported.activeLayer(), LayerId(0));
    QVERIFY(!imported.isModified());
    QCOMPARE(recovered.restoreRecovery(imported.recoveryData()), Result::Success);
    QVERIFY(recovered.snapshot()->layers.isEmpty());
    QVERIFY(recovered.addLayer(QStringLiteral("First")));
}

void KisGpuOperationDocumentTest::inputChunksRetainTheSameOperation()
{
    OperationDocument one({1024, 1024}), chunks({1024, 1024});
    QList<InputSample> input;
    for (int i = 0; i < 2049; ++i) {
        input.append({QPointF(i % 1024, (i * 7) % 1024), qreal(i % 101) / 100,
                      qreal(i % 91), -qreal(i % 91), qreal(i % 360), quint64(i)});
    }
    const auto a = one.beginStroke(one.activeLayer(), Brush{}, 193);
    const auto b = chunks.beginStroke(chunks.activeLayer(), Brush{}, 193);
    QCOMPARE(one.appendSamples(a, input), Result::Success);
    for (const auto &point : input) {
        QCOMPARE(chunks.appendSamples(b, {point}), Result::Success);
    }
    QCOMPARE(one.commitStroke(a), Result::Success);
    QCOMPARE(chunks.commitStroke(b), Result::Success);
    const auto left = std::get<PaintSnapshot>(one.snapshot()->layers.front().operations.back());
    const auto right = std::get<PaintSnapshot>(chunks.snapshot()->layers.front().operations.back());
    QCOMPARE(left->inputSamples(), right->inputSamples());
    QCOMPARE(left->inputSamples(), input);
    QCOMPARE(left->brush, right->brush);
    QCOMPARE(left->randomSeed, right->randomSeed);
    QCOMPARE(one.undoCount(), 1);
    QCOMPARE(chunks.undoCount(), 1);
}

void KisGpuOperationDocumentTest::shapePreviewCommitsAsOneHistoryEntry()
{
    OperationDocument document({1024, 1024});
    const auto original = document.snapshot();
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 42);
    QCOMPARE(document.appendSamples(id, {sample(1, 2), sample(30, 40)}), Result::Success);
    const auto freehand = document.preview();
    QCOMPARE(document.updateShape(id, {ShapeKind::Line, {1, 2}, {60, 70}, 0}), Result::Success);
    QCOMPARE(document.updateShape(id, {ShapeKind::Ellipse, {1, 2}, {90, 70}, 0.25}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.snapshot(), preview);
    QCOMPARE(document.undoCount(), 1);
    const auto operation = std::get<PaintSnapshot>(preview->layers.front().operations.back());
    QCOMPARE(operation->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(operation->inputSamples().size(), 2);
    QCOMPARE(std::get<PaintSnapshot>(freehand->layers.front().operations.back())->shape.kind, ShapeKind::Freehand);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), preview);
}

void KisGpuOperationDocumentTest::cancelAndRejectedInputPreserveRedo()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(2, 3)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.undo(), Result::Success);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 1), sample(2, 2, -1)}), Result::InvalidInput);
    QCOMPARE(document.preview(), document.snapshot());
    QCOMPARE(document.appendSamples(id, {sample(5, 5)}), Result::Success);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!paint(document, {sample(8, 9)}).isNull());
    QCOMPARE(document.redoCount(), 0);
}

void KisGpuOperationDocumentTest::documentChangesShareOneHistory()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Ink"));
    QVERIFY(second != 0);
    QCOMPARE(document.selectLayer(second), Result::Success);
    QVERIFY(!paint(document, {sample(3, 4)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.setLayerVisible(second, false), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.back().operations.empty());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!document.snapshot()->layers.back().visible);
    QVERIFY(document.beginStroke(second, Brush{}, 0).isNull());
}

void KisGpuOperationDocumentTest::savesPinTheirVersionWhileEditingContinues()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto saving = document.snapshot();
    QVERIFY(!paint(document, {sample(4, 5)}).isNull());
    const auto edited = document.snapshot();
    QCOMPARE(document.markSaved(saving), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(saving->layers.front().operations.size(), 1);
    QCOMPARE(edited->layers.front().operations.size(), 2);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.isModified());
    OperationDocument other({64, 64});
    QCOMPARE(document.markSaved(other.snapshot()), Result::StaleVersion);
    QCOMPARE(document.snapshot(), edited);
}

void KisGpuOperationDocumentTest::historyRetains250Operations()
{
    OperationDocument document({64, 64});
    for (int i = 0; i < 260; ++i) {
        QVERIFY(!paint(document, {sample(i, i)}).isNull());
    }
    QCOMPARE(document.undoCount(), 250);
    const auto recovered = document.recoveryData();
    QVERIFY(!recovered.isEmpty());
    QCOMPARE(document.restoreRecovery(recovered), Result::Success);
    QCOMPARE(document.undoCount(), 250);
    for (int i = 0; i < 250; ++i) {
        QCOMPARE(document.undo(), Result::Success);
    }
    QCOMPARE(document.undo(), Result::NothingToUndo);
    QCOMPARE(document.snapshot()->layers.front().operations.size(), 10);
    for (int i = 0; i < 250; ++i) {
        QCOMPARE(document.redo(), Result::Success);
    }
    QCOMPARE(document.snapshot()->layers.front().operations.size(), 260);
    QCOMPARE(document.redo(), Result::NothingToRedo);
}

void KisGpuOperationDocumentTest::staleAndForeignEditsCannotChangeADocument()
{
    OperationDocument document({64, 64}), other({64, 64});
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(other.appendSamples(id, {sample(1, 2)}), Result::StaleOperation);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    const auto next = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QVERIFY(next != id);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::StaleOperation);
    QCOMPARE(document.commitStroke(id), Result::StaleOperation);
    QCOMPARE(document.undo(), Result::EditInProgress);
    QCOMPARE(document.setLayerVisible(document.activeLayer(), false), Result::EditInProgress);
    QCOMPARE(document.appendSamples(next, {sample(1, 2)}), Result::Success);
    QCOMPARE(document.commitStroke(next), Result::Success);
}

void KisGpuOperationDocumentTest::frozenOperationsOutliveTheDocument()
{
    Snapshot frozen;
    {
        OperationDocument document({64, 64});
        Brush brush;
        brush.rgba = {12, 34, 56, 78};
        const auto id = document.beginStroke(document.activeLayer(), brush, 123);
        brush.rgba = {0, 0, 0, 0};
        QCOMPARE(document.appendSamples(id, {sample(4, 8, 0.5)}), Result::Success);
        frozen = document.preview();
        QCOMPARE(document.commitStroke(id), Result::Success);
    }
    QCOMPARE(std::get<PaintSnapshot>(frozen->layers.front().operations.back())->brush.rgba[0], 12);
    QCOMPARE(std::get<PaintSnapshot>(frozen->layers.front().operations.back())->randomSeed, quint64(123));
    QCOMPARE(std::get<PaintSnapshot>(frozen->layers.front().operations.back())->inputSamples().front().pressure, qreal(0.5));
}

void KisGpuOperationDocumentTest::limitsRejectWholeRequests()
{
    Limits limits;
    limits.samplesPerStroke = 2;
    limits.layerCount = 1;
    OperationDocument document({64, 64}, limits);
    const auto initial = document.snapshot();
    QCOMPARE(document.addLayer(QStringLiteral("over limit")), LayerId(0));
    QCOMPARE(document.snapshot(), initial);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.appendSamples(id, {sample(3, 4), sample(5, 6)}), Result::CapacityExceeded);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.appendSamples(id, {sample(std::numeric_limits<qreal>::infinity(), 0)}), Result::InvalidInput);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.snapshot(), preview);
}

void KisGpuOperationDocumentTest::closedDocumentPreservesSnapshots()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto frozen = document.snapshot();
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(8, 9)}), Result::Success);
    document.close();
    QCOMPARE(document.snapshot(), frozen);
    QCOMPARE(document.appendSamples(id, {sample(10, 11)}), Result::Closed);
    QCOMPARE(document.commitStroke(id), Result::Closed);
    QCOMPARE(document.undo(), Result::Closed);
    QVERIFY(document.beginStroke(document.activeLayer(), Brush{}, 0).isNull());
    QCOMPARE(frozen->layers.front().operations.size(), 1);
}

void KisGpuOperationDocumentTest::cancelledPreviewsCannotBecomeSavedVersions()
{
    OperationDocument document({64, 64});
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::Success);
    const auto abandoned = document.preview();
    QCOMPARE(document.markSaved(abandoned), Result::StaleVersion);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.markSaved(abandoned), Result::StaleVersion);
    QVERIFY(!document.isModified());
}

void KisGpuOperationDocumentTest::emptyAndInvalidRequestsLeaveHistoryUnchanged()
{
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument(QSize(0, 1)));
    OperationDocument document({64, 64});
    const auto original = document.snapshot();
    Brush invalid;
    invalid.diameter = std::numeric_limits<qreal>::quiet_NaN();
    QVERIFY(document.beginStroke(document.activeLayer(), invalid, 0).isNull());
    QVERIFY(document.beginStroke(0, Brush{}, 0).isNull());
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.commitStroke(id), Result::EmptyOperation);
    QCOMPARE(document.appendSamples(id, {}), Result::Success);
    QCOMPARE(document.appendSamples(id, {sample(1, 2, 1, 2)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.appendSamples(id, {sample(4, 5, 1, 1)}), Result::InvalidInput);
    QCOMPARE(document.updateShape(id, {ShapeKind::Ellipse, {1, 1}, {1, 2}, 0}), Result::InvalidInput);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.setLayerVisible(document.activeLayer(), true), Result::Success);
    QCOMPARE(document.selectLayer(document.activeLayer()), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::recoveryPreservesHistoryAndUncommittedIntent()
{
    OperationDocument source({128, 256});
    QVERIFY(!paint(source, {sample(3, 4, 0.5, 1), sample(5, 6, 0.8, 2)}).isNull());
    QCOMPARE(source.markSaved(source.snapshot()), Result::Success);
    const auto layer = source.addLayer(QStringLiteral("線画 🖌"));
    QVERIFY(!paint(source, {sample(7, 8)}).isNull());
    QCOMPARE(source.undo(), Result::Success);
    const auto pending = source.beginStroke(layer, Brush{}, 903);
    QCOMPARE(source.appendSamples(pending, {sample(9, 10), sample(15, 20)}), Result::Success);
    QCOMPARE(source.updateShape(pending, {ShapeKind::Ellipse, {9, 10}, {15, 20}, 0.3}), Result::Success);
    const QByteArray recovery = source.recoveryData();
    QVERIFY(!recovery.isEmpty());

    OperationDocument restored({1, 1});
    const auto previous = restored.beginStroke(restored.activeLayer(), Brush{}, 0);
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    qInfo() << "recovery SHA-256:" << QCryptographicHash::hash(recovery, QCryptographicHash::Sha256).toHex();
    QCOMPARE(restored.snapshot()->size, QSize(128, 256));
    QCOMPARE(restored.undoCount(), source.undoCount());
    QCOMPARE(restored.redoCount(), source.redoCount());
    QCOMPARE(restored.snapshot()->layers.back().name, QStringLiteral("線画 🖌"));
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.back().operations.back())->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.back().operations.back())->randomSeed, quint64(903));
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.back().operations.back())->inputSamples(),
             std::get<PaintSnapshot>(source.preview()->layers.back().operations.back())->inputSamples());
    QCOMPARE(restored.appendSamples(previous, {sample(0, 0)}), Result::StaleOperation);
    QCOMPARE(restored.appendSamples(pending, {sample(0, 0)}), Result::StaleOperation);
    const auto recoveredEdit = restored.activeOperation();
    QVERIFY(!recoveredEdit.isNull());
    QCOMPARE(restored.cancelStroke(recoveredEdit), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(restored.snapshot()->layers.back().operations.back())->inputSamples(), QList<InputSample>{sample(7, 8)});
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(!restored.isModified());
}

void KisGpuOperationDocumentTest::invalidRecoveryLeavesTheLiveDocumentUntouched()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(3, 4)}), Result::Success);
    const auto committed = document.snapshot();
    const auto preview = document.preview();
    const auto good = document.recoveryData();
    QVERIFY(!good.isEmpty());
    QList<QByteArray> damaged{{}, good.first(good.size() - 1), QByteArray(256, char(0xff))};
    auto flipped = good;
    flipped[flipped.size() / 2] ^= 1;
    damaged.append(flipped);
    for (const auto &bytes : damaged) {
        QCOMPARE(document.restoreRecovery(bytes), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), committed);
        QCOMPARE(document.preview(), preview);
        QCOMPARE(document.activeOperation(), id);
    }
    Limits tiny;
    tiny.recoveryBytes = 64;
    OperationDocument constrained({1, 1}, tiny);
    QCOMPARE(constrained.restoreRecovery(good), Result::CapacityExceeded);
    QVERIFY(constrained.recoveryData().isEmpty());
}

void KisGpuOperationDocumentTest::checkedRecoveryRejectsInvalidCountsAndValues()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto original = document.snapshot();
    const auto good = document.recoveryData();
    // The paint-only header ends at byte 54; pressure follows the
    // operation tag, 99-byte paint record and two 64-bit coordinates.
    for (const bool invalidCount : {true, false}) {
        auto payload = good.first(good.size() - 32);
        QBuffer buffer(&payload);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        QVERIFY(buffer.seek(invalidCount ? 50 : 170));
        QDataStream stream(&buffer);
        stream.setVersion(QDataStream::Qt_5_15);
        stream.setByteOrder(QDataStream::BigEndian);
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        if (invalidCount) stream << std::numeric_limits<quint32>::max();
        else stream << std::numeric_limits<qreal>::quiet_NaN();
        buffer.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(document.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), original);
    }
}

void KisGpuOperationDocumentTest::importedPixelsPreserveLayersAndStartWithoutUndo()
{
    auto input = rasterImage();
    OperationDocument document(input);
    const auto frozen = document.snapshot();
    QCOMPARE(frozen->size, input.size);
    QCOMPARE(frozen->layers.size(), 2);
    QCOMPARE(document.activeLayer(), LayerId(2));
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), 0);
    QVERIFY(!document.isModified());
    QCOMPARE(frozen->rasterBytes, qsizetype(2 * RgbaTile::ByteCount));
    QCOMPARE(frozen->layers[0].uuid, input.layers[0].uuid);
    QCOMPARE(frozen->layers[0].defaultPixel, input.layers[0].defaultPixel);
    QCOMPARE(frozen->layers[1].uuid, input.layers[1].uuid);
    QVERIFY(!frozen->layers[1].visible);
    QCOMPARE(frozen->layers[1].name, input.layers[1].name);
    const auto checkpoint = std::get<TileWriteSnapshot>(frozen->layers[1].operations.first());
    QCOMPARE(checkpoint->tiles.first()->coordinate(), QPoint(1, -1));
    QCOMPARE(checkpoint->tiles.first()->rgba(), input.layers[1].tiles.first().rgba);
    input.layers[1].tiles[0].rgba.fill('\0');
    QCOMPARE(checkpoint->tiles.first()->rgba(), QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4));

    for (int invalid = 0; invalid < 5; ++invalid) {
        auto image = rasterImage();
        if (invalid == 0) image.layers.clear();
        if (invalid == 1) image.activeLayer = 2;
        if (invalid == 2) image.layers[1].uuid = image.layers[0].uuid;
        if (invalid == 3) image.layers[0].tiles.append(image.layers[0].tiles.first());
        if (invalid == 4) image.layers[0].name = QString(QChar(0xd800));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument{image});
    }
    QCOMPARE(document.addLayer(QString(QChar(0xd800))), LayerId(0));
    QCOMPARE(document.snapshot(), frozen);
    const auto added = document.addLayer(QStringLiteral("New"));
    QVERIFY(added);
    QVERIFY(!document.snapshot()->layers.last().uuid.isNull());
    QVERIFY(document.snapshot()->layers.last().uuid != frozen->layers.first().uuid);
}

void KisGpuOperationDocumentTest::tileReplacementSharesHistoryAndRejectsWholeRequests()
{
    Limits limits;
    limits.rasterBytes = 3 * RgbaTile::ByteCount;
    OperationDocument document({512, 64}, limits);
    const auto layer = document.activeLayer();
    const RgbaTile red{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(layer, {red}), Result::Success);
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    const auto original = document.snapshot();
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.replaceTiles(layer, {red, red}), Result::InvalidInput);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, QByteArrayLiteral("short")}}), Result::InvalidInput);
    QCOMPARE(document.replaceTiles(layer, {}), Result::EmptyOperation);
    QCOMPARE(document.replaceTiles(999, {red}), Result::LayerUnavailable);
    QCOMPARE(document.replaceTiles(layer, {red, {{1, 0}, red.rgba}, {{2, 0}, red.rgba}}), Result::CapacityExceeded);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redoCount(), 1);
    const auto stroke = document.beginStroke(layer, Brush{}, 0);
    QCOMPARE(document.replaceTiles(layer, {red}), Result::EditInProgress);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    auto blue = red;
    blue.rgba = QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {blue}), Result::Success);
    const auto replaced = document.snapshot();
    blue.rgba.fill('\0');
    QCOMPARE(std::get<TileWriteSnapshot>(replaced->layers.first().operations.last())->tiles.first()->rgba(),
             QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.redoCount(), 0);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), replaced);
    document.close();
    QCOMPARE(document.replaceTiles(layer, {red}), Result::Closed);
}

void KisGpuOperationDocumentTest::rasterRecoveryPreservesPixelsIdentityAndPendingInput()
{
    auto image = rasterImage();
    image.projectionBackground = {17, 34, 51, 128};
    image.layers[0].pinnedToTimeline = true;
    OperationDocument document(image);
    QCOMPARE(document.selectLayer(1), Result::Success);
    const RgbaTile green{{0, 0}, QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(1, {green}), Result::Success);
    const auto replaced = document.snapshot();
    QCOMPARE(document.markSaved(replaced), Result::Success);
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    const auto pending = document.beginStroke(1, Brush{}, 17);
    QCOMPARE(document.appendSamples(pending, {sample(10, 20)}), Result::Success);
    const auto bytes = document.recoveryData();
    QVERIFY(!bytes.isEmpty());
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    QCOMPARE(restored.snapshot()->projectionBackground, image.projectionBackground);
    QVERIFY(restored.snapshot()->layers[0].pinnedToTimeline);
    QCOMPARE(restored.snapshot()->rasterBytes, replaced->rasterBytes);
    QCOMPARE(restored.snapshot()->layers[0].uuid, image.layers[0].uuid);
    QCOMPARE(restored.snapshot()->layers[0].defaultPixel, image.layers[0].defaultPixel);
    QCOMPARE(restored.snapshot()->layers[1].uuid, image.layers[1].uuid);
    const auto initial = std::get<TileWriteSnapshot>(restored.snapshot()->layers[1].operations.first());
    QCOMPARE(initial->tiles.first()->rgba(), image.layers[1].tiles.first().rgba);
    const auto current = std::get<TileWriteSnapshot>(restored.snapshot()->layers[0].operations.last());
    QCOMPARE(current->tiles.first()->rgba(), green.rgba);
    QVERIFY(!restored.isModified());
    QCOMPARE(restored.undoCount(), 1);
    QCOMPARE(restored.redoCount(), 1);
    QCOMPARE(restored.cancelStroke(restored.activeOperation()), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    const auto old = std::get<TileWriteSnapshot>(restored.snapshot()->layers[0].operations.first());
    QCOMPARE(old->tiles.first()->rgba(), image.layers[0].tiles.first().rgba);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->sampleCount, qsizetype(1));
    qInfo() << "raster recovery SHA-256:" << QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

void KisGpuOperationDocumentTest::damagedRasterRecoveryPreservesTheLiveDocument()
{
    OperationDocument source(rasterImage());
    const auto good = source.recoveryData();
    const auto originalPayload = good.first(good.size() - 32);
    QBuffer reader;
    reader.setData(originalPayload);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QVERIFY(reader.seek(50)); // The fixed header includes the tile count.
    QDataStream input(&reader);
    input.setVersion(QDataStream::Qt_5_15);
    input.setByteOrder(QDataStream::BigEndian);
    for (int i = 0; i < 2; ++i) {
        qint32 x = 0, y = 0;
        quint32 size = 0;
        input >> x >> y >> size;
        QVERIFY(reader.seek(reader.pos() + size));
    }
    const auto firstTileReference = reader.pos() + 4 + 25 + 4;
    OperationDocument live({64, 64});
    QVERIFY(!paint(live, {sample(3, 4)}).isNull());
    const auto pending = live.beginStroke(live.activeLayer(), Brush{}, 23);
    QCOMPARE(live.appendSamples(pending, {sample(10, 12)}), Result::Success);
    const auto committed = live.snapshot();
    const auto preview = live.preview();
    const auto recovery = live.recoveryData();
    for (int damage = 0; damage < 4; ++damage) {
        auto payload = originalPayload;
        QBuffer buffer(&payload);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        QDataStream output(&buffer);
        output.setVersion(QDataStream::Qt_5_15);
        output.setByteOrder(QDataStream::BigEndian);
        if (damage == 0) {
            QVERIFY(buffer.seek(58));
            output << quint32(RasterTile::maximumCompressedBytes() + 1);
        } else if (damage == 1) {
            payload[62] ^= char(0xff); // Invalid zlib header with a valid outer digest.
        } else if (damage == 2) {
            QVERIFY(buffer.seek(firstTileReference));
            output << quint32(2); // Out-of-range index in the first tile-write operation.
        } else {
            const auto uuid = rasterImage().layers[1].uuid.toRfc4122();
            const auto offset = payload.indexOf(uuid);
            QVERIFY(offset >= 0);
            payload.replace(offset, uuid.size(), rasterImage().layers[0].uuid.toRfc4122());
        }
        buffer.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(live.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(live.snapshot(), committed);
        QCOMPARE(live.preview(), preview);
        QCOMPARE(live.activeOperation(), pending);
        QCOMPARE(live.recoveryData(), recovery);
    }
    Limits limits;
    limits.rasterBytes = RgbaTile::ByteCount;
    OperationDocument small({64, 64}, limits);
    const auto unchanged = small.snapshot();
    QCOMPARE(small.restoreRecovery(good), Result::CapacityExceeded);
    QCOMPARE(small.snapshot(), unchanged);
}

QTEST_GUILESS_MAIN(KisGpuOperationDocumentTest)
#include "KisGpuOperationDocumentTest.moc"
