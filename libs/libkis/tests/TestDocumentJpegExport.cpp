/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestDocument.h"
#include <testui.h>
#include <Document.h>
#include <InfoObject.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/kis_config.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_properties_configuration.h>
#include <kis_config_widget.h>
#include <kra/KisKraRasterCodec.h>
#include <metadata/KoDocumentInfo.h>
#include <QColorSpace>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QDialog>
#include <QDomDocument>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtEndian>
#include <iterator>
#include <utility>

namespace {
namespace Gpu = LibrePaint::Image::Gpu;

const char *const jpegOptions[] = {"defaults", "quality-zero", "quality-100", "progressive", "subsampling-1",
    "subsampling-2", "subsampling-3", "extended-quality", "unoptimized", "smooth-50", "smooth-100",
    "force-srgb", "no-profile", "colored-background", "partial-background", "transparent-background",
    "metadata-all", "metadata-exif", "metadata-iptc", "metadata-xmp", "anonymized-metadata"};

QByteArray jpegPixels(QSize size)
{
    QByteArray rgba(qsizetype(size.width()) * size.height() * 4, '\0');
    for (int y = 0; y < size.height(); ++y) for (int x = 0; x < size.width(); ++x) {
        auto *pixel = rgba.data() + (qsizetype(y) * size.width() + x) * 4;
        pixel[0] = char((x * 11 + y * 19) % 256);
        pixel[1] = char((x * 29 + y * 3) % 256);
        pixel[2] = char((x * 13 + y * 7) % 256);
        pixel[3] = char(((x + y) % 17) * 15);
    }
    return rgba;
}

bool fixedJpegInformation(KisDocument *document)
{
    QDomDocument information;
    if (!information.setContent(QByteArrayLiteral(
        "<document-info><about><title>Fixed title</title><subject>Fixed description</subject>"
        "<keyword>fixed, JPEG</keyword><license>CC0</license><date>2001-02-03T04:05:06Z</date>"
        "</about><author><full-name>Fixture</full-name><contact type=\"email\">paint@example.invalid</contact>"
        "</author></document-info>"))) return false;
    if (!document->documentInfo()->load(information)) return false;
    return document->documentInfo()->authorInfo(QStringLiteral("creator")) == QStringLiteral("Fixture");
}

Gpu::PaintImageSP jpegGpuDocument(KisDocument *document, QSize size, QString &diagnostic)
{
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    if (!execution) return {};
    auto owner = Gpu::PaintImageSP::create(execution, size);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("JPEG export")));
    return owner;
}

void jpegCpuDocument(KisDocument *document, QSize size, const KoColorSpace *colorSpace, const QByteArray &rgba)
{
    KisImageSP image = new KisImage(document->createUndoStore(), size.width(), size.height(), colorSpace, QStringLiteral("JPEG export"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Fixed pixels"), 255);
    QByteArray native = rgba;
    for (qsizetype i = 0; i < native.size(); i += 4) std::swap(native[i], native[i + 2]);
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(native.constData()), 0, 0, size.width(), size.height());
    image->addNode(layer);
    image->setResolution(300.0 / 72, 150.0 / 72);
    image->initialRefreshGraph();
    image->waitForDone();
    document->setCurrentImage(image);
    document->setFileBatchMode(true);
}

KisPropertiesConfigurationSP jpegConfiguration(int option, const KoColorSpace *colorSpace)
{
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    if (option == 1) settings->setProperty("quality", 0);
    if (option == 2 || option == 7) settings->setProperty("quality", 100);
    if (option == 3) settings->setProperty("progressive", true);
    if (option >= 4 && option <= 6) settings->setProperty("subsampling", option - 3);
    if (option == 7) settings->setProperty("baseline", false);
    if (option == 8) settings->setProperty("optimize", false);
    if (option == 9 || option == 10) settings->setProperty("smoothing", option == 9 ? 50 : 100);
    if (option == 11) settings->setProperty("forceSRGB", true);
    if (option == 12) settings->setProperty("saveProfile", false);
    if (option >= 13 && option <= 15) {
        const QColor background(31, 117, 209, option == 14 ? 128 : option == 15 ? 0 : 255);
        settings->setProperty("transparencyFillcolor", QVariant::fromValue(KoColor(background, colorSpace)));
    }
    if (option >= 16) {
        settings->setProperty("storeAuthor", true);
        settings->setProperty("storeMetaData", true);
        settings->setProperty("exif", option == 16 || option == 17 || option == 20);
        settings->setProperty("iptc", option == 16 || option == 18 || option == 20);
        settings->setProperty("xmp", option == 16 || option == 19 || option == 20);
        if (option == 20) settings->setProperty("filters", QStringLiteral("Anonymizer"));
    }
    return settings;
}
}

void TestDocument::testGpuJpegExportPreservesOptionsAndDocument_data()
{
    QTest::addColumn<int>("option");
    QTest::addColumn<bool>("menuExport");
    QTest::addColumn<QString>("blendMode");
    for (int option = 0; option < int(std::size(jpegOptions)); ++option) for (const bool menu : {false, true}) {
        const QByteArray name = (menu ? QByteArrayLiteral("menu-") : QByteArrayLiteral("api-")) + jpegOptions[option];
        QTest::newRow(name.constData()) << option << menu << QString();
    }
    for (const auto &mode : {QStringLiteral("add"), QStringLiteral("linear_dodge")}) for (bool menu : {false, true}) {
        const QByteArray name = QByteArray(menu ? "menu-" : "api-") + mode.toLatin1();
        QTest::newRow(name.constData()) << 0 << menu << mode;
    }
}

void TestDocument::testGpuJpegExportPreservesOptionsAndDocument()
{
    QFETCH(int, option);
    QFETCH(bool, menuExport);
    QFETCH(QString, blendMode);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    const auto remove = qScopeGuard([&] {
        KisPart::instance()->removeDocument(reference.data(), false);
        KisPart::instance()->removeDocument(document.data(), false);
    });
    const QSize size(33, 17);
    QString diagnostic;
    const auto owner = jpegGpuDocument(document.data(), size, diagnostic);
    QVERIFY2(owner, qPrintable(diagnostic));
    document->setFileBatchMode(true);
    KisImageSP image = document->image().toStrongRef();
    const auto rgba = jpegPixels(size);
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, image->bounds(), rgba), Gpu::Result::Success);
    if (!blendMode.isEmpty()) {
        const auto upper = owner->addLayer(QStringLiteral("Addition"));
        QVERIFY(upper);
        QCOMPARE(owner->replaceRgbaPixels(upper, image->bounds(), QByteArray::fromHex("d0e0f089").repeated(size.width() * size.height())), Gpu::Result::Success);
        QCOMPARE(owner->setLayerOpacity(upper, 137), Gpu::Result::Success);
        QCOMPARE(owner->setLayerBlendMode(upper, blendMode == QLatin1String("add") ? Gpu::LayerBlendMode::Addition : Gpu::LayerBlendMode::LinearDodge), Gpu::Result::Success);
    }
    image->setResolution(300.0 / 72, 150.0 / 72);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "JPEG document comparison artifacts retained at" << directory.path();
        }
    });
    const auto nativePath = directory.filePath(QStringLiteral("source.kra"));
    KisPropertiesConfigurationSP nativeSettings(new KisPropertiesConfiguration);
    nativeSettings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    QVERIFY2(document->saveAs(nativePath, KisDocument::nativeFormatMimeType(), false, nativeSettings), qPrintable(document->errorMessage()));
    QFile native(nativePath);
    QVERIFY(native.open(QIODevice::ReadOnly));
    const auto nativeBytes = native.readAll();
    native.close();
    QCOMPARE(owner->renameLayer(layer, QStringLiteral("Edited name")), Gpu::Result::Success);
    QVERIFY(fixedJpegInformation(document.data()));
    const auto version = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    const auto savedIndex = owner->operations().savedHistoryIndex();
    const auto settings = jpegConfiguration(option, image->colorSpace());
    jpegCpuDocument(reference.data(), size, image->colorSpace(), rgba);
    if (!blendMode.isEmpty()) {
        KisImageSP referenceImage = reference->image().toStrongRef();
        KisPaintLayerSP upper = new KisPaintLayer(referenceImage, QStringLiteral("Addition"), 137);
        const auto nativePixels = QByteArray::fromHex("f0e0d089").repeated(size.width() * size.height());
        upper->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(nativePixels.constData()), 0, 0, size.width(), size.height());
        upper->setCompositeOpId(blendMode);
        referenceImage->addNode(upper);
        referenceImage->initialRefreshGraph();
        referenceImage->waitForDone();
    }
    QVERIFY(fixedJpegInformation(reference.data()));
    const auto referencePath = directory.filePath(QStringLiteral("reference.jpg"));
    QVERIFY2(reference->exportDocumentSync(referencePath, QByteArrayLiteral("image/jpeg"), settings), qPrintable(reference->errorMessage()));
    QFile expected(referencePath);
    QVERIFY(expected.open(QIODevice::ReadOnly));
    const auto expectedBytes = expected.readAll();
    const auto path = directory.filePath(QStringLiteral("export.jpg"));
    QSignalSpy saved(document.data(), &KisDocument::sigSavingFinished);
    Document publicDocument(document.data(), false);
    InfoObject publicSettings(settings);
    const bool success = menuExport ? document->exportDocument(path, QByteArrayLiteral("image/jpeg"), false, false, settings) :
        publicDocument.exportImage(path, publicSettings);
    QVERIFY2(success, qPrintable(document->errorMessage()));
    QFile output(path);
    QVERIFY(output.open(QIODevice::ReadOnly));
    const auto bytes = output.readAll();
    QCOMPARE(bytes, expectedBytes);
    const QImage decoded(path);
    QCOMPARE(decoded.size(), size);
    QCOMPARE(bytes.mid(6, 5), QByteArrayLiteral("JFIF\0"));
    QCOMPARE(quint8(bytes[13]), quint8(1));
    QCOMPARE(qFromBigEndian<quint16>(bytes.constData() + 14), quint16(300));
    QCOMPARE(qFromBigEndian<quint16>(bytes.constData() + 16), quint16(150));
    const QImage referenceImage(referencePath);
    QCOMPARE(decoded.dotsPerMeterX(), referenceImage.dotsPerMeterX());
    QCOMPARE(decoded.dotsPerMeterY(), referenceImage.dotsPerMeterY());
    const auto profile = option == 11 ? KoColorSpaceRegistry::instance()->rgb8(QStringLiteral("sRGB built-in"))->profile() : image->profile();
    QCOMPARE(decoded.colorSpace().iccProfile(), option == 12 ? QByteArray() : profile->rawData());
    if (option >= 16 && option < 20) QVERIFY(bytes.contains("Fixture"));
    if (option == 20) QVERIFY(!bytes.contains("Fixture"));
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QSaveFile artifact(QDir(resultDirectory).filePath(QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".jpg")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
        QVERIFY(artifact.commit());
    }
    const auto failedPath = directory.filePath(QStringLiteral("missing/export.jpg"));
    QVERIFY(!publicDocument.exportImage(failedPath, publicSettings));
    QVERIFY(!document->errorMessage().isEmpty());
    QCOMPARE(document->path(), nativePath);
    QCOMPARE(document->mimeType(), KisDocument::nativeFormatMimeType());
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), version);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->operations().savedHistoryIndex(), savedIndex);
    QCOMPARE(saved.size(), menuExport ? 1 : 0);
    QVERIFY(native.open(QIODevice::ReadOnly));
    QCOMPARE(native.readAll(), nativeBytes);
}

void TestDocument::testGpuJpegExportDialogLifetimes_data()
{
    QTest::addColumn<int>("reaction");
    QTest::addColumn<bool>("saving");
    for (const bool saving : {false, true}) {
        const QByteArray prefix = saving ? QByteArrayLiteral("save-") : QByteArrayLiteral("export-");
        QTest::newRow(QByteArray(prefix + "cancel-preserves-file").constData()) << 0 << saving;
        QTest::newRow(QByteArray(prefix + "later-edit-keeps-written-version").constData()) << 1 << saving;
        QTest::newRow(QByteArray(prefix + "close-document").constData()) << 2 << saving;
        QTest::newRow(QByteArray(prefix + "replace-image").constData()) << 3 << saving;
        QTest::newRow(QByteArray(prefix + "also-save-fixed-kra").constData()) << 4 << saving;
    }
}

void TestDocument::testGpuJpegExportDialogLifetimes()
{
    QFETCH(int, reaction);
    QFETCH(bool, saving);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    QScopedPointer<KisDocument> reference(KisPart::instance()->createDocument());
    const auto remove = qScopeGuard([&] {
        if (reference) KisPart::instance()->removeDocument(reference.data(), false);
        if (document) KisPart::instance()->removeDocument(document.data(), false);
    });
    const QSize size(33, 17);
    QString diagnostic;
    const auto owner = jpegGpuDocument(document.data(), size, diagnostic);
    QVERIFY2(owner, qPrintable(diagnostic));
    KisImageSP image = document->image().toStrongRef();
    image->setResolution(300.0 / 72, 150.0 / 72);
    const auto layer = owner->operations().activeLayer();
    auto rgba = jpegPixels(size);
    for (qsizetype i = 3; i < rgba.size(); i += 4) rgba[i] = char(255);
    QCOMPARE(owner->replaceRgbaPixels(layer, image->bounds(), rgba), Gpu::Result::Success);
    if (reaction == 4) QVERIFY(owner->addLayer(QStringLiteral("Second layer")));
    QVERIFY(fixedJpegInformation(document.data()));
    const auto savingHistory = owner->operations().undoCount();
    jpegCpuDocument(reference.data(), size, image->colorSpace(), rgba);
    QVERIFY(fixedJpegInformation(reference.data()));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "JPEG dialog artifacts retained at" << directory.path();
        }
    });
    const auto path = directory.filePath(QStringLiteral("existing.jpg"));
    const auto original = QByteArrayLiteral("Existing data remains intact when JPEG export is cancelled");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();
    const bool previousAlsoAsKra = KisConfig(true).readEntry<bool>("AlsoSaveAsKra", false);
    KisConfig(false).writeEntry<bool>("AlsoSaveAsKra", false);
    const auto restore = qScopeGuard([previousAlsoAsKra] {
        KisConfig(false).writeEntry<bool>("AlsoSaveAsKra", previousAlsoAsKra);
    });
    bool dialogVisited = false;
    bool interactionSucceeded = true;
    KisPropertiesConfigurationSP selected;
    QTimer interaction;
    interaction.setInterval(1);
    connect(&interaction, &QTimer::timeout, this, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        interaction.stop();
        dialogVisited = true;
        const auto widgets = dialog->findChildren<KisConfigWidget *>();
        if (widgets.size() != 1) {
            interactionSucceeded = false;
            dialog->reject();
            return;
        }
        widgets.first()->setConfiguration(jpegConfiguration(0, image->colorSpace()));
        selected = widgets.first()->configuration();
        if (reaction == 4) {
            bool found = false;
            for (auto *check : dialog->findChildren<QCheckBox *>()) {
                if (check->text().contains(QStringLiteral("KRA"))) {
                    check->setChecked(true);
                    found = true;
                }
            }
            if (!found) {
                interactionSucceeded = false;
                dialog->reject();
                return;
            }
        }
        if (reaction == 0) {
            dialog->reject();
            return;
        }
        if (reaction == 1) {
            interactionSucceeded = owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("aabbccff")) == Gpu::Result::Success;
        } else if (reaction == 2) {
            KisPart::instance()->removeDocument(document.data(), false);
            document.reset();
        } else if (reaction == 3) {
            interactionSucceeded = bool(jpegGpuDocument(document.data(), QSize(16, 16), diagnostic));
        }
        if (interactionSucceeded) dialog->accept();
        else dialog->reject();
    });
    interaction.start();
    const bool success = saving ? document->saveAs(path, QByteArrayLiteral("image/jpeg"), reaction == 4) :
        document->exportDocument(path, QByteArrayLiteral("image/jpeg"), false, reaction == 4);
    QVERIFY(dialogVisited);
    QVERIFY(interactionSucceeded);
    QCOMPARE(success, reaction != 0);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    file.close();
    if (reaction == 0) QCOMPARE(bytes, original);
    else {
        QVERIFY(selected);
        const auto referencePath = directory.filePath(QStringLiteral("reference.jpg"));
        QVERIFY2(reference->exportDocumentSync(referencePath, QByteArrayLiteral("image/jpeg"), selected), qPrintable(reference->errorMessage()));
        QFile expected(referencePath);
        QVERIFY(expected.open(QIODevice::ReadOnly));
        QCOMPARE(bytes, expected.readAll());
        QCOMPARE(QImage::fromData(bytes, "JPEG").size(), size);
        if (reaction == 1)
            QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("aabbccff"));
        if (reaction == 4) {
            QFile kra(path + QStringLiteral(".kra"));
            QVERIFY(kra.open(QIODevice::ReadOnly));
            const auto saved = LibrePaint::ImportExport::Kra::RasterCodec::read(kra);
            QVERIFY2(saved.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(saved.diagnostic));
            QCOMPARE(saved.document.image.layers.size(), 2);
            QCOMPARE(saved.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4), rgba.mid((5 * size.width() + 5) * 4, 4));
        }
        const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!resultDirectory.isEmpty()) {
            QVERIFY(QDir().mkpath(resultDirectory));
            QSaveFile artifact(QDir(resultDirectory).filePath(QStringLiteral("dialog-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".jpg")));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(bytes), bytes.size());
            QVERIFY(artifact.commit());
        }
    }
    if (document) {
        if (saving && reaction != 0 && reaction != 3) {
            QCOMPARE(document->path(), path);
            QCOMPARE(document->mimeType(), QByteArrayLiteral("image/jpeg"));
            QCOMPARE(document->isModified(), reaction == 1);
            QCOMPARE(owner->operations().savedHistoryIndex(), savingHistory);
            if (reaction == 1) {
                const auto current = owner->operations().snapshot();
                const auto history = owner->operations().undoCount();
                document->setFileBatchMode(true);
                QSignalSpy saved(document.data(), &KisDocument::sigSavingFinished);
                Document publicDocument(document.data(), false);
                QVERIFY2(publicDocument.save(), qPrintable(document->errorMessage()));
                QVERIFY(!document->isModified());
                QCOMPARE(saved.size(), 1);
                QCOMPARE(owner->operations().snapshot(), current);
                QCOMPARE(owner->operations().undoCount(), history);
                QCOMPARE(owner->operations().savedHistoryIndex(), history);
                rgba.replace((5 * size.width() + 5) * 4, 4, QByteArray::fromHex("aabbccff"));
                jpegCpuDocument(reference.data(), size, image->colorSpace(), rgba);
                QVERIFY(fixedJpegInformation(reference.data()));
                const auto resavedReferencePath = directory.filePath(QStringLiteral("resaved-reference.jpg"));
                QVERIFY2(reference->exportDocumentSync(resavedReferencePath, QByteArrayLiteral("image/jpeg"), selected), qPrintable(reference->errorMessage()));
                QFile expected(resavedReferencePath);
                QVERIFY(expected.open(QIODevice::ReadOnly));
                QVERIFY(file.open(QIODevice::ReadOnly));
                const auto resavedBytes = file.readAll();
                file.close();
                QCOMPARE(resavedBytes, expected.readAll());
                const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                if (!resultDirectory.isEmpty()) {
                    QSaveFile artifact(QDir(resultDirectory).filePath(QStringLiteral("dialog-resaved.jpg")));
                    QVERIFY(artifact.open(QIODevice::WriteOnly));
                    QCOMPARE(artifact.write(resavedBytes), resavedBytes.size());
                    QVERIFY(artifact.commit());
                }
                QCOMPARE(owner->undo(), Gpu::Result::Success);
                QVERIFY(document->isModified());
                QCOMPARE(owner->redo(), Gpu::Result::Success);
                QVERIFY(!document->isModified());
            }
        } else QVERIFY(document->path().isEmpty());
    }
}

void TestDocument::testGpuJpegExportStreamsLargeFixedDocuments_data()
{
    QTest::addColumn<bool>("progressive");
    QTest::newRow("optimized") << false;
    QTest::newRow("progressive") << true;
}

void TestDocument::testGpuJpegExportStreamsLargeFixedDocuments()
{
    QFETCH(bool, progressive);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto remove = qScopeGuard([&] { KisPart::instance()->removeDocument(document.data(), false); });
    QString diagnostic;
    const QSize size(16384, 16384);
    const auto owner = jpegGpuDocument(document.data(), size, diagnostic);
    QVERIFY2(owner, qPrintable(diagnostic));
    document->setFileBatchMode(true);
    QCOMPARE(owner->renameLayer(owner->operations().activeLayer(), QStringLiteral("Modified")), Gpu::Result::Success);
    const auto pinned = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    const auto savedIndex = owner->operations().savedHistoryIndex();
    const auto resident = owner->execution().residentBytes();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto preserve = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            directory.setAutoRemove(false);
            qInfo() << "Large JPEG artifacts retained at" << directory.path();
        }
    });
    const auto path = directory.filePath(QStringLiteral("large.jpg"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("previous file") > 0);
    file.close();
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("quality", 80);
    settings->setProperty("baseline", true);
    settings->setProperty("subsampling", 0);
    settings->setProperty("smoothing", 0);
    settings->setProperty("forceSRGB", false);
    settings->setProperty("saveProfile", true);
    settings->setProperty("transparencyFillcolor", QVariant::fromValue(KoColor(Qt::white, document->image()->colorSpace())));
    settings->setProperty("exif", false);
    settings->setProperty("iptc", false);
    settings->setProperty("xmp", false);
    settings->setProperty("filters", QString());
    settings->setProperty("optimize", true);
    settings->setProperty("progressive", progressive);
    settings->setProperty("storeAuthor", false);
    settings->setProperty("storeMetaData", false);
    QVERIFY2(document->exportDocumentSync(path, QByteArrayLiteral("image/jpeg"), settings), qPrintable(document->errorMessage()));
    QImageReader reader(path, "JPEG");
    QCOMPARE(reader.size(), size);
    reader.setScaledSize(QSize(8, 8));
    const auto preview = reader.read();
    QVERIFY2(!preview.isNull(), qPrintable(reader.errorString()));
    QCOMPARE(preview.size(), QSize(8, 8));
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) QCOMPARE(preview.pixelColor(x, y), QColor(Qt::white));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    QCOMPARE(bytes.last(2), QByteArray::fromHex("ffd9"));
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QSaveFile artifact(QDir(resultDirectory).filePath(QStringLiteral("large-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".jpg")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
        QVERIFY(artifact.commit());
    }
    QVERIFY(document->path().isEmpty());
    QVERIFY(document->isModified());
    QCOMPARE(owner->operations().snapshot(), pinned);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->operations().savedHistoryIndex(), savedIndex);
    QCOMPARE(owner->execution().residentBytes(), resident);
}
