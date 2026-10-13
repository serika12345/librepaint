/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuExecution.h>
#include <kra/KisKraRasterCodec.h>
#include <QBuffer>
#include <QColorSpace>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <rhi/qrhi.h>
#include <quazip.h>
#include <quazipfile.h>

using namespace Krita::Image::Gpu;
using namespace Krita::Document::Gpu;
namespace Kra = Krita::ImportExport::Kra;

namespace {
void dot(PaintDocument &document, QPointF position, std::array<quint8, 4> color)
{
    Brush brush;
    brush.rgba = color;
    brush.diameter = 6;
    const auto id = document.beginStroke(document.operations().activeLayer(), brush, 0);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{position, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
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
    void acceptedChangesUpdateOneAttachedDocument();
    void pinnedSavePreservesLaterEditsAndHistory();
    void rasterImportAndSaveRetainOffCanvasHiddenPixelsAndMetadata();
    void rejectedSavesPreserveTheFileIdentityAndHistory();
    void rejectedOpenPreservesAttachedDocuments();
    void notificationsAllowSynchronousCloseAndDeletion();
private:
    DevicePtr m_device;
};

void KisGpuPaintDocumentTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
    qInfo() << "GPU backend:" << m_device->rhi().backendName();
}

void KisGpuPaintDocumentTest::acceptedChangesUpdateOneAttachedDocument()
{
    Execution execution(m_device);
    DocumentHandle handle = 0;
    Snapshot retained;
    {
        PaintDocument document(execution, QSize(512, 64));
        handle = document.handle();
        QVERIFY(handle);
        QCOMPARE(execution.requested(handle), document.operations().snapshot());
        QSignalSpy changed(&document, &PaintDocument::changed);
        const auto initial = document.operations().snapshot();
        Brush brush;
        auto stroke = document.beginStroke(document.operations().activeLayer(), brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
        QCOMPARE(execution.requested(handle), document.operations().preview());
        QCOMPARE(changed.size(), 1);
        const auto preview = document.operations().preview();
        QCOMPARE(document.commitStroke(stroke), Result::Success);
        QCOMPARE(document.operations().snapshot(), preview);
        QCOMPARE(execution.requested(handle), preview);
        QCOMPARE(document.undo(), Result::Success);
        QCOMPARE(execution.requested(handle), initial);
        QCOMPARE(document.redo(), Result::Success);
        QCOMPARE(execution.requested(handle), preview);
        stroke = document.beginStroke(document.operations().activeLayer(), brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{264, 12}, 1}}), Result::Success);
        QCOMPARE(document.cancelStroke(stroke), Result::Success);
        QCOMPARE(execution.requested(handle), preview);
        const auto layer = document.addLayer(QStringLiteral("Second"));
        QVERIFY(layer);
        QCOMPARE(document.setLayerVisible(layer, false), Result::Success);
        QCOMPARE(execution.requested(handle), document.operations().snapshot());
        retained = document.operations().snapshot();
    }
    QVERIFY(!execution.requested(handle));
    QCOMPARE(execution.pendingDocuments(), 0);
    QCOMPARE(retained->layers.size(), 2);
}

void KisGpuPaintDocumentTest::pinnedSavePreservesLaterEditsAndHistory()
{
    Execution execution(m_device);
    PaintDocument document(execution, QSize(512, 64));
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.operations().snapshot();
    dot(document, {264, 12}, {0, 0, 255, 255});
    const auto latest = document.operations().snapshot();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("pinned.kra"));
    const auto saved = document.saveKra(path, pinned);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QCOMPARE(saved.version, pinned);
    QCOMPARE(document.filePath(), path);
    QVERIFY(document.operations().isModified());
    QCOMPARE(document.operations().snapshot(), latest);
    QCOMPARE(execution.requested(document.handle()), latest);
    QCOMPARE(document.operations().undoCount(), 2);
    const auto merged = mergedImage(path);
    QCOMPARE(merged.size(), pinned->size);
    QCOMPARE(merged.pixelColor(8, 12), QColor(255, 0, 0, 255));
    QCOMPARE(merged.pixelColor(264, 12), QColor(0, 0, 0, 0));
    auto opened = PaintDocument::openKra(execution, path);
    QVERIFY2(opened.code == FileCode::Success, qPrintable(opened.diagnostic));
    QVERIFY(opened.document);
    QVERIFY(!opened.document->operations().isModified());
    QCOMPARE(opened.document->operations().undoCount(), 0);
    const auto image = opened.document->operations().snapshot();
    const auto read = execution.readPixels(opened.document->handle(), image, {0, 0, 512, 64}, {image->activeLayer});
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(pixel(read, image->activeLayer, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QVERIFY(pixel(read, image->activeLayer, {264, 12}).isEmpty() || pixel(read, image->activeLayer, {264, 12}) == QByteArray(4, '\0'));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.operations().isModified());
    const auto alternate = directory.filePath(QStringLiteral("alternate.kra"));
    QCOMPARE(document.saveKra(alternate).code, FileCode::Success);
    QCOMPARE(document.filePath(), alternate);
    QCOMPARE(document.operations().redoCount(), 1);
    QVERIFY(!fileBytes(path).isEmpty());
}

void KisGpuPaintDocumentTest::rasterImportAndSaveRetainOffCanvasHiddenPixelsAndMetadata()
{
    Kra::Document source;
    source.image.size = {100, 100};
    source.image.activeLayer = 1;
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
    const auto snapshot = opened.document->operations().snapshot();
    QCOMPARE(opened.document->selectLayer(snapshot->layers[0].id), Result::Success);
    dot(*opened.document, {8, 12}, {255, 0, 0, 255});
    const auto savedPath = directory.filePath(QStringLiteral("saved.kra"));
    const auto saved = opened.document->saveKra(savedPath);
    QVERIFY2(saved.code == FileCode::Success, qPrintable(saved.diagnostic));
    QFile input(savedPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    const auto decoded = Kra::RasterCodec::read(input);
    QVERIFY2(decoded.code == Kra::Status::Success, qPrintable(decoded.diagnostic));
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
    dot(document, {8, 12}, {255, 0, 0, 255});
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("existing.kra"));
    QCOMPARE(document.saveKra(path).code, FileCode::Success);
    const auto bytes = fileBytes(path);
    const auto saved = document.operations().snapshot();
    Brush brush;
    const auto stroke = document.beginStroke(saved->activeLayer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{24, 12}, 1}}), Result::Success);
    QCOMPARE(document.saveKra(path, document.operations().preview()).code, FileCode::InvalidData);
    QCOMPARE(fileBytes(path), bytes);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    dot(document, {24, 12}, {0, 0, 255, 255});
    const auto latest = document.operations().snapshot();
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(document.saveKra(path).code, FileCode::Busy);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(document.saveKra(directory.filePath(QStringLiteral("missing/failure.kra"))).code, FileCode::IoFailure);
    OperationDocument foreign(QSize(64, 64));
    QCOMPARE(document.saveKra(path, foreign.snapshot()).code, FileCode::InvalidData);
    QCOMPARE(fileBytes(path), bytes);
    QCOMPARE(document.filePath(), path);
    QCOMPARE(document.operations().snapshot(), latest);
    QCOMPARE(document.operations().undoCount(), 2);
    QVERIFY(document.operations().isModified());
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
    QCOMPARE(execution.requested(document.handle()), document.operations().snapshot());
    const auto absent = PaintDocument::openKra(execution, directory.filePath(QStringLiteral("absent.kra")));
    QCOMPARE(absent.code, FileCode::IoFailure);
    QVERIFY(!absent.document);
}

void KisGpuPaintDocumentTest::notificationsAllowSynchronousCloseAndDeletion()
{
    Execution execution(m_device);
    PaintDocument closing(execution, QSize(64, 64));
    const auto handle = closing.handle();
    connect(&closing, &PaintDocument::changed, &closing, [&] { if (closing.handle()) closing.close(); });
    const auto stroke = closing.beginStroke(closing.operations().activeLayer(), {}, 0);
    QCOMPARE(closing.appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
    QCOMPARE(closing.handle(), 0);
    QVERIFY(!execution.requested(handle));

    auto deleting = std::make_unique<PaintDocument>(execution, QSize(64, 64));
    const auto second = deleting->handle();
    const auto next = deleting->beginStroke(deleting->operations().activeLayer(), {}, 0);
    connect(deleting.get(), &PaintDocument::changed, this, [&] { deleting.reset(); });
    QCOMPARE(deleting->appendSamples(next, {{{8, 12}, 1}}), Result::Success);
    QVERIFY(!deleting);
    QVERIFY(!execution.requested(second));
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuPaintDocumentTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPaintDocumentTest.moc"
