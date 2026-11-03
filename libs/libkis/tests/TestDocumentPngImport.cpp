/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestDocument.h"
#include <simpletest.h>
#include <Document.h>
#include <Node.h>
#include <InfoObject.h>
#include <application/kis_config.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kis_paint_device.h>
#include <kis_png_converter.h>
#include <KoColorProfile.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <QDateTime>
#include <QDir>
#include <QBuffer>
#include <QSignalSpy>
#include <kundo2stack.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_layer.h>
#include <metadata/kis_meta_data_store.h>
#include <metadata/kis_meta_data_value.h>
#include <QDomDocument>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>

namespace {
// Fixed 2 x 1 RGBA8 PNGs with the same opaque pixels. The variants have
// absent pHYs, unknown pHYs units, and metre units, respectively.
const QByteArray resolutionPngs[] = {
    QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a"
                        "0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049454e44ae426082"),
    QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a"
                        "000000097048597300002e230000171200e245d64d"
                        "0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049454e44ae426082"),
    QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a"
                        "000000097048597300002e2300001712019542e6db"
                        "0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049454e44ae426082")
};
// Fixed PNG chunks and zlib rows; metadata is a TIFF Make XMP attribute.
const QByteArray metadataPng = QByteArray::fromHex(
    "89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a00000015744558745469746c6500466978"
    "656420504e47207469746c65bb093ecd000001e8744558745261772070726f66696c65207479706520786d70000a786d700a"
    "20202020203232360a3363373833613738366437303664363537343631323037383664366336653733336137383364323236"
    "3136343666363236353361366537333361366436353734363132663232336533633732363436363361353234343436323037"
    "3836643663366537333361373236343636336432323638373437343730336132663266373737373737326537373333326536"
    "6637323637326633313339333933393266333033323266333233323264373236343636326437333739366537343631373832"
    "6436653733323332323365336337323634363633613434363537333633373236393730373436393666366532303732363436"
    "3633613631363236663735373433643232323232303738366436633665373333613734363936363636336432323638373437"
    "3437303361326632663665373332653631363436663632363532653633366636643266373436393636363632663331326533"
    "3032663232323037343639363636363361346436313662363533643232343636393738363536343230353034653437323036"
    "3336313664363537323631323232663365336332663732363436363361353234343436336533633266373833613738366437"
    "30366436353734363133650a5bbec34d0000001149444154789c63105432feef1090f01f000b5b0355651e6f7f0000000049"
    "454e44ae426082");

// Fixed PNG chunks and zlib rows; metadata is a TIFF Make XMP attribute.
const QByteArray hiddenColorPng = QByteArray::fromHex(
    "89504e470d0a1a0a0000000d4948445200000002000000010806000000f4227f8a0000001149444154789c631032096358b5"
    "fb5d3d000b3d036f15e6d7180000000049454e44ae426082");

// Fixed PNG chunks and zlib rows; metadata is a TIFF Make XMP attribute.
const QByteArray palettePng = QByteArray::fromHex(
    "89504e470d0a1a0a0000000d4948445200000002000000010803000000c3fc8fb800000006504c5445123456aabbeec5d234"
    "dd0000000274524e53007fb629a1950000000b49444154789c636060040000040002bf7a3f4a0000000049454e44ae426082");

}

void TestDocument::testPngWithoutPhysicalResolutionKeepsImageDefault_data()
{
    QTest::addColumn<bool>("gpuSession");
    QTest::addColumn<int>("precedingPng");
    for (bool gpu : {false, true}) {
        for (int preceding = 0; preceding < 3; ++preceding) {
            const QByteArray name = QByteArray(gpu ? "gpu-session-" : "cpu-session-") +
                QList<QByteArray>{"first-open", "after-unknown-units", "after-metre-units"}[preceding];
            QTest::newRow(name.constData()) << gpu << preceding;
        }
    }
}

void TestDocument::testPngWithoutPhysicalResolutionKeepsImageDefault()
{
    QFETCH(bool, gpuSession);
    QFETCH(int, precedingPng);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpuSession ? "1" : "0");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const auto open = [&](int index) {
        const auto path = directory.filePath(QStringLiteral("resolution-%1.png").arg(index));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(resolutionPngs[index]) != resolutionPngs[index].size()) return false;
        file.close();
        return document->openPath(path, KisDocument::DontAddToRecent);
    };
    if (precedingPng) QVERIFY2(open(precedingPng), qPrintable(document->errorMessage()));
    QVERIFY2(open(0), qPrintable(document->errorMessage()));
    KisImageSP image = document->image();
    QVERIFY(image);
    image->waitForDone();
    QCOMPARE(image->width(), 2);
    QCOMPARE(image->height(), 1);
    Document api(document.data(), false);
    QCOMPARE(api.pixelData(0, 0, 2, 1), QByteArray::fromHex("332211ff605040ff"));
    QCOMPARE(image->xRes(), 1.0);
    QCOMPARE(image->yRes(), 1.0);
}

void TestDocument::testGpuSessionOpensPngWithGpuPixelOwnership_data()
{
    QTest::addColumn<QByteArray>("inputPng");
    QTest::addColumn<bool>("compressRaster");
    QTest::addColumn<QByteArray>("sourcePixels");
    QTest::newRow("without-physical-metadata") << resolutionPngs[0] << false << QByteArray();
    QTest::newRow("with-physical-metadata") << resolutionPngs[2] << false << QByteArray();
    QTest::newRow("hidden-rgb-and-partial-alpha") << hiddenColorPng << false << QByteArray();
    QTest::newRow("palette-transparency") << palettePng << false << QByteArray();
    QTest::newRow("layer-metadata") << metadataPng << false << QByteArray();
    QTest::newRow("compressed-native-checkpoints") << resolutionPngs[0] << true << QByteArray();
    // Frozen PNG Suite inputs from plugins/impex/png/tests/data/sources/.
    // Keeping these small encoded inputs here also serves installed test executables.
    QTest::newRow("basn6a08") << QByteArray::fromHex(
        "89504e470d0a1a0a0000000d4948445200000020000000200806000000737a7af40000000467414d41000186a031e8965f00"
        "00006f49444154789cedd6310a80300c46e12764684fa1f73f55048f21c4ddc545781d52e85028fc1f4d28d98a01305e7b7e"
        "9cffba33831d75054703ca06a8f90d58a0074e351e227d805c8254e31bb0420f5cdc2e0079208892ffe2a00136a07b400794"
        "3c1004d900195036407f011bf00052201a9c160fb84c0000000049454e44ae426082") << false << QByteArray();
    QTest::newRow("basi3p01") << QByteArray::fromHex(
        "89504e470d0a1a0a0000000d49484452000000200000002001030000013eb3d8210000000467414d41000186a031e8965f00"
        "000006504c5445eeff222266ff6c1ad2260000002949444154789c636080820f50b80a0a43d1b0b131c39933d84908834836"
        "3f1040880f4000212810030009ba422d76a9c39e0000000049454e44ae426082") << false << QByteArray();

    // Native RGB16 encoding fixes lower bits, transparent RGB and boundaries
    // across both the CPU's 64-pixel and GPU's 256-pixel tiles.
    const auto cs = KoColorSpaceRegistry::instance()->rgb16();
    QVERIFY(cs);
    const QSize size(257, 259);
    for (bool alpha : {false, true}) for (bool interlaced : {false, true}) {
        QByteArray pixels(size.width() * size.height() * cs->pixelSize(), Qt::Uninitialized);
        auto *words = reinterpret_cast<quint16 *>(pixels.data());
        const quint16 alphas[] = {0, 1, 32768, 65535};
        for (int i = 0; i < size.width() * size.height(); ++i) {
            words[i * 4] = quint16(0x1357 + i * 317);
            words[i * 4 + 1] = quint16(0x4321 + i * 719);
            words[i * 4 + 2] = quint16(0x9876 + i * 1237);
            words[i * 4 + 3] = alpha ? alphas[i % 4] : 65535;
        }
        KisPaintDeviceSP device = new KisPaintDevice(cs);
        device->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), QRect(QPoint(), size));
        KisPNGOptions options;
        options.alpha = alpha;
        options.tryToSaveAsIndexed = false;
        options.saveSRGBProfile = true;
        options.interlace = interlaced;
        options.compression = 3;
        vKisAnnotationSP annotations;
        QBuffer encoded;
        QVERIFY(encoded.open(QIODevice::ReadWrite));
        KisPNGConverter writer(nullptr, true);
        const auto result = writer.buildFile(&encoded, QRect(QPoint(), size), 1, 1, device,
                                             annotations.begin(), annotations.end(), options, nullptr);
        QVERIFY2(result.isOk(), qPrintable(result.errorMessage()));
        const QByteArray name = QByteArray(alpha ? "rgba16" : "rgb16") +
            (interlaced ? "-adam7" : "-ordinary");
        QTest::newRow(name.constData()) << encoded.data() << interlaced << pixels;
    }
}

void TestDocument::testGpuSessionOpensPngWithGpuPixelOwnership()
{
    QFETCH(QByteArray, inputPng);
    QFETCH(bool, compressRaster);
    QFETCH(QByteArray, sourcePixels);
    const bool previousCompression = KisConfig(true).compressKra();
    const auto restoreCompression = qScopeGuard([&] { KisConfig(false).setCompressKra(previousCompression); });
    KisConfig(false).setCompressKra(compressRaster);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "PNG document artifacts retained at" << directory.path();
        }
    });
    const auto source = directory.filePath(QStringLiteral("source.png"));
    QFile file(source);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(inputPng), inputPng.size());
    file.close();

    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    reference->setFileBatchMode(true);
    reference->setAutoSaveActive(false);
    QVERIFY2(reference->openPath(source, KisDocument::DontAddToRecent), qPrintable(reference->errorMessage()));
    KisImageSP referenceImage = reference->image();
    QVERIFY(referenceImage);
    referenceImage->waitForDone();
    QScopedPointer<Node> referenceNode(Node::createNode(referenceImage, referenceImage->root()->firstChild()));
    const auto size = referenceImage->size();
    const auto expectedPixels = referenceNode->pixelData(0, 0, size.width(), size.height());
    const auto expectedProjection = Document(reference.data(), false).pixelData(0, 0, size.width(), size.height());
    if (!sourcePixels.isEmpty()) QCOMPARE(expectedPixels, sourcePixels);
    else if (inputPng == hiddenColorPng) QCOMPARE(expectedPixels, QByteArray::fromHex("56341200eebbaa7f"));
    else if (inputPng == palettePng) QCOMPARE(expectedPixels, QByteArray::fromHex("00000000eebbaa7f"));
    else if (size == QSize(2, 1)) QCOMPARE(expectedPixels, QByteArray::fromHex("332211ff605040ff"));

    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY2(document->openPath(source, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    KisImageSP image = document->image();
    QVERIFY(image);
    QVERIFY2(image->gpuImage(), "Ordinary PNG Open in a GPU session must use GPU pixel ownership");
    QCOMPARE(image->size(), referenceImage->size());
    QCOMPARE(image->xRes(), referenceImage->xRes());
    QCOMPARE(image->yRes(), referenceImage->yRes());
    QCOMPARE(image->colorSpace()->id(), referenceImage->colorSpace()->id());
    QCOMPARE(image->colorSpace()->profile()->rawData(), referenceImage->colorSpace()->profile()->rawData());
    QScopedPointer<Node> node(Node::createNode(image, image->root()->firstChild()));
    QCOMPARE(node->name(), referenceNode->name());
    QCOMPARE(node->pixelData(0, 0, size.width(), size.height()), expectedPixels);
    QCOMPARE(node->uniqueId(), referenceNode->uniqueId());
    Document api(document.data(), false);
    QCOMPARE(api.pixelData(0, 0, size.width(), size.height()), expectedProjection);
    QCOMPARE(image->root()->firstChild()->isPinnedToTimeline(), referenceImage->root()->firstChild()->isPinnedToTimeline());
    if (inputPng == metadataPng) {
        QCOMPARE(document->documentInfo()->aboutInfo("title"), QStringLiteral("Fixed PNG title"));
        auto *store = qobject_cast<KisLayer *>(image->root()->firstChild().data())->metaData();
        QCOMPARE(store->getValue(QStringLiteral("http://ns.adobe.com/tiff/1.0/"), QStringLiteral("Make")).asVariant().toString(),
                 QStringLiteral("Fixed PNG camera"));
    }

    reference->setColorHistoryColors({});
    document->setColorHistoryColors({});
    reference->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    document->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    QDomDocument fixedInfo;
    QVERIFY(fixedInfo.setContent(QByteArrayLiteral("<document-info><about><title>Fixed PNG import</title>"
        "<creation-date>2000-01-02T03:04:05Z</creation-date><date>2001-02-03T04:05:06Z</date>"
        "<editing-cycles>7</editing-cycles><editing-time>10</editing-time><language>en</language></about>"
        "<author><full-name>LibrePaint fixture</full-name></author></document-info>")));
    QVERIFY(document->documentInfo()->load(fixedInfo));
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    const auto destination = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", directory.path())).filePath(
        QStringLiteral("png-import-%1").arg(QString::fromLatin1(QTest::currentDataTag())));
    QVERIFY(QDir().mkpath(destination));
    const auto native = QDir(destination).filePath(QStringLiteral("gpu.kra"));
    QVERIFY2(document->saveAs(native, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(native, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    QVERIFY(reopened->image()->gpuImage());
    QScopedPointer<Node> savedNode(Node::createNode(reopened->image(), reopened->image()->root()->firstChild()));
    QCOMPARE(savedNode->uniqueId(), node->uniqueId());
    QCOMPARE(savedNode->pixelData(0, 0, size.width(), size.height()), expectedPixels);
    QCOMPARE(reopened->image()->xRes(), image->xRes());
    QCOMPARE(reopened->image()->yRes(), image->yRes());
    if (inputPng == metadataPng) {
        auto *store = qobject_cast<KisLayer *>(reopened->image()->root()->firstChild().data())->metaData();
        QCOMPARE(store->getValue(QStringLiteral("http://ns.adobe.com/tiff/1.0/"), QStringLiteral("Make")).asVariant().toString(),
                 QStringLiteral("Fixed PNG camera"));
    }

    QVERIFY(reference->documentInfo()->load(fixedInfo));
    reference->setPreActivatedNode(referenceImage->root()->firstChild());
    reference->setColorHistoryColors({});
    document->setColorHistoryColors({});
    const auto cpuNative = QDir(destination).filePath(QStringLiteral("cpu.kra"));
    QVERIFY2(reference->saveAs(cpuNative, KisDocument::nativeFormatMimeType(), false, settings), qPrintable(reference->errorMessage()));
    reference->waitForSavingToComplete();
    // Compare complete files after fixing input, selection, document metadata,
    // save time and settings. Archive metadata differences remain observable.
    const auto read = [&](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    for (const auto &format : {QStringLiteral("png"), QStringLiteral("jpg")}) {
        const auto cpuPath = QDir(destination).filePath(QStringLiteral("cpu.") + format);
        const auto gpuPath = QDir(destination).filePath(QStringLiteral("gpu.") + format);
        Document referenceApi(reference.data(), false);
        InfoObject outputSettings;
        outputSettings.setProperty("forceSRGB", false);
        outputSettings.setProperty("saveSRGBProfile", true);
        outputSettings.setProperty("alpha", true);
        outputSettings.setProperty("compression", 3);
        outputSettings.setProperty("quality", 90);
        outputSettings.setProperty("storeMetaData", true);
        outputSettings.setProperty("storeAuthor", false);
        outputSettings.setProperty("exif", true);
        outputSettings.setProperty("iptc", true);
        outputSettings.setProperty("xmp", true);
        QVERIFY(referenceApi.exportImage(cpuPath, outputSettings));
        QVERIFY(api.exportImage(gpuPath, outputSettings));
        QCOMPARE(read(gpuPath), read(cpuPath));
    }
    QVERIFY(!read(native).isEmpty());
    QCOMPARE(read(native), read(cpuNative));

    // A file imported and saved by the ordinary document path remains editable.
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto owner = reopened->image()->gpuImage();
    const bool sixteen = !sourcePixels.isEmpty();
    Gpu::Brush brush;
    brush.diameter = 4;
    brush.rgba = sixteen ? Gpu::RgbaPixel{0x113f, 0x2559, 0x8183, 65535} : Gpu::RgbaPixel{17, 37, 129, 255};
    const auto stroke = owner->beginStroke(owner->operations().activeLayer(), brush, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{0.5, 0.5}, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    const auto painted = savedNode->pixelData(0, 0, size.width(), size.height());
    QCOMPARE(savedNode->pixelData(0, 0, 1, 1), QByteArray::fromHex(sixteen ? "838159253f11ffff" : "812511ff"));
    QVERIFY(painted != expectedPixels);
    reopened->undoStack()->undo();
    QCOMPARE(savedNode->pixelData(0, 0, size.width(), size.height()), expectedPixels);
    reopened->undoStack()->redo();
    QCOMPARE(savedNode->pixelData(0, 0, size.width(), size.height()), painted);
    const auto editedPath = QDir(destination).filePath(QStringLiteral("edited.kra"));
    QVERIFY(reopened->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false, settings));
    reopened->waitForSavingToComplete();
    QScopedPointer<KisDocument> after(KisPart::instance()->createDocument());
    after->setFileBatchMode(true);
    after->setAutoSaveActive(false);
    QVERIFY2(after->openPath(editedPath, KisDocument::DontAddToRecent), qPrintable(after->errorMessage()));
    QVERIFY(after->image()->gpuImage());
    QScopedPointer<Node> afterNode(Node::createNode(after->image(), after->image()->root()->firstChild()));
    QCOMPARE(afterNode->pixelData(0, 0, size.width(), size.height()), painted);
    QCOMPARE(afterNode->uniqueId(), savedNode->uniqueId());
}

void TestDocument::testGpuPngRejectedOpenKeepsDocument_data()
{
    QTest::addColumn<QByteArray>("inputPng");
    QTest::newRow("signature") << QByteArrayLiteral("not a PNG");
    QTest::newRow("truncated-pixels") << resolutionPngs[0].left(50);
    QTest::newRow("truncated-end-after-metadata") << metadataPng.left(metadataPng.size() - 8);
    QTest::newRow("unsupported-gray16") << QByteArray::fromHex("89504e470d0a1a0a0000000d494844520000000200000001100000000081d9fc15000000004944415435af061e0000000049454e44ae426082");
    QTest::newRow("raster-capacity") << QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200008000000080000806000000c47ca37f000000004944415435af061e0000000049454e44ae426082");
    QTest::newRow("word-raster-capacity-before-rows") << QByteArray::fromHex("89504e470d0a1a0a0000000d4948445200002000000020001006000000223a161a000000004944415435af061e0000000049454e44ae426082");
    QTest::newRow("dimension-limit-before-rows") << QByteArray::fromHex("89504e470d0a1a0a0000000d494844521000000000000001080600000044d0096d000000004944415435af061e0000000049454e44ae426082");
}

void TestDocument::testGpuPngRejectedOpenKeepsDocument()
{
    QFETCH(QByteArray, inputPng);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto write = [&](const QString &name, const QByteArray &bytes) {
        QFile file(directory.filePath(name));
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    QVERIFY(write(QStringLiteral("source.png"), resolutionPngs[0]));
    QVERIFY(write(QStringLiteral("rejected.png"), inputPng));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const auto source = directory.filePath(QStringLiteral("source.png"));
    QVERIFY(document->openPath(source, KisDocument::DontAddToRecent));
    KisImageSP image = document->image();
    const auto owner = image->gpuImage();
    QVERIFY(owner);
    const auto layer = owner->operations().activeLayer();
    LibrePaint::Image::Gpu::Brush brush;
    const auto stroke = owner->beginStroke(layer, brush, 17);
    QVERIFY(!stroke.isNull());
    QVERIFY(owner->appendSamples(stroke, {{{0.5, 0.5}, 1, 0, 0, 0, 10}}) == LibrePaint::Image::Gpu::Result::Success);
    QVERIFY(owner->commitStroke(stroke) == LibrePaint::Image::Gpu::Result::Success);
    const auto state = owner->operations().preview();
    const auto path = document->localFilePath();
    const auto mime = document->mimeType();
    const auto undo = document->undoStack()->count();
    const auto modified = document->isModified();
    document->documentInfo()->setAboutInfo("title", QStringLiteral("Previous document title"));
    QSignalSpy changed(document.data(), &KisDocument::sigCurrentImageChanged);
    QVERIFY(!document->openPath(directory.filePath(QStringLiteral("rejected.png")), KisDocument::DontAddToRecent));
    QCOMPARE(KisImageSP(document->image()), image);
    QCOMPARE(owner->operations().preview(), state);
    QCOMPARE(document->documentInfo()->aboutInfo("title"), QStringLiteral("Previous document title"));
    QCOMPARE(document->localFilePath(), path);
    QCOMPARE(document->mimeType(), mime);
    QCOMPARE(document->undoStack()->count(), undo);
    QCOMPARE(document->isModified(), modified);
    QCOMPARE(changed.count(), 0);
}
