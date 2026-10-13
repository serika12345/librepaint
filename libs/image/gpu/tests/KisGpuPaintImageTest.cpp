/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>

using namespace Krita::Image::Gpu;

class KisGpuPaintImageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment();
    void recoveryPreservesPendingInputAndRedoOnTheSameAttachment();
    void directPixelsReachGpuWithoutEditingNotificationsOrHistory();
};

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
    QCOMPARE(image.writeTiles(layer, {pixels}), Result::Success);
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
    QCOMPARE(image.writeTiles(layer, {pixels}), Result::Closed);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuPaintImageTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPaintImageTest.moc"
