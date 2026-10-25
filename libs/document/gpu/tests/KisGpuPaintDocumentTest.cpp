/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuExecution.h>
#include <kra/KisKraRasterCodec.h>
#include <QBuffer>
#include <QColorSpace>
#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDomDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QImage>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <quazip.h>
#include <quazipfile.h>
#include <cstring>
#include <limits>

using namespace LibrePaint::Image::Gpu;
using namespace LibrePaint::Document::Gpu;
namespace Kra = LibrePaint::ImportExport::Kra;

namespace {
// Version 29 record from TestNode::testHierarchyOrderingPreservesPlacementPixelsHistoryAndSaving,
// scenario13, child-first layout, commit 9b7d2f1c83. A 32 x 16 image contains
// Base, Leaf, Inner, Outer and Top; Leaf belongs to Inner, and Inner to Outer.
// SHA-256: 3d8c2aa4e90cd84da557c52cc5484673b6e93e3bf7826ac70512d050a8a5e90b.
inline QByteArray groupedLayerRecoveryV29()
{
    return QByteArray::fromHex(
        "4c504f52001d000000200000001000000000000000000000000000000001000000000000000500000000000000000000"
        "0001000000000000000000000118789cedc101010000080220ff9f2e0fb420200100005e9a0600000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "382c727801ff000000010100000000000000010000000000000000000000000000000200000001000000000000000000"
        "000000000000000000000100000000000000200000001000000000000000000000000000000000000000000000000000"
        "00000100000005000000000000000100000004426173650100000001112233445566778899aabbccff0000ffff000000"
        "000000000000000000000000000000000000000000000000000000000000000000000002000000044c65616601000000"
        "02112233445566778899aabbcc00000000ff000000000000000000000000000003112233445566778899aabbcc000000"
        "0100000000000000000000000300000005496e6e65720100000003112233445566778899aabbcc00000000ff00000000"
        "0000000000000100000004112233445566778899aabbcc000000000000000000000004000000054f7574657201000000"
        "04112233445566778899aabbcc00000000ff000000000000000000000100000000000000000000000000000000000000"
        "00000000000000000500000003546f700100000005112233445566778899aabbcc00000000ff00000000000000000000"
        "00000000000000000000000000000000000000000000ffffffff0000000000000000709dc9918a97f816a079f64e424d"
        "6b95626d32d56603d34602c4be64097b0f12"
    );
}

void dot(PaintImage &image, QPointF position, std::array<quint8, 4> color)
{
    Brush brush;
    brush.rgba = color;
    brush.diameter = 6;
    const auto id = image.beginStroke(image.operations().activeLayer(), brush, 0);
    QVERIFY(!id.isNull());
    QCOMPARE(image.appendSamples(id, {{position, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(id), Result::Success);
}
QByteArray pixel(const PixelReadbackOutcome &value, LayerId layer, QPoint point)
{
    for (const auto &tile : value.tiles) {
        const auto local = point - tile.coordinate * 256;
        if (tile.layer == layer && QRect(0, 0, 256, 256).contains(local)) return tile.rgba.mid((local.y() * 256 + local.x()) * 4, 4);
    }
    return {};
}
QByteArray fileBytes(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}
QImage mergedImage(const QString &path)
{
    QuaZip archive(path);
    if (!archive.open(QuaZip::mdUnzip) || !archive.setCurrentFile(QStringLiteral("mergedimage.png"))) return {};
    QuaZipFile file(&archive);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QImage::fromData(file.readAll());
}
}

class KisGpuPaintDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void groupedFilesRetainSubtreeEditsPinnedSavingAndProjection_data();
    void groupedFilesRetainSubtreeEditsPinnedSavingAndProjection();
    void fixedEditsSaveReopenAndExportIdenticalResults();
    void colorFillSavesReopensAndContinuesIdentically_data();
    void colorFillSavesReopensAndContinuesIdentically();
    void curvedPathFillsSaveAndReopenAcrossInteriorTiles();
    void rectangularSelectionSavesReopensAndLimitsPainting();
    void invertColorsSavesReopensAndRecoversPixels();
    void grayscaleSavesReopensAndRecoversPixels();
    void hsvAdjustmentSavesReopensAndRecoversPixels();
    void freehandSelectionMaskClipsPaintFillEraseAndRoundTripsKra();
    void featheredSelectionClipsPaintAndRoundTripsKra();
    void patternFillSavesReopensAndPreservesSelectionPixels();
    void transparencyMaskComposesSavesReopensAndContinues();
    void transparencyMaskAcceptsDirectBrushEraseFillAndRoundTrips();
    void weightedSmoothingSaveReopenAndContinuePreservesPixels();
    void gaussianLineEditsSaveReopenAndContinueAtTheirNewPosition();
    void sharedExecutionOpenReturnsValidatedMetadataAndKeepsRetainedImagesAlive();
    void acceptedChangesUpdateOneAttachedDocument();
    void pinnedSavePreservesLaterEditsAndHistory();
    void rasterImportAndSaveRetainOffCanvasHiddenPixelsAndMetadata();
    void rejectedSavesPreserveTheFileIdentityAndHistory();
    void rejectedOpenPreservesAttachedDocuments();
    void notificationsAllowSynchronousCloseAndDeletion();
    void recoveryKeepsHistoryPendingInputAndAssociatedFiles();
    void legacyRecoveryRetainsFileColorLabels();
    void invalidRecoveryPreservesTheAttachedDocument();
    void layerStructureAndEmptyDocumentsRoundTripWithUnifiedHistory();
    void savingStreamsPixelsWithinTheGpuAndTransferBudgets();
    void sharedImageEditsAndRecoveryPublishCompleteFileMetadata();
    void adoptedImagesExportWithoutChangingSaveIdentity();
    void savedStateNotificationsAllowDocumentDeletion();
    void completedFormatWritesAdoptPinnedIdentityAndRecoverIt();
private:
    DevicePtr m_device;
};

void KisGpuPaintDocumentTest::groupedFilesRetainSubtreeEditsPinnedSavingAndProjection_data()
{
    QTest::addColumn<bool>("nested");
    QTest::addColumn<bool>("passThrough");
    QTest::addColumn<int>("blendMode");
    for (int mode : {0, 1, 2, 3, 4}) for (bool nested : {false, true}) for (bool passThrough : {false, true}) {
        const QByteArray tag = QByteArray(nested ? "nested" : "group") + (passThrough ? "-pass-through" : "") +
            (mode == 1 ? "-multiply" : mode == 2 ? "-screen" : mode == 3 ? "-darken" : mode == 4 ? "-lighten" : "");
        QTest::newRow(tag.constData()) << nested << passThrough << mode;
    }
}

void KisGpuPaintDocumentTest::groupedFilesRetainSubtreeEditsPinnedSavingAndProjection()
{
    QFETCH(bool, nested);
    QFETCH(bool, passThrough);
    QFETCH(int, blendMode);
    const bool blended = blendMode != 0;
    Execution execution(m_device, 16 * TileRenderer::TileBytes, 8, 2 * TileRenderer::TileBytes);
    RasterImage input;
    input.size = {64, 48};
    input.activeLayer = 1;
    input.layers = {
        {QStringLiteral("Backdrop"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}, {0, 0, 255, 255}},
        {QStringLiteral("Child"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")),
         {{{-1, 0}, QByteArray::fromHex("11223300").repeated(RgbaTile::ByteCount / 4)}}, {255, 0, 0, 255}},
        {QStringLiteral("Group"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}}};
    input.layers[1].parentUuid = input.layers[2].uuid;
    input.layers[2].kind = LayerKind::Group;
    input.layers[2].passThrough = passThrough;
    input.layers[2].opacity = 128;
    if (nested) {
        input.layers.append({QStringLiteral("Outer"), true, QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}")), {}});
        input.layers[3].kind = LayerKind::Group;
        input.layers[3].passThrough = passThrough;
        input.layers[3].opacity = 128;
        input.layers[2].parentUuid = input.layers[3].uuid;
    }
    if (blended) {
        input.layers[0].defaultPixel = {37, 89, 151, 255};
        input.layers[2].blendMode = LayerBlendMode(blendMode);
        if (nested) input.layers[3].blendMode = LayerBlendMode(blendMode);
        if (passThrough) input.layers[1].blendMode = LayerBlendMode(blendMode);
    }
    PaintDocument document(execution, input);
    const auto image = document.image();
    QCOMPARE(image->replaceRgbaPixels(2, {12, 9, 1, 1}, QByteArray::fromHex("00ff00ff")), Result::Success);
    const auto fixed = image->operations().snapshot();
    const auto expected = execution.readProjectionImage(image->handle(), fixed, 64 * 48 * 4);
    QVERIFY2(expected.code == ExecutionCode::Success, qPrintable(expected.diagnostic));
    QCOMPARE(expected.image.pixelColor(12, 9), (blendMode == 2 || blendMode == 4) ? (nested ? QColor(37, 131, 151, 255) : QColor(37, 172, 151, 255)) :
             blended ? (nested ? QColor(28, 89, 113, 255) : QColor(18, 89, 75, 255)) :
             nested ? QColor(0, 64, 191, 255) : QColor(0, 128, 127, 255));
    QCOMPARE(image->setLayerOpacity(3, 64), Result::Success);
    if (blended) QCOMPARE(image->setLayerBlendMode(3, LayerBlendMode::Normal), Result::Success);
    const auto current = image->operations().snapshot();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Hierarchy save artifacts retained at" << directory.path();
        }
    });
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const QString stem = QStringLiteral("hierarchy-%1%2").arg(nested ? QStringLiteral("nested") : QStringLiteral("group"))
        .arg(passThrough ? QStringLiteral("-pass-through") : QString()) + (blendMode == 1 ? QStringLiteral("-multiply") : blendMode == 2 ? QStringLiteral("-screen") :
        blendMode == 3 ? QStringLiteral("-darken") : blendMode == 4 ? QStringLiteral("-lighten") : QString());
    const auto path = QDir(destination).filePath(stem + QStringLiteral(".kra"));
    const auto saved = document.saveKra(path, fixed);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QCOMPARE(image->operations().snapshot(), current);
    QCOMPARE(image->operations().undoCount(), 2 + int(blended));
    QCOMPARE(image->operations().savedHistoryIndex(), 1);
    QVERIFY(image->operations().isModified());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto decoded = Kra::RasterCodec::read(file);
    QVERIFY2(decoded.code == Kra::Status::Success, qPrintable(decoded.diagnostic));
    QCOMPARE(decoded.document.image.layers.size(), input.layers.size());
    for (int i = 0; i < input.layers.size(); ++i) {
        const auto &actual = decoded.document.image.layers[i];
        QCOMPARE(actual.uuid, input.layers[i].uuid);
        QCOMPARE(actual.parentUuid, input.layers[i].parentUuid);
        QCOMPARE(actual.kind, input.layers[i].kind);
        QCOMPARE(actual.passThrough, input.layers[i].passThrough);
        QCOMPARE(actual.blendMode, input.layers[i].blendMode);
        if (actual.kind == LayerKind::Group) QVERIFY(actual.tiles.isEmpty());
    }
    QCOMPARE(decoded.document.image.layers[decoded.document.image.activeLayer].uuid, input.layers[1].uuid);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto restored = reopened.document->image();
    const auto projected = execution.readProjectionImage(restored->handle(), restored->operations().snapshot(), 64 * 48 * 4);
    QCOMPARE(projected.code, ExecutionCode::Success);
    QCOMPARE(projected.image, expected.image);
    auto storedProjection = expected.image;
    storedProjection.setColorSpace({});
    QCOMPARE(mergedImage(path).convertToFormat(QImage::Format_RGBA8888), storedProjection);
    QCOMPARE(restored->readRgbaPixels(2, {-5, 7, 1, 1}, 4).bytes, QByteArray::fromHex("11223300"));
    QCOMPARE(restored->replaceRgbaPixels(2, {12, 9, 1, 1}, QByteArray::fromHex("ff0000ff")), Result::Success);
    QCOMPARE(restored->undo(), Result::Success);
    QCOMPARE(restored->readRgbaPixels(2, {12, 9, 1, 1}, 4).bytes, QByteArray::fromHex("00ff00ff"));
    QFile pixels(QDir(destination).filePath(stem + QStringLiteral(".rgba")));
    QVERIFY(pixels.open(QIODevice::WriteOnly));
    for (int y = 0; y < expected.image.height(); ++y)
        QCOMPARE(pixels.write(reinterpret_cast<const char *>(expected.image.constScanLine(y)), 64 * 4), qint64(64 * 4));
}

void KisGpuPaintDocumentTest::curvedPathFillsSaveAndReopenAcrossInteriorTiles()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {768, 768};
    initial.layers.append({QStringLiteral("Curved path"), true,
                            QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000021}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::CubicTo,
                           PathCommand::LineTo, PathCommand::CubicTo, PathCommand::MoveTo,
                           PathCommand::LineTo, PathCommand::LineTo, PathCommand::LineTo, PathCommand::LineTo};
    const QList<InputSample> samples{{{32, 32}, 1}, {{736, 32}, 1}, {{752, 32}, 1},
                                    {{752, 736}, 1}, {{736, 736}, 1}, {{32, 736}, 1},
                                    {{16, 736}, 1}, {{16, 32}, 1}, {{32, 32}, 1},
                                    {{256, 256}, 1}, {{512, 256}, 1}, {{512, 512}, 1},
                                    {{256, 512}, 1}, {{256, 256}, 1}};
    FigureStyle style;
    style.outline = false;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(image->paintFigure(layer, Brush{}, samples, shape, style, 42), Result::Success);
    const QRect region(QPoint(), initial.size);
    const auto painted = image->readRgbaPixels(layer, region, 768 * 768 * 4);
    QVERIFY2(painted.code == ExecutionCode::Success, qPrintable(painted.diagnostic));
    QCOMPARE(painted.bytes.mid((384 * 768 + 128) * 4, 4), QByteArray::fromHex("dd112cff"));
    QCOMPARE(painted.bytes.mid((384 * 768 + 384) * 4, 4), QByteArray(4, char(0)));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto path = QDir(destination).filePath(QStringLiteral("path-filled.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    QuaZip archive(path);
    QVERIFY(archive.open(QuaZip::mdUnzip));
    QVERIFY(archive.setCurrentFile(QStringLiteral("mergedimage.png")));
    QuaZipFile merged(&archive);
    QVERIFY(merged.open(QIODevice::ReadOnly));
    const auto png = merged.readAll();
    merged.close();
    archive.close();
    QFile output(QDir(destination).filePath(QStringLiteral("path-filled-merged.png")));
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(output.write(png), qint64(png.size()));
    output.close();
    const auto preview = QImage::fromData(png);
    QVERIFY(!preview.isNull());
    QCOMPARE(preview.pixelColor(128, 384), QColor(221, 17, 44, 255));
    QCOMPARE(preview.pixelColor(384, 384).alpha(), 0);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    const auto actual = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, painted.bytes.size());
    QVERIFY2(actual.code == ExecutionCode::Success, qPrintable(actual.diagnostic));
    QCOMPARE(QCryptographicHash::hash(actual.bytes, QCryptographicHash::Sha256),
             QCryptographicHash::hash(painted.bytes, QCryptographicHash::Sha256));
    QCOMPARE(image->operations().undoCount(), 1);
    QVERIFY(!image->operations().isModified());
}

void KisGpuPaintDocumentTest::rectangularSelectionSavesReopensAndLimitsPainting()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {128, 128};
    initial.layers.append({QStringLiteral("Selection"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000023}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    QCOMPARE(image->setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    Brush brush;
    brush.rgba = {31, 97, 163, 255};
    brush.diameter = 16;
    QCOMPARE(image->paintFigure(layer, brush, {{{16, 64}, 1}, {{112, 64}, 1}}, Shape{}, FigureStyle{}, 9), Result::Success);
    const QRect region(QPoint(), initial.size);
    const auto painted = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(painted.code, ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 128 + x) * 4, 4); };
    QCOMPARE(pixel(painted.bytes, 64, 64), QByteArray::fromHex("1f61a3ff"));
    QCOMPARE(pixel(painted.bytes, 16, 64), QByteArray(4, char(0)));
    QCOMPARE(pixel(painted.bytes, 112, 64), QByteArray(4, char(0)));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto path = QDir(destination).filePath(QStringLiteral("rectangular-selection.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    QCOMPARE(loaded->selectionRect(), QRect(32, 32, 64, 64));
    QCOMPARE(loaded->readRgbaPixels(loaded->operations().activeLayer(), region, painted.bytes.size()).bytes, painted.bytes);

    // A later selection leaves the earlier painted pixels intact on both owners.
    QCOMPARE(image->setSelectionRect(QRect(64, 32, 32, 64)), Result::Success);
    QCOMPARE(loaded->setSelectionRect(QRect(64, 32, 32, 64)), Result::Success);
    QCOMPARE(image->paintFigure(layer, brush, {{{80, 80}, 1}}, Shape{}, FigureStyle{}, 10), Result::Success);
    QCOMPARE(loaded->paintFigure(loaded->operations().activeLayer(), brush, {{{80, 80}, 1}}, Shape{}, FigureStyle{}, 10), Result::Success);
    const auto continued = image->readRgbaPixels(layer, region, painted.bytes.size()).bytes;
    QCOMPARE(loaded->readRgbaPixels(loaded->operations().activeLayer(), region, painted.bytes.size()).bytes, continued);
    QCOMPARE(pixel(continued, 64, 64), QByteArray::fromHex("1f61a3ff"));
    QCOMPARE(pixel(continued, 16, 64), QByteArray(4, char(0)));
    QCOMPARE(pixel(continued, 80, 80), QByteArray::fromHex("1f61a3ff"));
    QCOMPARE(pixel(continued, 64, 112), QByteArray(4, char(0)));

    const auto secondPath = QDir(destination).filePath(QStringLiteral("rectangular-selection-continued.kra"));
    QCOMPARE(reopened.document->saveKra(secondPath).code, FileCode::Success);
    const auto again = PaintDocument::openKra(execution, secondPath);
    QVERIFY2(again.code == FileCode::Success, qPrintable(again.diagnostic));
    QCOMPARE(again.document->image()->selectionRect(), QRect(64, 32, 32, 64));
    QCOMPARE(again.document->image()->readRgbaPixels(again.document->image()->operations().activeLayer(), region,
             continued.size()).bytes, continued);
}

void KisGpuPaintDocumentTest::invertColorsSavesReopensAndRecoversPixels()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {128, 128};
    initial.layers.append({QStringLiteral("Invert"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000041}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    Brush brush;
    brush.rgba = {31, 97, 163, 255};
    brush.opacity = 128;
    brush.diameter = 16;
    QCOMPARE(image->paintFigure(layer, brush, {{{16, 64}, 1}, {{112, 64}, 1}}, Shape{}, FigureStyle{}, 9), Result::Success);
    QCOMPARE(image->setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    QCOMPARE(image->invertColors(layer), Result::Success);
    const auto count = image->operations().undoCount();
    QCOMPARE(count, 3);
    QCOMPARE(image->operations().historyAction(count), HistoryAction::InvertColors);

    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 128 + x) * 4, 4); };
    const auto inverted = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(inverted.code, ExecutionCode::Success);
    QCOMPARE(pixel(inverted.bytes, 64, 64), QByteArray::fromHex("e09e5c80"));
    QCOMPARE(pixel(inverted.bytes, 16, 64), QByteArray::fromHex("1f61a380"));
    // Fully transparent pixels keep alpha zero and retain complemented RGB.
    QCOMPARE(pixel(inverted.bytes, 40, 40), QByteArray::fromHex("ffffff00"));
    QCOMPARE(pixel(inverted.bytes, 100, 20), QByteArray(4, char(0)));

    // One undo step restores the stroke pixels; redo restores the filter result.
    QCOMPARE(image->undo(), Result::Success);
    const auto original = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(pixel(original.bytes, 64, 64), QByteArray::fromHex("1f61a380"));
    QCOMPARE(pixel(original.bytes, 40, 40), QByteArray(4, char(0)));
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, 128 * 128 * 4).bytes, inverted.bytes);

    // Recovery retains the frozen filter selection and its pixels.
    PaintDocument recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recoveredImage = recovered.image();
    QCOMPARE(recoveredImage->operations().undoCount(), count);
    QCOMPARE(recoveredImage->operations().historyAction(count), HistoryAction::InvertColors);
    QCOMPARE(recoveredImage->readRgbaPixels(recoveredImage->operations().activeLayer(), region,
             inverted.bytes.size()).bytes, inverted.bytes);

    // KRA saving reads the filter result; reopening reproduces the same pixels.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto path = QDir(destination).filePath(QStringLiteral("invert-colors.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    QCOMPARE(loaded->operations().undoCount(), 0);
    QCOMPARE(loaded->readRgbaPixels(loaded->operations().activeLayer(), region, inverted.bytes.size()).bytes,
             inverted.bytes);

    // A reopened layer stays editable and a later filter uses the new selection.
    QCOMPARE(loaded->clearSelection(), Result::Success);
    QCOMPARE(loaded->setSelectionRect(QRect(0, 0, 32, 32)), Result::Success);
    QCOMPARE(loaded->invertColors(loaded->operations().activeLayer()), Result::Success);
    QCOMPARE(loaded->operations().historyAction(loaded->operations().undoCount()), HistoryAction::InvertColors);
    const auto continued = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, inverted.bytes.size());
    QCOMPARE(continued.code, ExecutionCode::Success);
    QCOMPARE(pixel(continued.bytes, 64, 64), QByteArray::fromHex("e09e5c80"));
    QCOMPARE(pixel(continued.bytes, 16, 64), QByteArray::fromHex("1f61a380"));
    QCOMPARE(pixel(continued.bytes, 10, 10), QByteArray::fromHex("ffffff00"));
}

void KisGpuPaintDocumentTest::grayscaleSavesReopensAndRecoversPixels()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {256, 256};
    initial.layers.append({QStringLiteral("Grayscale"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000042}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    QByteArray bytes(RgbaTile::ByteCount, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * RgbaTile::Size + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));  // 10,20,30,40
    set(40, 40, QByteArray::fromHex("0a141e00"));  // hidden RGB under alpha 0
    set(50, 50, QByteArray::fromHex("ff8000ff"));  // 255,128,0,255
    set(80, 80, QByteArray::fromHex("808080ff"));  // neutral gray
    set(100, 100, QByteArray::fromHex("11223344"));
    set(120, 120, QByteArray::fromHex("c86432ff"));
    QCOMPARE(image->replaceTiles(layer, {RgbaTile{{0, 0}, bytes}}), Result::Success);
    QCOMPARE(image->setSelectionRect(QRect(0, 0, 64, 64)), Result::Success);
    QCOMPARE(image->grayscale(layer), Result::Success);
    const auto count = image->operations().undoCount();
    QCOMPARE(count, 3);
    QCOMPARE(image->operations().historyAction(count), HistoryAction::Grayscale);

    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &value, int x, int y) { return value.mid((y * 256 + x) * 4, 4); };
    const auto filtered = image->readRgbaPixels(layer, region, 256 * 256 * 4);
    QCOMPARE(filtered.code, ExecutionCode::Success);
    QCOMPARE(pixel(filtered.bytes, 10, 10), QByteArray::fromHex("13131328"));
    // Hidden RGB is filtered and alpha stays zero, matching Invert Colors.
    QCOMPARE(pixel(filtered.bytes, 40, 40), QByteArray::fromHex("13131300"));
    QCOMPARE(pixel(filtered.bytes, 50, 50), QByteArray::fromHex("929292ff"));
    QCOMPARE(pixel(filtered.bytes, 80, 80), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(filtered.bytes, 100, 100), QByteArray::fromHex("11223344"));
    QCOMPARE(pixel(filtered.bytes, 120, 120), QByteArray::fromHex("c86432ff"));

    // One undo step restores the raw tile; redo restores the filter result.
    QCOMPARE(image->undo(), Result::Success);
    const auto original = image->readRgbaPixels(layer, region, 256 * 256 * 4);
    QCOMPARE(original.bytes, bytes);
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, 256 * 256 * 4).bytes, filtered.bytes);

    // Recovery retains the frozen Grayscale kind, its coverage and its pixels.
    PaintDocument recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recoveredImage = recovered.image();
    QCOMPARE(recoveredImage->operations().undoCount(), count);
    QCOMPARE(recoveredImage->operations().historyAction(count), HistoryAction::Grayscale);
    QCOMPARE(recoveredImage->readRgbaPixels(recoveredImage->operations().activeLayer(), region,
             filtered.bytes.size()).bytes, filtered.bytes);

    // KRA saving reads the filter result; reopening reproduces the same pixels.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto path = QDir(destination).filePath(QStringLiteral("grayscale.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    QCOMPARE(loaded->operations().undoCount(), 0);
    QCOMPARE(loaded->readRgbaPixels(loaded->operations().activeLayer(), region, filtered.bytes.size()).bytes,
             filtered.bytes);

    // A reopened layer stays editable and a later filter uses the new selection.
    QCOMPARE(loaded->clearSelection(), Result::Success);
    QCOMPARE(loaded->setSelectionRect(QRect(96, 96, 32, 32)), Result::Success);
    QCOMPARE(loaded->grayscale(loaded->operations().activeLayer()), Result::Success);
    QCOMPARE(loaded->operations().historyAction(loaded->operations().undoCount()), HistoryAction::Grayscale);
    const auto continued = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, filtered.bytes.size());
    QCOMPARE(continued.code, ExecutionCode::Success);
    QCOMPARE(pixel(continued.bytes, 120, 120), QByteArray::fromHex("767676ff"));
    QCOMPARE(pixel(continued.bytes, 10, 10), QByteArray::fromHex("13131328"));
}

void KisGpuPaintDocumentTest::hsvAdjustmentSavesReopensAndRecoversPixels()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {256, 256};
    initial.layers.append({QStringLiteral("HSV"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000043}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    QByteArray bytes(RgbaTile::ByteCount, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * RgbaTile::Size + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));  // 10,20,30,40
    set(40, 40, QByteArray::fromHex("0a141e00"));  // hidden RGB under alpha 0
    set(50, 50, QByteArray::fromHex("c81e46ff"));  // 200,30,70,255
    set(80, 80, QByteArray::fromHex("808080ff"));  // neutral gray
    set(100, 100, QByteArray::fromHex("11223344"));
    set(120, 120, QByteArray::fromHex("646464ff"));
    QCOMPARE(image->replaceTiles(layer, {RgbaTile{{0, 0}, bytes}}), Result::Success);
    QCOMPARE(image->setSelectionRect(QRect(0, 0, 64, 64)), Result::Success);
    QCOMPARE(image->hsvAdjustment(layer, {0, 0, 20, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto count = image->operations().undoCount();
    QCOMPARE(count, 3);
    QCOMPARE(image->operations().historyAction(count), HistoryAction::HsvAdjustment);

    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &value, int x, int y) { return value.mid((y * 256 + x) * 4, 4); };
    const auto filtered = image->readRgbaPixels(layer, region, 256 * 256 * 4);
    QCOMPARE(filtered.code, ExecutionCode::Success);
    QCOMPARE(pixel(filtered.bytes, 10, 10), QByteArray::fromHex("3b434b28"));
    // Hidden RGB is filtered and alpha stays zero, matching the other filters.
    QCOMPARE(pixel(filtered.bytes, 40, 40), QByteArray::fromHex("3b434b00"));
    QCOMPARE(pixel(filtered.bytes, 50, 50), QByteArray::fromHex("d34b6bff"));
    QCOMPARE(pixel(filtered.bytes, 80, 80), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(filtered.bytes, 100, 100), QByteArray::fromHex("11223344"));
    QCOMPARE(pixel(filtered.bytes, 120, 120), QByteArray::fromHex("646464ff"));

    // One undo step restores the raw tile; redo restores the filter result.
    QCOMPARE(image->undo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, 256 * 256 * 4).bytes, bytes);
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, 256 * 256 * 4).bytes, filtered.bytes);

    // Recovery retains the frozen values, mode and selection.
    PaintDocument recovered(execution, QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recoveredImage = recovered.image();
    QCOMPARE(recoveredImage->operations().undoCount(), count);
    QCOMPARE(recoveredImage->operations().historyAction(count), HistoryAction::HsvAdjustment);
    const auto recoveredOperation =
        std::get<FilterSnapshot>(recoveredImage->operations().snapshot()->layers.first().operations.last());
    QCOMPARE(recoveredOperation->hue, 0);
    QCOMPARE(recoveredOperation->saturation, 0);
    QCOMPARE(recoveredOperation->value, 20);
    QCOMPARE(recoveredOperation->hsvType, HsvAdjustmentType::Hsv);
    QVERIFY(!recoveredOperation->compatibilityMode);
    QCOMPARE(recoveredImage->readRgbaPixels(recoveredImage->operations().activeLayer(), region,
             filtered.bytes.size()).bytes, filtered.bytes);

    // KRA saving reads the filter result; reopening reproduces the same pixels.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto path = QDir(destination).filePath(QStringLiteral("hsv-adjustment.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    QCOMPARE(loaded->operations().undoCount(), 0);
    QCOMPARE(loaded->readRgbaPixels(loaded->operations().activeLayer(), region, filtered.bytes.size()).bytes,
             filtered.bytes);

    // A reopened layer stays editable; a later legacy-mode adjustment uses the
    // new selection and the frozen compatibility flag.
    QCOMPARE(loaded->clearSelection(), Result::Success);
    QCOMPARE(loaded->setSelectionRect(QRect(96, 96, 32, 32)), Result::Success);
    QCOMPARE(loaded->hsvAdjustment(loaded->operations().activeLayer(),
                                   {0, 50, 0, HsvAdjustmentType::Hsv, true}), Result::Success);
    QCOMPARE(loaded->operations().historyAction(loaded->operations().undoCount()), HistoryAction::HsvAdjustment);
    const auto continued = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, filtered.bytes.size());
    QCOMPARE(continued.code, ExecutionCode::Success);
    // The legacy saturation rule can color the neutral gray produced by the
    // first adjustment; pixels outside the new selection keep their values.
    QCOMPARE(pixel(continued.bytes, 120, 120), QByteArray::fromHex("643233ff"));
    QCOMPARE(pixel(continued.bytes, 80, 80), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(continued.bytes, 10, 10), QByteArray::fromHex("3b434b28"));
}

void KisGpuPaintDocumentTest::freehandSelectionMaskClipsPaintFillEraseAndRoundTripsKra()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {128, 128};
    initial.layers.append({QStringLiteral("Freehand"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000024}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    const QList<QPointF> polygon{{8, 8}, {120, 8}, {8, 120}};
    const auto mask = SelectionMask::fromPolygon(polygon, QRect(QPoint(), initial.size));
    QVERIFY(mask);
    QCOMPARE(image->setSelectionMask(mask), Result::Success);
    QCOMPARE(image->selectionKind(), SelectionKind::Freehand);
    QCOMPARE(image->selectionRect(), mask->bounds());

    Brush brush;
    brush.rgba = {31, 97, 163, 255};
    brush.diameter = 16;
    QCOMPARE(image->paintFigure(layer, brush, {{{8, 64}, 1}, {{120, 64}, 1}}, Shape{}, FigureStyle{}, 9), Result::Success);
    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 128 + x) * 4, 4); };
    const auto painted = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(painted.code, ExecutionCode::Success);
    QCOMPARE(pixel(painted.bytes, 32, 64), QByteArray::fromHex("1f61a3ff"));
    QCOMPARE(pixel(painted.bytes, 96, 64), QByteArray(4, char(0)));

    // Keep a painted marker outside the mask so erase clipping remains observable.
    QCOMPARE(image->clearSelection(), Result::Success);
    QCOMPARE(image->paintFigure(layer, brush, {{{100, 100}, 1}}, Shape{}, FigureStyle{}, 10), Result::Success);
    QCOMPARE(image->setSelectionMask(mask), Result::Success);
    QCOMPARE(image->fillLayer(layer, {19, 211, 97}, 255, false, false), Result::Success);
    const auto filled = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(filled.code, ExecutionCode::Success);
    QCOMPARE(pixel(filled.bytes, 24, 24), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(filled.bytes, 100, 100), QByteArray::fromHex("1f61a3ff"));
    QCOMPARE(pixel(filled.bytes, 110, 110), QByteArray(4, char(0)));

    QCOMPARE(image->fillLayer(layer, {0, 0, 0}, 255, true, false), Result::Success);
    const auto erased = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(erased.code, ExecutionCode::Success);
    QCOMPARE(quint8(pixel(erased.bytes, 24, 24).at(3)), 0);
    QCOMPARE(pixel(erased.bytes, 100, 100), QByteArray::fromHex("1f61a3ff"));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("freehand-selection.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    QCOMPARE(loaded->selectionKind(), SelectionKind::Freehand);
    QCOMPARE(loaded->selectionRect(), mask->bounds());
    QVERIFY(loaded->selectionMask());
    QVERIFY(*loaded->selectionMask() == *mask);
    const auto loadedPixels = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, 128 * 128 * 4);
    QCOMPARE(loadedPixels.code, ExecutionCode::Success);
    QCOMPARE(loadedPixels.bytes, erased.bytes);

    QCOMPARE(loaded->setSelectionMask(loaded->selectionMask(), true), Result::Success);
    QVERIFY(loaded->selectionInverted());
    QCOMPARE(loaded->fillLayer(loaded->operations().activeLayer(), {211, 19, 97}, 255, false, false), Result::Success);
    const auto inverted = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, 128 * 128 * 4);
    QCOMPARE(inverted.code, ExecutionCode::Success);
    QCOMPARE(quint8(pixel(inverted.bytes, 24, 24).at(3)), 0);
    QCOMPARE(pixel(inverted.bytes, 100, 100), QByteArray::fromHex("d31361ff"));
    QCOMPARE(pixel(inverted.bytes, 8, 120), QByteArray::fromHex("d31361ff"));

    // The KRA stream stores coverage, so an inverted freehand selection returns
    // as the equivalent complement mask rather than a shape plus a flag.
    const auto invertedPath = directory.filePath(QStringLiteral("inverted-freehand-selection.kra"));
    QCOMPARE(reopened.document->saveKra(invertedPath).code, FileCode::Success);
    const auto reopenedInverted = PaintDocument::openKra(execution, invertedPath);
    QVERIFY2(reopenedInverted.code == FileCode::Success, qPrintable(reopenedInverted.diagnostic));
    const auto loadedInverted = reopenedInverted.document->image();
    QCOMPARE(loadedInverted->selectionKind(), SelectionKind::Freehand);
    QVERIFY(!loadedInverted->selectionInverted());
    QVERIFY(loadedInverted->selectionMask());
    QVERIFY(!loadedInverted->selectionMask()->contains(24, 24));
    QVERIFY(loadedInverted->selectionMask()->contains(100, 100));
    const auto invertedPixels = loadedInverted->readRgbaPixels(loadedInverted->operations().activeLayer(), region,
                                                               128 * 128 * 4);
    QCOMPARE(invertedPixels.code, ExecutionCode::Success);
    QCOMPARE(invertedPixels.bytes, inverted.bytes);
}

void KisGpuPaintDocumentTest::patternFillSavesReopensAndPreservesSelectionPixels()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {512, 256};
    initial.layers.append({QStringLiteral("Pattern"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000028}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    const auto patternPixel = [](int x, int y) {
        const int px = ((x % 4) + 4) % 4;
        const int py = ((y % 4) + 4) % 4;
        QByteArray value(4, '\0');
        value[0] = char(px * 60 + 3);
        value[1] = char(py * 60 + 5);
        value[2] = char((px + py) * 20 + 7);
        value[3] = char(255);
        return value;
    };
    PatternFill pattern;
    pattern.size = QSize(4, 4);
    pattern.rgba = QByteArray(4 * 4 * 4, '\0');
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            const auto value = patternPixel(x, y);
            std::memcpy(pattern.rgba.data() + (y * 4 + x) * 4, value.constData(), 4);
        }
    }

    const QRect region(QPoint(), initial.size);
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * 512 + x) * 4, 4);
    };
    QCOMPARE(image->setSelectionRect(QRect(100, 60, 300, 100)), Result::Success);
    QCOMPARE(image->fillLayerPattern(layer, pattern), Result::Success);
    QCOMPARE(image->operations().undoCount(), 2);
    QCOMPARE(image->operations().historyAction(2), HistoryAction::FillLayer);
    const auto filled = image->readRgbaPixels(layer, region, 512 * 256 * 4);
    QVERIFY2(filled.code == ExecutionCode::Success, qPrintable(filled.diagnostic));
    QCOMPARE(pixel(filled.bytes, 99, 60), QByteArray(4, '\0'));
    QCOMPARE(pixel(filled.bytes, 100, 60), patternPixel(100, 60));
    QCOMPARE(pixel(filled.bytes, 255, 159), patternPixel(255, 159));
    QCOMPARE(pixel(filled.bytes, 256, 60), patternPixel(256, 60));
    QCOMPARE(pixel(filled.bytes, 399, 159), patternPixel(399, 159));
    QCOMPARE(pixel(filled.bytes, 400, 60), QByteArray(4, '\0'));

    QCOMPARE(image->undo(), Result::Success);
    const auto undone = image->readRgbaPixels(layer, region, 512 * 256 * 4);
    QCOMPARE(pixel(undone.bytes, 100, 60), QByteArray(4, '\0'));
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, 512 * 256 * 4).bytes, filled.bytes);

    // KRA stores the resulting pixels; the frozen pattern resource is not a
    // document property.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("pattern-fill.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    const auto loadedPixels = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, 512 * 256 * 4);
    QVERIFY2(loadedPixels.code == ExecutionCode::Success, qPrintable(loadedPixels.diagnostic));
    QCOMPARE(loadedPixels.bytes, filled.bytes);
    QCOMPARE(pixel(loadedPixels.bytes, 256, 60), patternPixel(256, 60));
    QCOMPARE(pixel(loadedPixels.bytes, 400, 60), QByteArray(4, '\0'));

    // Pattern fill through a retained transparency mask uses the same frozen
    // coverage and remains one history entry.
    RasterImage maskInitial;
    maskInitial.size = {128, 128};
    maskInitial.layers.append({QStringLiteral("Mask pattern"), true,
                               QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000029}")), {}});
    PaintDocument maskDocument(execution, maskInitial);
    const auto maskImage = maskDocument.image();
    const auto maskLayer = maskImage->operations().activeLayer();
    QCOMPARE(maskImage->fillLayer(maskLayer, {19, 211, 97}), Result::Success);
    QCOMPARE(maskImage->setSelectionRect(QRect(16, 16, 32, 32)), Result::Success);
    QCOMPARE(maskImage->addTransparencyMaskFromSelection(maskLayer), Result::Success);
    QCOMPARE(maskImage->clearSelection(), Result::Success);
    QCOMPARE(maskImage->setSelectionRect(QRect(64, 64, 16, 16)), Result::Success);
    PatternFill opaque;
    opaque.size = QSize(2, 2);
    opaque.rgba = QByteArray(2 * 2 * 4, '\0');
    for (qsizetype i = 0; i < opaque.rgba.size(); i += 4) {
        opaque.rgba[i + 0] = char(10);
        opaque.rgba[i + 1] = char(20);
        opaque.rgba[i + 2] = char(30);
        opaque.rgba[i + 3] = char(255);
    }
    const auto undoBefore = maskImage->operations().undoCount();
    QCOMPARE(maskImage->fillTransparencyMaskPattern(maskLayer, opaque), Result::Success);
    QCOMPARE(maskImage->operations().undoCount(), undoBefore + 1);
    QCOMPARE(maskImage->operations().historyAction(maskImage->operations().undoCount()), HistoryAction::FillLayer);
    const auto mask = maskImage->layerTransparencyMask(maskLayer);
    QVERIFY(mask);
    QCOMPARE(mask->coverage(68, 68), quint8(255));
    QCOMPARE(mask->coverage(24, 24), quint8(255));
    QCOMPARE(mask->coverage(0, 0), quint8(0));
    QCOMPARE(maskImage->undo(), Result::Success);
    QCOMPARE(maskImage->layerTransparencyMask(maskLayer)->coverage(68, 68), quint8(0));
    QCOMPARE(maskImage->redo(), Result::Success);
    QCOMPARE(maskImage->layerTransparencyMask(maskLayer)->coverage(68, 68), quint8(255));
}

void KisGpuPaintDocumentTest::featheredSelectionClipsPaintAndRoundTripsKra()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {128, 128};
    initial.layers.append({QStringLiteral("Feather"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000025}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    const QRect clip(QPoint(), initial.size);
    QByteArray coverage(128 * 128, '\0');
    for (int y = 32; y < 96; ++y)
        for (int x = 32; x < 96; ++x) coverage[y * 128 + x] = char(255);
    const auto hard = SelectionMask::fromCoverage(QRect(QPoint(), initial.size), coverage, clip);
    QVERIFY(hard);
    const auto soft = hard->feathered(6, clip);
    QVERIFY(soft && soft->isAlpha());
    const quint8 edge = soft->coverage(32, 64);
    QVERIFY(edge > 0 && edge < 255);

    QCOMPARE(image->setSelectionMask(hard), Result::Success);
    QCOMPARE(image->setSelectionMask(soft), Result::Success);
    QCOMPARE(image->fillLayer(layer, {19, 211, 97}, 255, false, false), Result::Success);
    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 128 + x) * 4, 4); };
    const auto filled = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(filled.code, ExecutionCode::Success);
    QCOMPARE(pixel(filled.bytes, 64, 64), QByteArray::fromHex("13d361ff"));
    QCOMPARE(quint8(pixel(filled.bytes, 32, 64).at(3)), edge);
    QCOMPARE(pixel(filled.bytes, 8, 8), QByteArray(4, char(0)));

    QCOMPARE(image->undo(), Result::Success);
    QVERIFY(image->selectionMask()->isAlpha());
    QCOMPARE(image->undo(), Result::Success);
    QVERIFY(!image->selectionMask()->isAlpha());
    QVERIFY(*image->selectionMask() == *hard);
    QCOMPARE(image->redo(), Result::Success);
    QVERIFY(image->selectionMask()->isAlpha());
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, 128 * 128 * 4).bytes, filled.bytes);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("feathered-selection.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    QCOMPARE(loaded->selectionKind(), SelectionKind::Freehand);
    QCOMPARE(loaded->selectionRect(), soft->bounds());
    QVERIFY(loaded->selectionMask());
    QVERIFY(loaded->selectionMask()->isAlpha());
    QVERIFY(*loaded->selectionMask() == *soft);
    const auto loadedPixels = loaded->readRgbaPixels(loaded->operations().activeLayer(), region, 128 * 128 * 4);
    QCOMPARE(loadedPixels.code, ExecutionCode::Success);
    QCOMPARE(loadedPixels.bytes, filled.bytes);
    QCOMPARE(quint8(pixel(loadedPixels.bytes, 32, 64).at(3)), edge);
}

void KisGpuPaintDocumentTest::transparencyMaskComposesSavesReopensAndContinues()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {128, 128};
    initial.layers.append({QStringLiteral("Masked"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000026}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 128 + x) * 4, 4); };
    const auto projectionBytes = [&](PaintImage &target) {
        const auto outcome = execution.readProjectionImage(target.handle(), target.operations().snapshot(), 128 * 128 * 4);
        if (outcome.code != ExecutionCode::Success) return QByteArray();
        const auto converted = outcome.image.convertToFormat(QImage::Format_RGBA8888);
        return QByteArray(reinterpret_cast<const char *>(converted.constBits()), converted.sizeInBytes());
    };

    QCOMPARE(image->setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    QCOMPARE(image->addTransparencyMaskFromSelection(layer), Result::Success);
    QCOMPARE(image->clearSelection(), Result::Success);
    QVERIFY(image->layerTransparencyMask(layer));
    QVERIFY(image->layerTransparencyMaskVisible(layer));
    QCOMPARE(image->fillLayer(layer, {19, 211, 97}), Result::Success);
    const auto layerPixels = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(layerPixels.code, ExecutionCode::Success);
    QCOMPARE(pixel(layerPixels.bytes, 64, 64), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(layerPixels.bytes, 8, 8), QByteArray::fromHex("13d361ff"));
    const auto composed = projectionBytes(*image);
    QCOMPARE(composed.size(), 128 * 128 * 4);
    QCOMPARE(pixel(composed, 64, 64), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 120, 120), QByteArray(4, '\0'));

    QCOMPARE(image->setTransparencyMaskVisible(layer, false), Result::Success);
    const auto visible = projectionBytes(*image);
    QCOMPARE(pixel(visible, 8, 8), QByteArray::fromHex("13d361ff"));
    QCOMPARE(image->setTransparencyMaskVisible(layer, true), Result::Success);
    QCOMPARE(projectionBytes(*image), composed);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("transparency-mask.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    const auto loadedLayer = loaded->operations().activeLayer();
    const auto loadedMask = loaded->layerTransparencyMask(loadedLayer);
    QVERIFY(loadedMask);
    QCOMPARE(loadedMask->bounds(), image->layerTransparencyMask(layer)->bounds());
    QCOMPARE(loadedMask->coverage(64, 64), quint8(255));
    QCOMPARE(loadedMask->coverage(8, 8), quint8(0));
    QVERIFY(loaded->layerTransparencyMaskVisible(loadedLayer));
    QCOMPARE(projectionBytes(*loaded), composed);
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, 128 * 128 * 4).bytes, layerPixels.bytes);

    // Continuing to paint outside the mask keeps the composition hidden while
    // the layer's own pixels retain the stroke.
    Brush brush;
    brush.rgba = {211, 97, 19, 255};
    brush.diameter = 12;
    QCOMPARE(loaded->paintFigure(loadedLayer, brush, {{{100, 100}, 1}}, Shape{}, FigureStyle{}, 23), Result::Success);
    const auto continued = projectionBytes(*loaded);
    QCOMPARE(pixel(continued, 100, 100), QByteArray(4, '\0'));
    QCOMPARE(pixel(continued, 64, 64), QByteArray::fromHex("13d361ff"));
    const auto continuedPixels = loaded->readRgbaPixels(loadedLayer, region, 128 * 128 * 4);
    QCOMPARE(continuedPixels.code, ExecutionCode::Success);
    QCOMPARE(quint8(pixel(continuedPixels.bytes, 100, 100).at(3)), 255);
    const auto savedPath = directory.filePath(QStringLiteral("transparency-mask-continued.kra"));
    QCOMPARE(reopened.document->saveKra(savedPath).code, FileCode::Success);
    const auto again = PaintDocument::openKra(execution, savedPath);
    QVERIFY2(again.code == FileCode::Success, qPrintable(again.diagnostic));
    const auto againImage = again.document->image();
    QVERIFY(againImage->layerTransparencyMask(againImage->operations().activeLayer()));
    QCOMPARE(projectionBytes(*againImage), continued);
    QCOMPARE(againImage->readRgbaPixels(againImage->operations().activeLayer(), region, 128 * 128 * 4).bytes,
             continuedPixels.bytes);

    // Undo removes the stroke, not the mask; removing the mask restores full
    // composition coverage and persists across another round trip.
    QCOMPARE(againImage->removeTransparencyMask(againImage->operations().activeLayer()), Result::Success);
    QCOMPARE(againImage->undo(), Result::Success);
    QVERIFY(againImage->layerTransparencyMask(againImage->operations().activeLayer()));
    QCOMPARE(againImage->redo(), Result::Success);
    QVERIFY(!againImage->layerTransparencyMask(againImage->operations().activeLayer()));
    const auto unmasked = projectionBytes(*againImage);
    QCOMPARE(pixel(unmasked, 8, 8), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(unmasked, 64, 64), QByteArray::fromHex("13d361ff"));
    const auto removedPath = directory.filePath(QStringLiteral("transparency-mask-removed.kra"));
    QCOMPARE(again.document->saveKra(removedPath).code, FileCode::Success);
    const auto removed = PaintDocument::openKra(execution, removedPath);
    QVERIFY2(removed.code == FileCode::Success, qPrintable(removed.diagnostic));
    QVERIFY(!removed.document->image()->layerTransparencyMask(removed.document->image()->operations().activeLayer()));
    QCOMPARE(projectionBytes(*removed.document->image()), unmasked);
}

void KisGpuPaintDocumentTest::transparencyMaskAcceptsDirectBrushEraseFillAndRoundTrips()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {128, 128};
    initial.layers.append({QStringLiteral("Direct mask"), true,
                           QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000027}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    const QRect region(QPoint(), initial.size);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 128 + x) * 4, 4); };
    const auto projectionBytes = [&](PaintImage &target) {
        const auto outcome = execution.readProjectionImage(target.handle(), target.operations().snapshot(), 128 * 128 * 4);
        if (outcome.code != ExecutionCode::Success) return QByteArray();
        const auto converted = outcome.image.convertToFormat(QImage::Format_RGBA8888);
        return QByteArray(reinterpret_cast<const char *>(converted.constBits()), converted.sizeInBytes());
    };

    QCOMPARE(image->fillLayer(layer, {19, 211, 97}), Result::Success);
    QCOMPARE(image->setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    QCOMPARE(image->addTransparencyMaskFromSelection(layer), Result::Success);
    QCOMPARE(image->clearSelection(), Result::Success);
    const auto originalMask = image->layerTransparencyMask(layer);
    QVERIFY(originalMask);
    const auto layerPixels = image->readRgbaPixels(layer, region, 128 * 128 * 4);
    QCOMPARE(layerPixels.code, ExecutionCode::Success);
    const auto originalProjection = projectionBytes(*image);
    QCOMPARE(pixel(originalProjection, 64, 64), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(originalProjection, 4, 4), QByteArray(4, '\0'));

    // A brush stroke outside the original coverage expands the retained mask
    // and leaves the layer's own pixels untouched.
    Brush brush;
    brush.rgba = {211, 97, 19, 255};
    brush.diameter = 16;
    int undo = image->operations().undoCount();
    auto stroke = image->beginTransparencyMaskStroke(layer, brush, 7);
    QVERIFY(!stroke.isNull());
    QVERIFY(image->operations().hasActiveStroke());
    QCOMPARE(image->operations().activeOperation(), stroke);
    QCOMPARE(image->appendSamples(stroke, {{{16, 16}, 1}}), Result::Success);
    QCOMPARE(image->commitStroke(stroke), Result::Success);
    QVERIFY(!image->operations().hasActiveStroke());
    QCOMPARE(image->operations().undoCount(), undo + 1);
    QCOMPARE(image->operations().historyAction(image->operations().undoCount()),
             HistoryAction::PaintStroke);
    const auto brushedMask = image->layerTransparencyMask(layer);
    QVERIFY(brushedMask);
    QCOMPARE(brushedMask->coverage(16, 16), quint8(255));
    QCOMPARE(brushedMask->coverage(64, 64), quint8(255));
    QCOMPARE(brushedMask->coverage(4, 4), quint8(0));
    QCOMPARE(image->readRgbaPixels(layer, region, 128 * 128 * 4).bytes, layerPixels.bytes);
    const auto brushedProjection = projectionBytes(*image);
    QCOMPARE(pixel(brushedProjection, 16, 16), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(brushedProjection, 4, 4), QByteArray(4, '\0'));

    QCOMPARE(image->undo(), Result::Success);
    QVERIFY(image->layerTransparencyMask(layer) == originalMask);
    QCOMPARE(projectionBytes(*image), originalProjection);
    QCOMPARE(image->redo(), Result::Success);
    QVERIFY(image->layerTransparencyMask(layer) == brushedMask);

    // Erasing reduces only coverage inside the mask.
    Brush eraser = brush;
    eraser.erase = true;
    undo = image->operations().undoCount();
    stroke = image->beginTransparencyMaskStroke(layer, eraser, 11);
    QVERIFY(!stroke.isNull());
    QCOMPARE(image->appendSamples(stroke, {{{64, 64}, 1}}), Result::Success);
    QCOMPARE(image->commitStroke(stroke), Result::Success);
    QCOMPARE(image->operations().undoCount(), undo + 1);
    QCOMPARE(image->operations().historyAction(image->operations().undoCount()),
             HistoryAction::PaintStroke);
    const auto erasedMask = image->layerTransparencyMask(layer);
    QVERIFY(erasedMask);
    QCOMPARE(erasedMask->coverage(64, 64), quint8(0));
    QCOMPARE(image->readRgbaPixels(layer, region, 128 * 128 * 4).bytes, layerPixels.bytes);
    const auto erasedProjection = projectionBytes(*image);
    QCOMPARE(pixel(erasedProjection, 64, 64), QByteArray(4, '\0'));
    QCOMPARE(pixel(erasedProjection, 16, 16), QByteArray::fromHex("13d361ff"));
    QCOMPARE(image->undo(), Result::Success);
    QVERIFY(image->layerTransparencyMask(layer) == brushedMask);
    QCOMPARE(image->redo(), Result::Success);
    QVERIFY(image->layerTransparencyMask(layer) == erasedMask);

    // Selection fill reveals only the selected coverage inside the mask.
    QCOMPARE(image->setSelectionRect(QRect(104, 16, 16, 16)), Result::Success);
    undo = image->operations().undoCount();
    QCOMPARE(image->fillTransparencyMask(layer, {17, 129, 241}), Result::Success);
    QVERIFY(image->layerTransparencyMask(layer));
    QCOMPARE(image->operations().undoCount(), undo + 1);
    QCOMPARE(image->operations().historyAction(image->operations().undoCount()),
             HistoryAction::FillLayer);
    const auto filledMask = image->layerTransparencyMask(layer);
    QVERIFY(filledMask->coverage(112, 24) > 0);
    QCOMPARE(filledMask->coverage(112, 40), quint8(0));
    QCOMPARE(filledMask->coverage(4, 4), quint8(0));
    QCOMPARE(image->readRgbaPixels(layer, region, 128 * 128 * 4).bytes, layerPixels.bytes);
    const auto filledProjection = projectionBytes(*image);
    QCOMPARE(pixel(filledProjection, 112, 24), QByteArray::fromHex("13d361ff"));
    QCOMPARE(pixel(filledProjection, 112, 40), QByteArray(4, '\0'));
    QCOMPARE(image->clearSelection(), Result::Success);

    // Saving and reopening retains the edited coverage, composition and pixels.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("direct-mask.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    const auto loadedLayer = loaded->operations().activeLayer();
    const auto loadedMask = loaded->layerTransparencyMask(loadedLayer);
    QVERIFY(loadedMask);
    QCOMPARE(loadedMask->coverage(16, 16), quint8(255));
    QCOMPARE(loadedMask->coverage(64, 64), quint8(0));
    QVERIFY(loadedMask->coverage(112, 24) > 0);
    QCOMPARE(loadedMask->coverage(112, 40), quint8(0));
    QCOMPARE(loadedMask->coverage(4, 4), quint8(0));
    QVERIFY(loaded->layerTransparencyMaskVisible(loadedLayer));
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, 128 * 128 * 4).bytes, layerPixels.bytes);
    QCOMPARE(projectionBytes(*loaded), filledProjection);
}

void KisGpuPaintDocumentTest::weightedSmoothingSaveReopenAndContinuePreservesPixels()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {64, 64};
    initial.layers.append({QStringLiteral("Weighted stroke"), true,
                          QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000022}")), {}});
    PaintDocument document(execution, initial);
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 4;
    brush.basicSmoothing = true;
    brush.weightedSmoothing = WeightedSmoothing{24, 48, 0.01, true};
    const QList<InputSample> samples{{{8, 16}, 1, 0, 0, 0, 0, 0}, {{12, 20}, 0.8, 0, 0, 0, 8, 0.25},
                                    {{16, 16}, 0.6, 0, 0, 0, 16, 0.5}, {{20, 20}, 0.4, 0, 0, 0, 24, 0.75},
                                    {{24, 16}, 0.2, 0, 0, 0, 32, 1}, {{28, 20}, 0.1, 0, 0, 0, 40, 0}};
    QCOMPARE(image->paintFigure(layer, brush, samples, Shape{}, FigureStyle{}, 42), Result::Success);
    const QRect region(QPoint(), initial.size);
    const auto painted = image->readRgbaPixels(layer, region, 64 * 64 * 4);
    QCOMPARE(painted.code, ExecutionCode::Success);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto destination = results.isEmpty() ? directory.path() : results;
    QVERIFY(QDir().mkpath(destination));
    const auto path = QDir(destination).filePath(QStringLiteral("weighted-smoothing.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto png = mergedImage(path).convertToFormat(QImage::Format_RGBA8888);
    QVERIFY(!png.isNull());
    QCOMPARE(png.size(), initial.size);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), painted.bytes);
    // Preserve the encoded PNG too, so environment comparisons cover the format output.
    QuaZip archive(path);
    QVERIFY(archive.open(QuaZip::mdUnzip));
    QVERIFY(archive.setCurrentFile(QStringLiteral("mergedimage.png")));
    QuaZipFile merged(&archive);
    QVERIFY(merged.open(QIODevice::ReadOnly));
    QFile output(QDir(destination).filePath(QStringLiteral("weighted-smoothing-merged.png")));
    QVERIFY(output.open(QIODevice::WriteOnly));
    const auto encoded = merged.readAll();
    QCOMPARE(output.write(encoded), qint64(encoded.size()));
    output.close();
    merged.close();
    archive.close();
    const auto reopened = PaintDocument::openKra(execution, path);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    const auto loadedLayer = loaded->operations().activeLayer();
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, painted.bytes.size()).bytes, painted.bytes);
    QCOMPARE(image->paintFigure(layer, brush, samples, Shape{}, FigureStyle{}, 43), Result::Success);
    QCOMPARE(loaded->paintFigure(loadedLayer, brush, samples, Shape{}, FigureStyle{}, 43), Result::Success);
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, painted.bytes.size()).bytes,
             image->readRgbaPixels(layer, region, painted.bytes.size()).bytes);
    QCOMPARE(loaded->undo(), Result::Success);
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, painted.bytes.size()).bytes, painted.bytes);
    QCOMPARE(loaded->redo(), Result::Success);
}

void KisGpuPaintDocumentTest::gaussianLineEditsSaveReopenAndContinueAtTheirNewPosition()
{
    Execution execution(m_device);
    RasterImage initial;
    initial.size = {512, 512};
    initial.layers.append({QStringLiteral("Gaussian line"), true,
                          QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000023}")), {}});
    PaintDocument document(execution, initial), reference(execution, initial);
    QFile profile(QStringLiteral(":/gpu-paint-results/sRGB-elle-V2-srgbtrc.icc"));
    QVERIFY(profile.open(QIODevice::ReadOnly));
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    metadata.iccProfile = profile.readAll();
    metadata.documentInfo = QByteArrayLiteral("<document-info><title>Gaussian line</title></document-info>");
    QCOMPARE(document.setKraImageMetadata(metadata).code, FileCode::Success);
    QCOMPARE(reference.setKraImageMetadata(metadata).code, FileCode::Success);
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 20;
    auto &settings = brush.gaussianWash.emplace();
    settings.resourceDigest = QByteArray(32, 'r');
    settings.flow = .5;
    settings.canvasOpacity = .57;
    settings.sizeUsesPressureDirectly = settings.opacityUsesPressureDirectly = true;
    for (int i = 0; i < 256; ++i) settings.sizePressure[i] = settings.opacityPressure[i] = qreal(i) / 255;
    const QList<InputSample> samples{{{32, 40}, .2}, {{128, 76}, 1}, {{224, 40}, .8}};
    Shape path;
    path.kind = ShapeKind::Path;
    path.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::LineTo};
    const auto image = document.image();
    const auto layer = image->operations().activeLayer();
    const Shape line{ShapeKind::Line, {304, 464}, {464, 384}};
    QCOMPARE(image->paintFigure(layer, brush, samples, line, FigureStyle{}, 42), Result::Success);
    const auto referenceImage = reference.image();
    const auto referenceLayer = referenceImage->operations().activeLayer();
    QCOMPARE(referenceImage->paintFigure(referenceLayer, brush,
        {{{304, 464}, .2}, {{384, 424}, 1}, {{464, 384}, .8}}, path, FigureStyle{}, 42), Result::Success);
    const QRect region(QPoint(), initial.size);
    const qsizetype maximumBytes = 512 * 512 * 4;
    const auto painted = image->readRgbaPixels(layer, region, maximumBytes);
    QCOMPARE(painted.code, ExecutionCode::Success);
    QCOMPARE(painted.bytes, referenceImage->readRgbaPixels(referenceLayer, region, maximumBytes).bytes);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto requested = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const QDir destination(requested.isEmpty() ? temporary.path() : requested + QStringLiteral("/gaussian-line"));
    QVERIFY(QDir().mkpath(destination.path()));
    const auto saved = destination.filePath(QStringLiteral("saved.kra"));
    const auto expected = destination.filePath(QStringLiteral("reference.kra"));
    QCOMPARE(document.saveKra(saved).code, FileCode::Success);
    QCOMPARE(reference.exportKra(expected).code, FileCode::Success);
    QCOMPARE(fileBytes(saved), fileBytes(expected));
    auto reopened = PaintDocument::openKra(execution, saved);
    QVERIFY2(reopened.code == FileCode::Success, qPrintable(reopened.diagnostic));
    const auto loaded = reopened.document->image();
    const auto loadedLayer = loaded->operations().activeLayer();
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, maximumBytes).bytes, painted.bytes);
    const Shape next{ShapeKind::Line, {40, 300}, {120, 380}};
    for (const auto &target : {image, loaded})
        QCOMPARE(target->paintFigure(target->operations().activeLayer(), brush, samples, next, FigureStyle{}, 43), Result::Success);
    QCOMPARE(referenceImage->paintFigure(referenceLayer, brush,
        {{{40, 300}, .2}, {{80, 340}, 1}, {{120, 380}, .8}}, path, FigureStyle{}, 43), Result::Success);
    QCOMPARE(loaded->undo(), Result::Success);
    QCOMPARE(loaded->readRgbaPixels(loadedLayer, region, maximumBytes).bytes, painted.bytes);
    QCOMPARE(loaded->redo(), Result::Success);
    const auto continued = loaded->readRgbaPixels(loadedLayer, region, maximumBytes);
    QCOMPARE(continued.code, ExecutionCode::Success);
    QCOMPARE(continued.bytes, image->readRgbaPixels(layer, region, maximumBytes).bytes);
    QCOMPARE(continued.bytes, referenceImage->readRgbaPixels(referenceLayer, region, maximumBytes).bytes);
    const auto edited = destination.filePath(QStringLiteral("edited.kra"));
    QCOMPARE(reopened.document->saveKra(edited).code, FileCode::Success);
    QCOMPARE(reference.exportKra(expected).code, FileCode::Success);
    QCOMPARE(fileBytes(edited), fileBytes(expected));
    const auto continuous = destination.filePath(QStringLiteral("continuous.kra"));
    QCOMPARE(document.exportKra(continuous).code, FileCode::Success);
    QCOMPARE(fileBytes(continuous), fileBytes(edited));
    for (const auto &value : {qMakePair(QStringLiteral("saved.rgba"), painted.bytes),
                              qMakePair(QStringLiteral("edited.rgba"), continued.bytes)}) {
        QFile file(destination.filePath(value.first));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(value.second), value.second.size());
    }
}

void KisGpuPaintDocumentTest::fixedEditsSaveReopenAndExportIdenticalResults()
{
    QFile profile(QStringLiteral(":/gpu-paint-results/sRGB-elle-V2-srgbtrc.icc"));
    QVERIFY2(profile.open(QIODevice::ReadOnly), "The fixed cross-environment ICC profile must be embedded");
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    metadata.iccProfile = profile.readAll();
    metadata.documentInfo = QByteArrayLiteral("<document-info><title>Fixed RGBA8 results</title></document-info>");
    metadata.annotations = {{QStringLiteral("notes"), QStringLiteral("Fixed fixture"), QByteArrayLiteral("fixed annotation")}};
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QMap<QString, QByteArray> results;
    for (int repetition = 0; repetition < 2; ++repetition) {
        Execution execution(m_device);
        RasterImage initial;
        initial.size = {64, 64};
        for (int index = 1; index <= 3; ++index) {
            RasterLayer layer;
            layer.name = QStringLiteral("Layer %1").arg(index);
            layer.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-%1}").arg(index, 12, 10, QLatin1Char('0')));
            initial.layers.append(layer);
        }
        PaintDocument document(execution, initial);
        auto image = document.image();
        const auto layers = image->operations().snapshot()->layers;
        QCOMPARE(document.setKraImageMetadata(metadata).code, FileCode::Success);
        auto paint = [](PaintImage &target, LayerId layer, Brush brush, quint64 seed) {
            const auto id = target.beginStroke(layer, brush, seed);
            QVERIFY(!id.isNull());
            QCOMPARE(target.appendSamples(id, {{{8.25, 9.5}, 1, 0, 0, 0, 0},
                                               {{20.5, 18.25}, 0.5, 0, 0, 0, 100},
                                               {{45.75, 42.5}, 0.75, 0, 0, 0, 200}}), Result::Success);
            QCOMPARE(target.commitStroke(id), Result::Success);
        };
        Brush brush;
        brush.rgba = {17, 129, 241, 255};
        brush.diameter = 8.5;
        brush.spacing = 0.2;
        paint(*image, layers[0].id, brush, 42);
        brush.rgba = {219, 33, 81, 173};
        brush.diameter = 12.25;
        paint(*image, layers[1].id, brush, 43);
        brush.erase = true;
        brush.diameter = 4;
        paint(*image, layers[0].id, brush, 44);
        QCOMPARE(image->setLayerOpacity(layers[1].id, 113), Result::Success);
        QCOMPARE(image->replaceRgbaPixels(layers[2].id, QRect(-3, 2, 1, 1), QByteArray::fromHex("11223344")), Result::Success);
        QCOMPARE(image->setLayerVisible(layers[2].id, false), Result::Success);
        QCOMPARE(image->selectLayer(layers[1].id), Result::Success);
        const auto path = directory.filePath(QStringLiteral("document.kra"));
        QCOMPARE(document.saveKra(path).code, FileCode::Success);
        auto opened = PaintDocument::openKra(execution, path);
        QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
        auto loaded = opened.document->image();
        QCOMPARE(loaded->operations().snapshot()->layers.size(), 3);
        QCOMPARE(loaded->operations().activeLayer(), loaded->operations().snapshot()->layers[1].id);
        const auto loadedMetadata = opened.document->kraImageMetadata();
        QCOMPARE(loadedMetadata.code, Kra::Status::Success);
        QCOMPARE(loadedMetadata.metadata.iccProfile, metadata.iccProfile);
        QCOMPARE(loadedMetadata.metadata.resolution, metadata.resolution);
        QMap<QString, QByteArray> actual;
        actual.insert(QStringLiteral("document.kra"), fileBytes(path));
        auto recordPixels = [&](const QString &prefix, PaintImage &target) {
            QJsonArray layerState;
            const auto version = target.operations().snapshot();
            for (int index = 0; index < version->layers.size(); ++index) {
                const auto &layer = version->layers[index];
                const auto pixels = target.readRgbaPixels(layer.id, QRect(-4, 0, 68, 64), 68 * 64 * 4);
                QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
                actual.insert(prefix + QStringLiteral("layer-%1.rgba").arg(index), pixels.bytes);
                layerState.append(QJsonObject{{QStringLiteral("uuid"), layer.uuid.toString()},
                    {QStringLiteral("name"), layer.name}, {QStringLiteral("visible"), layer.visible},
                    {QStringLiteral("opacity"), layer.opacity}, {QStringLiteral("locked"), layer.userLocked},
                    {QStringLiteral("selected"), layer.id == version->activeLayer}});
            }
            const auto projection = execution.readProjectionImage(target.handle(), version, 64 * 64 * 4);
            QVERIFY2(projection.code == ExecutionCode::Success, qPrintable(projection.diagnostic));
            actual.insert(prefix + QStringLiteral("projection.rgba"), QByteArray(
                reinterpret_cast<const char *>(projection.image.constBits()), projection.image.sizeInBytes()));
            actual.insert(prefix + QStringLiteral("state.json"), QJsonDocument(layerState).toJson(QJsonDocument::Compact));
        };
        recordPixels(QStringLiteral("saved-"), *image);
        recordPixels(QStringLiteral("reopened-"), *loaded);
        for (const auto &name : {QStringLiteral("layer-0.rgba"), QStringLiteral("layer-1.rgba"),
                                 QStringLiteral("layer-2.rgba"), QStringLiteral("projection.rgba"), QStringLiteral("state.json")})
            QCOMPARE(actual.value(QStringLiteral("saved-") + name), actual.value(QStringLiteral("reopened-") + name));
        brush.erase = false;
        brush.diameter = 5.5;
        brush.rgba = {31, 211, 83, 149};
        paint(*image, image->operations().activeLayer(), brush, 45);
        paint(*loaded, loaded->operations().activeLayer(), brush, 45);
        QCOMPARE(loaded->undo(), Result::Success);
        QCOMPARE(loaded->redo(), Result::Success);
        recordPixels(QStringLiteral("edited-"), *loaded);
        const auto finalPath = directory.filePath(QStringLiteral("edited.kra"));
        const auto continuousPath = directory.filePath(QStringLiteral("continuous.kra"));
        QCOMPARE(opened.document->saveKra(finalPath).code, FileCode::Success);
        QCOMPARE(document.exportKra(continuousPath).code, FileCode::Success);
        actual.insert(QStringLiteral("edited.kra"), fileBytes(finalPath));
        QCOMPARE(actual.value(QStringLiteral("edited.kra")), fileBytes(continuousPath));
        const auto exportPath = directory.filePath(QStringLiteral("exported.kra"));
        QCOMPARE(opened.document->exportKra(exportPath).code, FileCode::Success);
        QCOMPARE(fileBytes(exportPath), actual.value(QStringLiteral("edited.kra")));
        QVERIFY(!actual.value(QStringLiteral("document.kra")).isEmpty());
        QVERIFY(!actual.value(QStringLiteral("edited.kra")).isEmpty());
        QVERIFY(actual.value(QStringLiteral("document.kra")) != actual.value(QStringLiteral("edited.kra")));
        if (repetition == 0) results = actual;
        else QCOMPARE(actual, results);
    }
    const QString destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!destination.isEmpty()) {
        QVERIFY(QDir().mkpath(destination));
        for (auto result = results.cbegin(); result != results.cend(); ++result) {
            QFile output(QDir(destination).filePath(result.key()));
            QVERIFY(output.open(QIODevice::WriteOnly));
            QCOMPARE(output.write(result.value()), result.value().size());
        }
    }
}

void KisGpuPaintDocumentTest::colorFillSavesReopensAndContinuesIdentically_data()
{
    QTest::addColumn<quint8>("opacity");
    QTest::addColumn<bool>("erase");
    QTest::addColumn<bool>("alphaLocked");
    QTest::addColumn<quint8>("channelMask");
    QTest::addColumn<QByteArray>("blank");
    QTest::addColumn<QByteArray>("covered");
    QTest::addColumn<int>("blendMode");
    QTest::newRow("opacity") << quint8(128) << false << false << quint8(15)
        << QByteArray::fromHex("1181f180") << QByteArray::fromHex("225281ff")
        << int(LayerBlendMode::Normal);
    QTest::newRow("alpha-lock") << quint8(255) << false << true << quint8(15)
        << QByteArray::fromHex("1181f100") << QByteArray::fromHex("1181f1ff")
        << int(LayerBlendMode::Normal);
    QTest::newRow("erase") << quint8(255) << true << false << quint8(15)
        << QByteArray::fromHex("00000000") << QByteArray::fromHex("33221100")
        << int(LayerBlendMode::Normal);
    QTest::newRow("channel-lock") << quint8(255) << false << false << quint8(14)
        << QByteArray::fromHex("0081f1ff") << QByteArray::fromHex("3381f1ff")
        << int(LayerBlendMode::Normal);
    QTest::newRow("multiply") << quint8(255) << false << false << quint8(15)
        << QByteArray::fromHex("1181f1ff") << QByteArray::fromHex("031110ff")
        << int(LayerBlendMode::Multiply);
    QTest::newRow("screen") << quint8(255) << false << false << quint8(15)
        << QByteArray::fromHex("1181f1ff") << QByteArray::fromHex("4192f2ff")
        << int(LayerBlendMode::Screen);
    QTest::newRow("darken") << quint8(255) << false << false << quint8(15)
        << QByteArray::fromHex("1181f1ff") << QByteArray::fromHex("112211ff")
        << int(LayerBlendMode::Darken);
    QTest::newRow("lighten") << quint8(255) << false << false << quint8(15)
        << QByteArray::fromHex("1181f1ff") << QByteArray::fromHex("3381f1ff")
        << int(LayerBlendMode::Lighten);
    QTest::newRow("add") << quint8(255) << false << false << quint8(15)
        << QByteArray::fromHex("1181f1ff") << QByteArray::fromHex("44a3ffff")
        << int(LayerBlendMode::Addition);
    QTest::newRow("linear-dodge") << quint8(255) << false << false << quint8(15)
        << QByteArray::fromHex("1181f1ff") << QByteArray::fromHex("44a3ffff")
        << int(LayerBlendMode::LinearDodge);
}

void KisGpuPaintDocumentTest::colorFillSavesReopensAndContinuesIdentically()
{
    QFETCH(quint8, opacity);
    QFETCH(bool, erase);
    QFETCH(bool, alphaLocked);
    QFETCH(quint8, channelMask);
    QFETCH(QByteArray, blank);
    QFETCH(QByteArray, covered);
    QFETCH(int, blendMode);
    QFile profile(QStringLiteral(":/gpu-paint-results/sRGB-elle-V2-srgbtrc.icc"));
    QVERIFY(profile.open(QIODevice::ReadOnly));
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    metadata.iccProfile = profile.readAll();
    metadata.documentInfo = QByteArrayLiteral("<document-info><about><title>Color fill</title>"
        "<creation-date>2000-01-01T00:00:00Z</creation-date><date>2000-01-02T00:00:00Z</date></about></document-info>");
    const auto seed = QByteArray::fromHex("332211ff");
    RasterImage initial;
    initial.size = {32, 16};
    initial.activeLayer = 1;
    for (int index = 0; index < 2; ++index) {
        RasterLayer layer;
        layer.name = index == 0 ? QStringLiteral("Paint") : QStringLiteral("Other");
        layer.uuid = QUuid(QStringLiteral("{11223344-5566-7788-99aa-%1}").arg(index + 1, 12, 10, QLatin1Char('0')));
        layer.opacity = index == 0 ? 255 : 0;
        layer.channelWriteMask = index == 0 ? channelMask : 15;
        RgbaTile inside{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
        RgbaTile outside{{-1, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
        for (const auto point : {QPoint(3, 5), QPoint(40, 20)})
            std::memcpy(inside.rgba.data() + (point.y() * 256 + point.x()) * 4, seed.constData(), 4);
        std::memcpy(outside.rgba.data() + (4 * 256 + 251) * 4, seed.constData(), 4);
        layer.tiles = {inside, outside};
        initial.layers.append(layer);
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QMap<QString, QByteArray> reference;
    const auto preserveFailure = qScopeGuard([&] {
        if (!QTest::currentTestFailed()) return;
        directory.setAutoRemove(false);
        for (auto expected = reference.cbegin(); expected != reference.cend(); ++expected) {
            QFile output(directory.filePath(QStringLiteral("expected-") + expected.key()));
            if (output.open(QIODevice::WriteOnly)) output.write(expected.value());
        }
        qInfo() << "Color-fill KRA comparison artifacts:" << directory.path();
    });
    for (int repetition = 0; repetition < 2; ++repetition) {
        Execution execution(m_device);
        PaintDocument document(execution, initial);
        QCOMPARE(document.setKraImageMetadata(metadata).code, FileCode::Success);
        const auto image = document.image();
        const auto layer = image->operations().snapshot()->layers[0].id;
        const auto active = image->operations().activeLayer();
        QCOMPARE(image->fillLayer(layer, {17, 129, 241}, opacity, erase, alphaLocked,
                                  LayerBlendMode(blendMode)), Result::Success);
        QCOMPARE(image->operations().undoCount(), 1);
        QCOMPARE(image->operations().historyAction(1), HistoryAction::FillLayer);
        QCOMPARE(image->operations().activeLayer(), active);
        const auto checkPixels = [&](const PaintImageSP &target) {
            const auto layers = target->operations().snapshot()->layers;
            QCOMPARE(layers.size(), 2);
            QCOMPARE(target->readRgbaPixels(layers[0].id, {8, 8, 1, 1}, 4).bytes, blank);
            QCOMPARE(target->readRgbaPixels(layers[0].id, {3, 5, 1, 1}, 4).bytes, covered);
            for (const auto point : {QPoint(-5, 4), QPoint(40, 20)})
                QCOMPARE(target->readRgbaPixels(layers[0].id, QRect(point, QSize(1, 1)), 4).bytes, seed);
            QCOMPARE(target->readRgbaPixels(layers[1].id, {3, 5, 1, 1}, 4).bytes, seed);
            QCOMPARE(layers[0].channelWriteMask, channelMask);
            QCOMPARE(layers[0].uuid, initial.layers[0].uuid);
            QCOMPARE(layers[1].opacity, quint8(0));
            QCOMPARE(target->operations().activeLayer(), layers[1].id);
        };
        checkPixels(image);
        QCOMPARE(image->undo(), Result::Success);
        QCOMPARE(image->readRgbaPixels(layer, {3, 5, 1, 1}, 4).bytes, seed);
        QCOMPARE(image->redo(), Result::Success);
        checkPixels(image);
        const auto saved = directory.filePath(QStringLiteral("saved.kra"));
        QCOMPARE(document.saveKra(saved).code, FileCode::Success);
        auto opened = PaintDocument::openKra(execution, saved);
        QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
        const auto loaded = opened.document->image();
        checkPixels(loaded);
        const auto loadedMetadata = opened.document->kraImageMetadata();
        QCOMPARE(loadedMetadata.code, Kra::Status::Success);
        QCOMPARE(loadedMetadata.metadata.iccProfile, metadata.iccProfile);
        QCOMPARE(loadedMetadata.metadata.documentInfo, metadata.documentInfo);
        QCOMPARE(loadedMetadata.metadata.resolution, metadata.resolution);
        const auto reopened = directory.filePath(QStringLiteral("reopened.kra"));
        QCOMPARE(opened.document->exportKra(reopened).code, FileCode::Success);
        QCOMPARE(fileBytes(reopened), fileBytes(saved));
        for (const auto &target : {image, loaded})
            QCOMPARE(target->fillLayer(target->operations().snapshot()->layers[0].id, {221, 17, 44}, 85), Result::Success);
        QCOMPARE(loaded->undo(), Result::Success);
        checkPixels(loaded);
        QCOMPARE(loaded->redo(), Result::Success);
        const auto edited = directory.filePath(QStringLiteral("edited.kra"));
        const auto continuous = directory.filePath(QStringLiteral("continuous.kra"));
        QCOMPARE(opened.document->saveKra(edited).code, FileCode::Success);
        QCOMPARE(document.exportKra(continuous).code, FileCode::Success);
        QCOMPARE(fileBytes(edited), fileBytes(continuous));
        QMap<QString, QByteArray> actual{{QStringLiteral("saved.kra"), fileBytes(saved)},
                                       {QStringLiteral("edited.kra"), fileBytes(edited)}};
        QVERIFY(!actual.value(QStringLiteral("saved.kra")).isEmpty());
        QVERIFY(actual.value(QStringLiteral("saved.kra")) != actual.value(QStringLiteral("edited.kra")));
        if (repetition == 0) reference = actual;
        else QCOMPARE(actual, reference);
    }
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!destination.isEmpty()) {
        const auto path = QDir(destination).filePath(QStringLiteral("color-fill-") + QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(path));
        for (auto result = reference.cbegin(); result != reference.cend(); ++result) {
            QFile output(QDir(path).filePath(result.key()));
            QVERIFY(output.open(QIODevice::WriteOnly));
            QCOMPARE(output.write(result.value()), result.value().size());
        }
    }
}

void KisGpuPaintDocumentTest::sharedExecutionOpenReturnsValidatedMetadataAndKeepsRetainedImagesAlive()
{
    auto execution = std::make_shared<Execution>(m_device);
    std::weak_ptr<Execution> lifetime = execution;
    auto source = std::make_unique<PaintDocument>(*execution, QSize(32, 24));
    Kra::ImageMetadata metadata;
    metadata.resolution = {300, 150};
    QDomDocument axes;
    QVERIFY(axes.setContent(QByteArrayLiteral(
        "<MirrorAxis>"
        "<mirrorHorizontal type=\"value\" value=\"1\"/>"
        "<mirrorVertical type=\"value\" value=\"1\"/>"
        "<lockHorizontal type=\"value\" value=\"1\"/>"
        "<lockVertical type=\"value\" value=\"0\"/>"
        "<hideHorizontalDecoration type=\"value\" value=\"0\"/>"
        "<hideVerticalDecoration type=\"value\" value=\"1\"/>"
        "<handleSize type=\"value\" value=\"19.5\"/>"
        "<horizontalHandlePosition type=\"value\" value=\"8.25\"/>"
        "<verticalHandlePosition type=\"value\" value=\"-4.5\"/>"
        "<axisPosition type=\"pointf\" x=\"21.25\" y=\"13.5\"/>"
        "</MirrorAxis>"
    )));
    metadata.mirrorAxis = axes.toByteArray();
    metadata.iccProfile = QColorSpace(QColorSpace::SRgb).iccProfile();
    metadata.documentInfo = QByteArrayLiteral("<document-info><title>Imported title</title></document-info>");
    metadata.annotations = {{QStringLiteral("notes"), QStringLiteral("Project notes"), QByteArrayLiteral("note bytes")},
                            {QStringLiteral("exif"), {}, QByteArray::fromHex("0011ff")}};
    QCOMPARE(source->setKraImageMetadata(metadata).code, FileCode::Success);
    dot(*source->image(), {12, 12}, {17, 34, 51, 255});
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("metadata.kra"));
    QCOMPARE(source->saveKra(path).code, FileCode::Success);
    auto opened = PaintDocument::openKra(execution, path);
    QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
    const auto loadedMetadata = opened.document->kraImageMetadata();
    QVERIFY2(loadedMetadata.code == Kra::Status::Success, qPrintable(loadedMetadata.diagnostic));
    const auto &loaded = loadedMetadata.metadata;
    QCOMPARE(loaded.resolution, metadata.resolution);
    QCOMPARE(loaded.iccProfile, metadata.iccProfile);
    QCOMPARE(loaded.documentInfo, metadata.documentInfo);
    QCOMPARE(loaded.mirrorAxis, metadata.mirrorAxis);
    QCOMPARE(loaded.annotations.size(), 2);
    QCOMPARE(loaded.annotations[0].type, metadata.annotations[0].type);
    QCOMPARE(loaded.annotations[0].description, metadata.annotations[0].description);
    QCOMPARE(loaded.annotations[0].bytes, metadata.annotations[0].bytes);
    QCOMPARE(loaded.annotations[1].type, QStringLiteral("exif"));
    QCOMPARE(loaded.annotations[1].bytes, metadata.annotations[1].bytes);
    auto retained = opened.document->image();
    opened.document.reset();
    source.reset();
    execution.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(retained->readRgbaPixels(retained->operations().activeLayer(), QRect(12, 12, 1, 1), 4).bytes,
             QByteArray::fromHex("112233ff"));
    retained.reset();
    QVERIFY(lifetime.expired());
    QCOMPARE(PaintDocument::openKra(std::shared_ptr<Execution>{}, path).code, FileCode::InvalidData);
}

void KisGpuPaintDocumentTest::adoptedImagesExportWithoutChangingSaveIdentity()
{
    Execution execution(m_device, 128 * 1024 * 1024, 1);
    auto image = PaintImageSP::create(execution, QSize(64, 64));
    const auto handle = image->handle();
    PaintDocument document(image);
    QCOMPARE(document.image(), image);
    QCOMPARE(document.image()->handle(), handle);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("saved.kra"));
    const auto exported = directory.filePath(QStringLiteral("exported.kra"));
    dot(*image, {12, 12}, {17, 34, 51, 255});
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto saved = image->operations().snapshot();
    dot(*image, {12, 12}, {68, 85, 102, 255});
    const auto current = image->operations().snapshot();
    QSignalSpy changes(&document, &PaintDocument::changed);
    QCOMPARE(document.exportKra(exported).code, FileCode::Success);
    QCOMPARE(document.filePath(), path);
    QCOMPARE(image->operations().snapshot(), current);
    QCOMPARE(image->operations().undoCount(), 2);
    QCOMPARE(image->operations().savedHistoryIndex(), 1);
    QCOMPARE(changes.size(), 0);
    QCOMPARE(mergedImage(exported).pixelColor(12, 12), QColor(68, 85, 102));
    QCOMPARE(document.exportKra(exported, saved).code, FileCode::Success);
    QCOMPARE(mergedImage(exported).pixelColor(12, 12), QColor(17, 34, 51));
    QBuffer serialized;
    QVERIFY(serialized.open(QIODevice::ReadWrite));
    const auto written = document.exportKra(serialized, saved);
    QCOMPARE(written.code, FileCode::Success);
    QCOMPARE(written.version, saved);
    QVERIFY(serialized.isOpen());
    QCOMPARE(serialized.data(), fileBytes(exported));
    QCOMPARE(document.filePath(), path);
    QCOMPARE(image->operations().snapshot(), current);
    QCOMPARE(image->operations().savedHistoryIndex(), 1);
    QCOMPARE(changes.size(), 0);
    QVERIFY(image->operations().isModified());
    const auto recovery = document.recoveryData();
    QList<Kra::ImageMetadata> invalidMetadata(6);
    invalidMetadata[0].resolution.setX(std::numeric_limits<qreal>::quiet_NaN());
    invalidMetadata[1].documentInfo = QByteArrayLiteral("invalid XML");
    invalidMetadata[2].iccProfile = QByteArrayLiteral("invalid ICC profile");
    invalidMetadata[3].annotations = {{QStringLiteral("../outside"), {}, {}}};
    invalidMetadata[4].annotations = {{QStringLiteral("duplicate"), {}, {}}, {QStringLiteral("duplicate"), {}, {}}};
    invalidMetadata[5].mirrorAxis = QByteArrayLiteral("<invalid/>");
    for (const auto &metadata : invalidMetadata) {
        QCOMPARE(document.setKraImageMetadata(metadata).code, FileCode::InvalidData);
        QCOMPARE(document.recoveryData(), recovery);
    }
}

void KisGpuPaintDocumentTest::completedFormatWritesAdoptPinnedIdentityAndRecoverIt()
{
    Execution execution(m_device);
    PaintDocument document(execution, QSize(64, 64));
    const auto image = document.image();
    dot(*image, {12, 12}, {17, 34, 51, 255});
    const auto pinned = image->operations().snapshot();
    dot(*image, {32, 32}, {68, 85, 102, 255});
    const auto latest = image->operations().snapshot();
    const auto path = QStringLiteral("completed.png");
    QSignalSpy changed(&document, &PaintDocument::changed);
    const auto adopted = document.adoptSavedFile(path, pinned);
    QCOMPARE(adopted.code, FileCode::Success);
    QCOMPARE(adopted.version, pinned);
    QCOMPARE(document.filePath(), path);
    QCOMPARE(image->operations().snapshot(), latest);
    QCOMPARE(image->operations().savedHistoryIndex(), 1);
    QVERIFY(image->operations().isModified());
    QCOMPARE(changed.size(), 1);
    const auto recovery = document.recoveryData();
    QVERIFY(!recovery.isEmpty());
    PaintDocument restored(execution, QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.filePath(), path);
    QVERIFY(restored.image()->operations().isModified());
    QCOMPARE(restored.image()->undo(), Result::Success);
    QVERIFY(!restored.image()->operations().isModified());
    OperationDocument foreign({64, 64});
    QCOMPARE(document.adoptSavedFile(QStringLiteral("foreign.png"), foreign.snapshot()).code, FileCode::InvalidData);
    QCOMPARE(document.adoptSavedFile({}, pinned).code, FileCode::InvalidData);
    QCOMPARE(document.filePath(), path);
    QCOMPARE(image->operations().savedHistoryIndex(), 1);
    QCOMPARE(changed.size(), 1);
}

void KisGpuPaintDocumentTest::savedStateNotificationsAllowDocumentDeletion()
{
    Execution execution(m_device);
    auto document = std::make_unique<PaintDocument>(execution, QSize(64, 64));
    auto image = document->image();
    dot(*image, {12, 12}, {17, 34, 51, 255});
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("saved.kra"));
    QString observedPath;
    connect(image.data(), &PaintImage::savedStateChanged, this, [&] {
        observedPath = document->filePath();
        document.reset();
    });
    QCOMPARE(document->saveKra(path).code, FileCode::Success);
    QVERIFY(!document);
    QCOMPARE(observedPath, path);
    QVERIFY(!image->operations().isModified());
    QCOMPARE(mergedImage(path).pixelColor(12, 12), QColor(17, 34, 51));
}

void KisGpuPaintDocumentTest::sharedImageEditsAndRecoveryPublishCompleteFileMetadata()
{
    Execution execution(m_device, 128 * 1024 * 1024, 1);
    auto document = std::make_unique<PaintDocument>(execution, QSize(64, 64));
    auto image = document->image();
    const auto handle = document->image()->handle();
    QSignalSpy documentChanged(document.get(), &PaintDocument::changed);
    QSignalSpy imageChanged(image.data(), &PaintImage::changed);
    const auto stroke = image->beginStroke(image->operations().activeLayer(), Brush{}, 1);
    QCOMPARE(image->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(image->commitStroke(stroke), Result::Success);
    QCOMPARE(document->image()->operations().undoCount(), 1);
    QCOMPARE(documentChanged.size(), 2);
    QCOMPARE(imageChanged.size(), 2);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto first = directory.filePath(QStringLiteral("first.kra"));
    const auto second = directory.filePath(QStringLiteral("second.kra"));
    QString expectedPath = first;
    bool metadataCurrent = true;
    bool checkMetadata = true;
    const auto observe = [&] {
        if (!checkMetadata) return;
        metadataCurrent &= document->filePath() == expectedPath;
        metadataCurrent &= execution.requested(handle) == image->operations().preview();
    };
    connect(document.get(), &PaintDocument::changed, this, observe);
    connect(image.data(), &PaintImage::changed, this, observe);
    QCOMPARE(document->saveKra(first).code, FileCode::Success);
    QCOMPARE(imageChanged.size(), 2);
    const auto recovery = document->recoveryData();
    QVERIFY(!recovery.isEmpty());
    expectedPath = second;
    QCOMPARE(document->saveKra(second).code, FileCode::Success);
    auto corrupt = recovery;
    corrupt[10] ^= 1;
    corrupt.replace(corrupt.size() - 32, 32,
                    QCryptographicHash::hash(QByteArrayView(corrupt.constData(), corrupt.size() - 32), QCryptographicHash::Sha256));
    const auto before = image->operations().snapshot();
    const auto documentCount = documentChanged.size();
    const auto imageCount = imageChanged.size();
    QCOMPARE(document->restoreRecovery(corrupt), Result::InvalidRecovery);
    QCOMPARE(document->filePath(), second);
    QCOMPARE(image->operations().snapshot(), before);
    QCOMPARE(documentChanged.size(), documentCount);
    QCOMPARE(imageChanged.size(), imageCount);
    expectedPath = first;
    QCOMPARE(document->restoreRecovery(recovery), Result::Success);
    QCOMPARE(document->image()->handle(), handle);
    QVERIFY(image->operations().snapshot()->documentId() != before->documentId());
    QCOMPARE(documentChanged.size(), documentCount + 1);
    QCOMPARE(imageChanged.size(), imageCount + 1);
    QVERIFY(metadataCurrent);
    checkMetadata = false;
    document.reset();
    QCOMPARE(image->undo(), Result::Success);
    QCOMPARE(execution.requested(handle), image->operations().preview());
    image.clear();
    QVERIFY(!execution.requested(handle));
}

void KisGpuPaintDocumentTest::savingStreamsPixelsWithinTheGpuAndTransferBudgets()
{
    RasterImage image;
    image.size = {512, 256};
    image.layers = {
        {QStringLiteral("Bottom"), true, QUuid::createUuid(),
         {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)},
          {{1, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Upper"), true, QUuid::createUuid(),
         {{{0, 0}, QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4)},
          {{1, 0}, QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Hidden"), false, QUuid::createUuid(),
         {{{-1, 0}, QByteArray::fromHex("09080700").repeated(RgbaTile::ByteCount / 4)},
          {{2, 0}, QByteArray::fromHex("09080700").repeated(RgbaTile::ByteCount / 4)}}, {12, 34, 56, 0}}
    };
    Execution execution(m_device, 4 * TileRenderer::TileBytes, 16, TileRenderer::TileBytes);
    PaintDocument document(execution, image);
    dot(*document.image(), {270, 100}, {0, 255, 0, 128});
    const auto pinned = document.image()->operations().snapshot();
    const auto history = document.image()->operations().undoCount();
    Execution oracle(m_device);
    const auto admitted = oracle.open(pinned);
    QCOMPARE(admitted.code, ExecutionCode::Success);
    const auto expected = oracle.readPixels(admitted.handle, pinned,
        {-8388608, -8388608, 16777216, 16777216}, {1, 2, 3}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(expected.code, ExecutionCode::Success);
    const auto projection = oracle.readPixels(admitted.handle, pinned, {0, 0, 512, 256});
    QCOMPARE(projection.code, ExecutionCode::Success);
    {
        const auto frame = execution.beginReadbackFrame();
        QCOMPARE(frame.code, ExecutionCode::Success);
        const auto displayed = execution.project(document.image()->handle(), {0, 0, 256, 256});
        QCOMPARE(displayed.code, RenderOutcome::Recorded);
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    }
    QCOMPARE(execution.published(document.image()->handle()), pinned);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("bounded.kra"));
    const auto saved = document.saveKra(path);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QCOMPARE(execution.requested(document.image()->handle()), pinned);
    QCOMPARE(execution.published(document.image()->handle()), pinned);
    QCOMPARE(document.image()->operations().undoCount(), history);
    QVERIFY(!document.image()->operations().isModified());
    QFile input(path);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(input);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.size(), image.layers.size());
    for (qsizetype i = 0; i < image.layers.size(); ++i) {
        const auto &layer = loaded.document.image.layers[i];
        QCOMPARE(layer.name, image.layers[i].name);
        QCOMPARE(layer.uuid, image.layers[i].uuid);
        QCOMPARE(layer.visible, image.layers[i].visible);
        QVERIFY(layer.defaultPixel == image.layers[i].defaultPixel);
        QCOMPARE(layer.tiles.size(), image.layers[i].tiles.size());
        for (const auto &tile : layer.tiles) {
            bool found = false;
            for (const auto &pixels : expected.tiles) {
                if (pixels.layer != LayerId(i + 1) || pixels.coordinate != tile.coordinate) continue;
                QCOMPARE(tile.rgba, pixels.rgba);
                found = true;
            }
            QVERIFY(found);
        }
    }
    QImage frame(image.size, QImage::Format_RGBA8888);
    frame.fill(Qt::transparent);
    for (const auto &tile : projection.tiles) {
        for (int y = 0; y < RgbaTile::Size; ++y)
            std::memcpy(frame.scanLine(y) + tile.coordinate.x() * RgbaTile::Size * 4,
                        tile.rgba.constData() + y * RgbaTile::Size * 4, RgbaTile::Size * 4);
    }
    QCOMPARE(mergedImage(path).convertToFormat(QImage::Format_RGBA8888), frame);
}

void KisGpuPaintDocumentTest::layerStructureAndEmptyDocumentsRoundTripWithUnifiedHistory()
{
    Execution execution(m_device);
    PaintDocument document(execution, QSize(64, 64));
    dot(*document.image(), {8, 12}, {255, 0, 0, 255});
    const auto red = document.image()->operations().activeLayer();
    const auto blue = document.image()->addLayer(QStringLiteral("Upper"));
    dot(*document.image(), {8, 12}, {0, 0, 255, 255});
    QCOMPARE(document.image()->renameLayer(red, QStringLiteral("Renamed 下地")), Result::Success);
    QCOMPARE(document.image()->moveLayer(red, 1), Result::Success);
    auto value = execution.readPixels(document.image()->handle(), document.image()->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(value.code, ExecutionCode::Success);
    QCOMPARE(pixel(value, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    const auto reordered = document.image()->operations().snapshot();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("layers.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    auto opened = PaintDocument::openKra(execution, path);
    QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
    QCOMPARE(opened.document->image()->operations().snapshot()->layers[1].name, QStringLiteral("Renamed 下地"));
    QCOMPARE(opened.document->image()->operations().snapshot()->layers[1].uuid, reordered->layers[1].uuid);
    QCOMPARE(document.image()->removeLayer(red), Result::Success);
    value = execution.readPixels(document.image()->handle(), document.image()->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(value.code, ExecutionCode::Success);
    QCOMPARE(pixel(value, 0, {8, 12}), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document.image()->removeLayer(blue), Result::Success);
    const auto empty = document.image()->operations().snapshot();
    value = execution.readPixels(document.image()->handle(), empty, {0, 0, 64, 64});
    QCOMPARE(value.code, ExecutionCode::Success);
    QVERIFY(value.tiles.isEmpty());
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto history = document.image()->operations().undoCount();
    const auto recovered = document.recoveryData();
    QVERIFY(!recovered.isEmpty());
    opened = PaintDocument::openKra(execution, path);
    QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
    QVERIFY(opened.document->image()->operations().snapshot()->layers.isEmpty());
    QCOMPARE(opened.document->image()->operations().undoCount(), 0);
    const auto merged = mergedImage(path);
    QCOMPARE(merged.size(), QSize(64, 64));
    QCOMPARE(merged.pixelColor(8, 12), QColor(Qt::transparent));
    QCOMPARE(document.restoreRecovery(recovered), Result::Success);
    QCOMPARE(document.image()->operations().undoCount(), history);
    QCOMPARE(document.image()->undo(), Result::Success);
    value = execution.readPixels(document.image()->handle(), document.image()->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(value.code, ExecutionCode::Success);
    QCOMPARE(pixel(value, 0, {8, 12}), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document.image()->undo(), Result::Success);
    value = execution.readPixels(document.image()->handle(), document.image()->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(value.code, ExecutionCode::Success);
    QCOMPARE(pixel(value, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(document.image()->redo(), Result::Success);
    QCOMPARE(document.image()->redo(), Result::Success);
    QVERIFY(document.image()->operations().snapshot()->layers.isEmpty());
    QVERIFY(document.image()->addLayer(QStringLiteral("New")));
    QCOMPARE(document.image()->operations().redoCount(), 0);
}

void KisGpuPaintDocumentTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
}

void KisGpuPaintDocumentTest::acceptedChangesUpdateOneAttachedDocument()
{
    Execution execution(m_device);
    DocumentHandle handle = 0;
    Snapshot retained;
    {
        PaintDocument document(execution, QSize(512, 64));
        handle = document.image()->handle();
        QVERIFY(handle);
        QCOMPARE(execution.requested(handle), document.image()->operations().snapshot());
        QSignalSpy changed(&document, &PaintDocument::changed);
        const auto initial = document.image()->operations().snapshot();
        Brush brush;
        auto stroke = document.image()->beginStroke(document.image()->operations().activeLayer(), brush, 0);
        QCOMPARE(document.image()->appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
        QCOMPARE(execution.requested(handle), document.image()->operations().preview());
        QCOMPARE(changed.size(), 1);
        const auto preview = document.image()->operations().preview();
        QCOMPARE(document.image()->commitStroke(stroke), Result::Success);
        QCOMPARE(document.image()->operations().snapshot(), preview);
        QCOMPARE(execution.requested(handle), preview);
        QCOMPARE(document.image()->undo(), Result::Success);
        QCOMPARE(execution.requested(handle), initial);
        QCOMPARE(document.image()->redo(), Result::Success);
        QCOMPARE(execution.requested(handle), preview);
        stroke = document.image()->beginStroke(document.image()->operations().activeLayer(), brush, 0);
        QCOMPARE(document.image()->appendSamples(stroke, {{{264, 12}, 1}}), Result::Success);
        QCOMPARE(document.image()->cancelStroke(stroke), Result::Success);
        QCOMPARE(execution.requested(handle), preview);
        const auto layer = document.image()->addLayer(QStringLiteral("Second"));
        QVERIFY(layer);
        QCOMPARE(document.image()->setLayerVisible(layer, false), Result::Success);
        QCOMPARE(execution.requested(handle), document.image()->operations().snapshot());
        retained = document.image()->operations().snapshot();
    }
    QVERIFY(!execution.requested(handle));
    QCOMPARE(execution.pendingDocuments(), 0);
    QCOMPARE(retained->layers.size(), 2);
}

void KisGpuPaintDocumentTest::pinnedSavePreservesLaterEditsAndHistory()
{
    Execution execution(m_device);
    PaintDocument document(execution, QSize(512, 64));
    dot(*document.image(), {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.image()->operations().snapshot();
    dot(*document.image(), {264, 12}, {0, 0, 255, 255});
    const auto latest = document.image()->operations().snapshot();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("pinned.kra"));
    const auto saved = document.saveKra(path, pinned);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QCOMPARE(saved.version, pinned);
    QCOMPARE(document.filePath(), path);
    QVERIFY(document.image()->operations().isModified());
    QCOMPARE(document.image()->operations().snapshot(), latest);
    QCOMPARE(execution.requested(document.image()->handle()), latest);
    QCOMPARE(document.image()->operations().undoCount(), 2);
    const auto merged = mergedImage(path);
    QCOMPARE(merged.size(), pinned->size);
    QCOMPARE(merged.pixelColor(8, 12), QColor(255, 0, 0, 255));
    QCOMPARE(merged.pixelColor(264, 12), QColor(0, 0, 0, 0));
    auto opened = PaintDocument::openKra(execution, path);
    QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
    QVERIFY(opened.document);
    QVERIFY(!opened.document->image()->operations().isModified());
    QCOMPARE(opened.document->image()->operations().undoCount(), 0);
    const auto image = opened.document->image()->operations().snapshot();
    const auto read = execution.readPixels(opened.document->image()->handle(), image, {0, 0, 512, 64}, {image->activeLayer});
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(pixel(read, image->activeLayer, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QVERIFY(pixel(read, image->activeLayer, {264, 12}).isEmpty() || pixel(read, image->activeLayer, {264, 12}) == QByteArray(4, '\0'));
    QCOMPARE(document.image()->undo(), Result::Success);
    QVERIFY(!document.image()->operations().isModified());
    const auto alternate = directory.filePath(QStringLiteral("alternate.kra"));
    QCOMPARE(document.saveKra(alternate).code, FileCode::Success);
    QCOMPARE(document.filePath(), alternate);
    QCOMPARE(document.image()->operations().redoCount(), 1);
    QVERIFY(!fileBytes(path).isEmpty());
}

void KisGpuPaintDocumentTest::rasterImportAndSaveRetainOffCanvasHiddenPixelsAndMetadata()
{
    Kra::Document source;
    source.image.size = {100, 100};
    source.image.activeLayer = 1;
    source.image.projectionBackground = {17, 34, 51, 128};
    source.image.layers = {
        {QStringLiteral("Lower"), true, QUuid::createUuid(), {{{-1, 0}, QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Hidden"), false, QUuid::createUuid(), {{{0, 0}, QByteArray::fromHex("197fafe0").repeated(RgbaTile::ByteCount / 4)}}}
    };
    source.image.layers[0].defaultPixel = {17, 23, 31, 0};
    source.image.layers[1].defaultPixel = {255, 255, 255, 255};
    source.files.insert(QStringLiteral("notes/data.bin"), QByteArray::fromHex("ff002137"));
    QImage projection(source.image.size, QImage::Format_RGBA8888);
    projection.fill(Qt::transparent);
    projection.setColorSpace(QColorSpace::SRgb);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto original = directory.filePath(QStringLiteral("original.kra"));
    QFile output(original);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, source, projection).code, Kra::Status::Success);
    output.close();
    Execution execution(m_device);
    auto opened = PaintDocument::openKra(execution, original);
    QVERIFY2(opened.document, qPrintable(opened.diagnostic));
    const auto snapshot = opened.document->image()->operations().snapshot();
    QCOMPARE(snapshot->projectionBackground, source.image.projectionBackground);
    QCOMPARE(opened.document->image()->selectLayer(snapshot->layers[0].id), Result::Success);
    dot(*opened.document->image(), {8, 12}, {255, 0, 0, 255});
    const auto savedPath = directory.filePath(QStringLiteral("saved.kra"));
    const auto saved = opened.document->saveKra(savedPath);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QFile input(savedPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto decoded = Kra::RasterCodec::read(input);
    QVERIFY2(decoded.code == Kra::Status::Success, qPrintable(decoded.diagnostic));
    QCOMPARE(decoded.document.image.projectionBackground, source.image.projectionBackground);
    QCOMPARE(decoded.document.files.value(QStringLiteral("notes/data.bin")), source.files.value(QStringLiteral("notes/data.bin")));
    for (int i = 0; i < 2; ++i) {
        QCOMPARE(decoded.document.image.layers[i].uuid, source.image.layers[i].uuid);
        QCOMPARE(decoded.document.image.layers[i].defaultPixel, source.image.layers[i].defaultPixel);
        QCOMPARE(decoded.document.image.layers[i].visible, source.image.layers[i].visible);
    }
    const auto &negative = decoded.document.image.layers[0].tiles.first();
    QCOMPARE(negative.coordinate, QPoint(-1, 0));
    QCOMPARE(negative.rgba, source.image.layers[0].tiles[0].rgba);
    QCOMPARE(decoded.document.image.layers[1].tiles.first().rgba, source.image.layers[1].tiles[0].rgba);
}

void KisGpuPaintDocumentTest::rejectedSavesPreserveTheFileIdentityAndHistory()
{
    Execution execution(m_device);
    PaintDocument document(execution, QSize(64, 64));
    dot(*document.image(), {8, 12}, {255, 0, 0, 255});
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("existing.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto bytes = fileBytes(path);
    const auto saved = document.image()->operations().snapshot();
    Brush brush;
    const auto stroke = document.image()->beginStroke(saved->activeLayer, brush, 0);
    QCOMPARE(document.image()->appendSamples(stroke, {{{24, 12}, 1}}), Result::Success);
    QCOMPARE(document.saveKra(path, document.image()->operations().preview()).code, FileCode::InvalidData);
    QCOMPARE(fileBytes(path), bytes);
    QCOMPARE(document.image()->cancelStroke(stroke), Result::Success);
    dot(*document.image(), {24, 12}, {0, 0, 255, 255});
    const auto latest = document.image()->operations().snapshot();
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(document.saveKra(path).code, FileCode::Busy);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(document.saveKra(directory.filePath(QStringLiteral("missing/failure.kra"))).code, FileCode::IoFailure);
    OperationDocument foreign(QSize(64, 64));
    QCOMPARE(document.saveKra(path, foreign.snapshot()).code, FileCode::InvalidData);
    QCOMPARE(fileBytes(path), bytes);
    QCOMPARE(document.filePath(), path);
    QCOMPARE(document.image()->operations().snapshot(), latest);
    QCOMPARE(document.image()->operations().undoCount(), 2);
    QVERIFY(document.image()->operations().isModified());
    document.close();
    QCOMPARE(document.saveKra(path).code, FileCode::Closed);
    QCOMPARE(fileBytes(path), bytes);
}

void KisGpuPaintDocumentTest::rejectedOpenPreservesAttachedDocuments()
{
    Execution execution(m_device, 128 * 1024 * 1024, 1);
    PaintDocument document(execution, QSize(64, 64));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("valid.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto opened = PaintDocument::openKra(execution, path);
    QCOMPARE(opened.code, FileCode::CapacityExceeded);
    QVERIFY(!opened.document);
    QCOMPARE(execution.requested(document.image()->handle()), document.image()->operations().snapshot());
    const auto absent = PaintDocument::openKra(execution, directory.filePath(QStringLiteral("absent.kra")));
    QCOMPARE(absent.code, FileCode::IoFailure);
    QVERIFY(!absent.document);
}

void KisGpuPaintDocumentTest::notificationsAllowSynchronousCloseAndDeletion()
{
    Execution execution(m_device);
    PaintDocument closing(execution, QSize(64, 64));
    const auto handle = closing.image()->handle();
    connect(&closing, &PaintDocument::changed, &closing, [&] { if (closing.image()->handle()) closing.close(); });
    const auto stroke = closing.image()->beginStroke(closing.image()->operations().activeLayer(), {}, 0);
    QCOMPARE(closing.image()->appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
    QCOMPARE(closing.image()->handle(), 0);
    QVERIFY(!execution.requested(handle));

    auto deleting = std::make_unique<PaintDocument>(execution, QSize(64, 64));
    const auto second = deleting->image()->handle();
    const auto next = deleting->image()->beginStroke(deleting->image()->operations().activeLayer(), {}, 0);
    connect(deleting.get(), &PaintDocument::changed, this, [&] { deleting.reset(); });
    QCOMPARE(deleting->image()->appendSamples(next, {{{8, 12}, 1}}), Result::Success);
    QVERIFY(!deleting);
    QVERIFY(!execution.requested(second));
}

void KisGpuPaintDocumentTest::legacyRecoveryRetainsFileColorLabels()
{
    const auto model = groupedLayerRecoveryV29();
    OperationDocument previous({1, 1});
    QCOMPARE(previous.restoreRecovery(model), Result::Success);
    const auto layers = previous.snapshot()->layers;
    QCOMPARE(layers.size(), 5);
    QCOMPARE(layers[0].name, QStringLiteral("Base"));
    QCOMPARE(layers[0].defaultPixel, (std::array<quint8, 4>{255, 0, 0, 255}));
    QCOMPARE(layers[1].parentUuid, layers[2].uuid);
    QCOMPARE(layers[2].parentUuid, layers[3].uuid);
    for (const auto &layer : layers) QCOMPARE(layer.colorLabelIndex, 0);
    QDomDocument xml;
    QVERIFY(xml.setContent(QByteArrayLiteral("<DOC syntaxVersion=\"2\"><IMAGE name=\"image\" colorspacename=\"RGBA\" profile=\"sRGB built-in\"><layers/></IMAGE></DOC>")));
    const auto root = xml.documentElement().firstChildElement(QStringLiteral("IMAGE")).firstChildElement(QStringLiteral("layers"));
    QMap<QUuid, QDomElement> elements;
    for (const auto &layer : layers) {
        auto element = xml.createElement(QStringLiteral("layer"));
        element.setAttribute(QStringLiteral("name"), layer.name);
        element.setAttribute(QStringLiteral("nodetype"), layer.kind == LayerKind::Paint ? QStringLiteral("paintlayer") : QStringLiteral("grouplayer"));
        element.setAttribute(QStringLiteral("compositeop"), QStringLiteral("normal"));
        element.setAttribute(QStringLiteral("visible"), QStringLiteral("1"));
        element.setAttribute(QStringLiteral("filename"), QStringLiteral("layer%1").arg(layer.id));
        element.setAttribute(QStringLiteral("uuid"), layer.uuid.toString());
        element.setAttribute(QStringLiteral("colorlabel"), 5);
        if (layer.kind == LayerKind::Group) element.appendChild(xml.createElement(QStringLiteral("layers")));
        elements.insert(layer.uuid, element);
    }
    for (const auto &layer : layers) {
        auto parent = layer.parentUuid.isNull() ? root : elements.value(layer.parentUuid).firstChildElement(QStringLiteral("layers"));
        parent.appendChild(elements.value(layer.uuid));
    }
    const auto description = xml.toByteArray();
    const auto metadata = Kra::RasterCodec::validateMetadata(description, {});
    QVERIFY2(metadata.code == Kra::Status::Success, qPrintable(metadata.diagnostic));
    QByteArray record;
    QDataStream stream(&record, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << quint32(0x4c504744) << quint16(1);
    for (const auto &field : {model, QByteArray{}, description}) {
        stream << quint32(field.size());
        stream.writeRawData(field.constData(), field.size());
    }
    stream << quint32(0);
    record.append(QCryptographicHash::hash(record, QCryptographicHash::Sha256));
    Execution execution(m_device, 128 * 1024 * 1024, 2);
    PaintDocument restored(execution, QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(record), Result::Success);
    QCOMPARE(restored.image()->operations().snapshot()->size, QSize(32, 16));
    const auto restoredLayers = restored.image()->operations().snapshot()->layers;
    for (const auto &layer : restoredLayers) QCOMPARE(layer.colorLabelIndex, 5);
    QCOMPARE(restoredLayers[1].parentUuid, restoredLayers[2].uuid);
    QCOMPARE(restoredLayers[2].parentUuid, restoredLayers[3].uuid);
    QCOMPARE(restored.image()->operations().undoCount(), 0);
    QVERIFY(!restored.image()->operations().isModified());
    QCOMPARE(restored.image()->writeLayerColorLabel(restoredLayers[0].id, 0), Result::Success);
    const auto current = restored.recoveryData();
    PaintDocument reopened(execution, QSize(1, 1));
    QCOMPARE(reopened.restoreRecovery(current), Result::Success);
    QCOMPARE(reopened.image()->operations().snapshot()->layers[0].colorLabelIndex, 0);
    QCOMPARE(reopened.image()->operations().snapshot()->layers[1].colorLabelIndex, 5);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path());
    QVERIFY(QDir().mkpath(destination));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Recovered color-label save retained at" << destination;
        }
    });
    const auto path = QDir(destination).filePath(QStringLiteral("legacy-color-labels.kra"));
    const auto saved = reopened.saveKra(path);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.size(), restoredLayers.size());
    for (const auto &layer : loaded.document.image.layers)
        QCOMPARE(layer.colorLabelIndex, layer.uuid == restoredLayers[0].uuid ? 0 : 5);
}

void KisGpuPaintDocumentTest::recoveryKeepsHistoryPendingInputAndAssociatedFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Kra::Document values;
    values.image.size = {64, 64};
    values.image.layers = {{QStringLiteral("Layer"), true, QUuid::createUuid(), {}}};
    values.files.insert(QStringLiteral("notes/retained.bin"), QByteArray::fromHex("ff002137"));
    QImage image(values.image.size, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    const auto path = directory.filePath(QStringLiteral("original.kra"));
    QFile output(path);
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(Kra::RasterCodec::write(output, values, image).code, Kra::Status::Success);
    output.close();
    Execution execution(m_device, 128 * 1024 * 1024, 2);
    auto opened = PaintDocument::openKra(execution, path);
    QVERIFY2(opened.document, qPrintable(opened.diagnostic));
    auto &source = *opened.document;
    dot(*source.image(), {8, 12}, {255, 0, 0, 255});
    dot(*source.image(), {24, 12}, {0, 0, 255, 255});
    QCOMPARE(source.image()->undo(), Result::Success);
    Brush green;
    green.rgba = {0, 255, 0, 255};
    const auto pending = source.image()->beginStroke(source.image()->operations().activeLayer(), green, 0);
    QCOMPARE(source.image()->appendSamples(pending, {{{40, 12}, 1}}), Result::Success);
    const auto before = execution.readPixels(source.image()->handle(), source.image()->operations().preview(), {0, 0, 64, 64});
    QCOMPARE(before.code, ExecutionCode::Success);
    const auto data = source.recoveryData();
    QVERIFY(!data.isEmpty());
    PaintDocument restored(execution, QSize(8, 8));
    const auto handle = restored.image()->handle();
    const auto previous = restored.image()->operations().snapshot();
    for (const auto &replacement : {qMakePair(QByteArrayLiteral("notes/retained.bin"), QByteArrayLiteral("../xx/retained.bin")),
                                    qMakePair(QByteArrayLiteral("paintlayer"), QByteArrayLiteral("invalidnod"))}) {
        auto corrupt = data;
        const auto position = corrupt.indexOf(replacement.first);
        QVERIFY(position >= 0);
        QCOMPARE(replacement.first.size(), replacement.second.size());
        corrupt.replace(position, replacement.first.size(), replacement.second);
        corrupt.replace(corrupt.size() - 32, 32, QCryptographicHash::hash(
            QByteArrayView(corrupt.constData(), corrupt.size() - 32), QCryptographicHash::Sha256));
        QCOMPARE(restored.restoreRecovery(corrupt), Result::InvalidRecovery);
        QCOMPARE(restored.image()->operations().snapshot(), previous);
        QCOMPARE(execution.requested(handle), previous);
    }
    QCOMPARE(restored.restoreRecovery(data), Result::Success);
    QCOMPARE(restored.image()->handle(), handle);
    QCOMPARE(restored.filePath(), path);
    QCOMPARE(restored.image()->operations().undoCount(), 1);
    QCOMPARE(restored.image()->operations().redoCount(), 1);
    QVERIFY(restored.image()->operations().isModified());
    QVERIFY(restored.image()->operations().hasActiveStroke());
    QCOMPARE(restored.image()->appendSamples(pending, {{{48, 12}, 1}}), Result::StaleOperation);
    QCOMPARE(restored.saveKra(path, previous).code, FileCode::InvalidData);
    const auto snapshot = restored.image()->operations().preview();
    QCOMPARE(execution.requested(handle), snapshot);
    const auto after = execution.readPixels(handle, snapshot, {0, 0, 64, 64});
    QCOMPARE(after.code, ExecutionCode::Success);
    QCOMPARE(after.tiles.first().rgba, before.tiles.first().rgba);
    QCOMPARE(restored.image()->cancelStroke(restored.image()->operations().activeOperation()), Result::Success);
    QCOMPARE(restored.image()->redo(), Result::Success);
    const auto recoveredPath = directory.filePath(QStringLiteral("recovered.kra"));
    QCOMPARE(restored.saveKra(recoveredPath).code, FileCode::Success);
    QFile input(recoveredPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto decoded = Kra::RasterCodec::read(input);
    QCOMPARE(decoded.code, Kra::Status::Success);
    QCOMPARE(decoded.document.files.value(QStringLiteral("notes/retained.bin")), values.files.value(QStringLiteral("notes/retained.bin")));
    QCOMPARE(decoded.document.image.layers.first().uuid, values.image.layers.first().uuid);
    QCOMPARE(pixel(execution.readPixels(handle, restored.image()->operations().snapshot(), {0, 0, 64, 64}), 0, {24, 12}), QByteArray::fromHex("0000ffff"));
}

void KisGpuPaintDocumentTest::invalidRecoveryPreservesTheAttachedDocument()
{
    Execution execution(m_device);
    PaintDocument document(execution, QSize(64, 64));
    dot(*document.image(), {8, 12}, {255, 0, 0, 255});
    const auto snapshot = document.image()->operations().snapshot();
    const auto data = document.recoveryData();
    QVERIFY(!data.isEmpty());
    for (int change = 0; change < 4; ++change) {
        auto corrupt = data;
        if (change == 0) corrupt.chop(1);
        if (change == 1) corrupt[corrupt.size() - 1] ^= 1;
        if (change >= 2) {
            if (change == 2) for (int i = 6; i < 10; ++i) corrupt[i] = char(0xff);
            if (change == 3) { corrupt[4] = char(0x7f); corrupt[5] = char(0xff); }
            const auto digest = QCryptographicHash::hash(QByteArrayView(corrupt.constData(), corrupt.size() - 32), QCryptographicHash::Sha256);
            corrupt.replace(corrupt.size() - 32, 32, digest);
        }
        QCOMPARE(document.restoreRecovery(corrupt), Result::InvalidRecovery);
        QCOMPARE(document.image()->operations().snapshot(), snapshot);
        QCOMPARE(execution.requested(document.image()->handle()), snapshot);
        QCOMPARE(document.image()->operations().undoCount(), 1);
    }
    document.close();
    QCOMPARE(document.restoreRecovery(data), Result::Closed);
    QVERIFY(document.recoveryData().isEmpty());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuPaintDocumentTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPaintDocumentTest.moc"
