/*
 *  SPDX-FileCopyrightText: 2019 Tusooa Zhu <tusooa@vista.aero>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDocumentReplaceTest.h"

#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <QMap>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QFile>
#include <QTimer>
#include <QSaveFile>
#include <QDomDocument>
#include <kundo2stack.h>
#include <KoProgressProxy.h>
#include <KoColorProfile.h>
#include <kis_composite_progress_proxy.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <metadata/KoDocumentInfo.h>
#include <kra/KisKraRasterCodec.h>
#include <files/kis_document_autosave_files.h>
#include <stdexcept>
#include <kis_image_animation_interface.h>
#include <kis_layer_utils.h>
#include <kis_time_span.h>
#include <kis_types.h>
#include <testui.h>

namespace {
class RecordingProgressProxy : public KoProgressProxy
{
public:
    int maximum() const override
    {
        return m_maximum;
    }

    void setValue(int value) override
    {
        m_value = value;
    }

    void setRange(int, int maximum) override
    {
        m_maximum = maximum;
    }

    void setFormat(const QString &) override
    {
    }

    int value() const
    {
        return m_value;
    }

private:
    int m_maximum = 0;
    int m_value = 0;
};
}


void KisDocumentReplaceTest::init()
{
    m_doc = KisPart::instance()->createDocument();
    qDebug() << m_doc->newImage("test", 512, 512, KoColorSpaceRegistry::instance()->colorSpace("RGBA", "U8", 0), KoColor(), KisConfig::RASTER_LAYER, 1, "", 96);
}

void KisDocumentReplaceTest::finalize()
{
    delete m_doc;
    m_doc = 0;
}

void KisDocumentReplaceTest::testCopyFromDocument()
{
    init();
    QScopedPointer<KisDocument> clonedDoc(m_doc->lockAndCreateSnapshot());
    KisDocument *anotherDoc = KisPart::instance()->createDocument();
    anotherDoc->newImage("test", 512, 512, KoColorSpaceRegistry::instance()->colorSpace("RGBA", "U8", 0), KoColor(), KisConfig::RASTER_LAYER, 2, "", 96);
    KisImageSP anotherImage(anotherDoc->image());
    KisNodeSP root(anotherImage->rootLayer());
    anotherDoc->copyFromDocument(*(clonedDoc.data()));
    // image pointer should not change
    QCOMPARE(anotherImage.data(), anotherDoc->image().data());
    // root node should change
    QVERIFY(root.data() != anotherDoc->image()->rootLayer().data());
    // node count should be the same
    QList<KisNodeSP> oldNodes, newNodes;
    KisLayerUtils::recursiveApplyNodes(clonedDoc->image()->root(), [&oldNodes](KisNodeSP node) { oldNodes << node; });
    KisLayerUtils::recursiveApplyNodes(anotherDoc->image()->root(), [&newNodes](KisNodeSP node) { newNodes << node; });
    QCOMPARE(oldNodes.size(), newNodes.size());

    KisPart::instance()->removeDocument(anotherDoc);
    finalize();
}

void KisDocumentReplaceTest::testGpuAutoSavePreservesEditingState_data()
{
    QTest::addColumn<int>("entry");
    QTest::newRow("timer") << 0;
    QTest::newRow("recovery-request") << 1;
    QTest::newRow("application-pause") << 2;
}

void KisDocumentReplaceTest::testGpuAutoSavePreservesEditingState()
{
    QFETCH(int, entry);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    KisConfig config(false);
    const bool trim = config.trimKra();
    config.setTrimKra(true);
    const auto restoreConfig = qScopeGuard([&] { config.setTrimKra(trim); });
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device, 128 * 1024 * 1024, 2);
    Gpu::RasterImage raster{QSize(32, 16), {{QStringLiteral("Layer"), true,
        QUuid(QStringLiteral("{11111111-2222-3333-4444-555555555555}"))}}};
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Autosave image")));
    document->image()->setResolution(300.0 / 72, 150.0 / 72);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Autosave title"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("original.kra"));
    QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QDomDocument fixedInfo;
    QVERIFY(fixedInfo.setContent(QByteArrayLiteral("<document-info><about><title>Autosave title</title>"
        "<creation-date>2026-01-01T00:00:00</creation-date><date>2026-01-02T00:00:00</date>"
        "<editing-cycles>7</editing-cycles><editing-time>10</editing-time></about>"
        "<author><full-name>LibrePaint test</full-name></author></document-info>")));
    QVERIFY(document->documentInfo()->load(fixedInfo));
    QFile original(path);
    QVERIFY(original.open(QIODevice::ReadOnly));
    const auto originalBytes = original.readAll();
    original.close();
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    const auto version = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    const auto savedIndex = owner->operations().savedHistoryIndex();
    const auto autoSavePath = Krita::Document::KisDocumentAutoSaveFiles::filePath(
        path, document->objectName(), config.readEntry<bool>("autosavefileshidden"));
    QSignalSpy recovered(document.data(), &KisDocument::sigRecoveryAutoSaveFinished);
    if (entry == 0) {
        QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
    } else if (entry == 1) {
        QCOMPARE(document->requestRecoveryAutoSave(), KisDocument::RecoveryAutoSaveStartResult::Started);
        QCOMPARE(recovered.size(), 0);
        QTRY_COMPARE(recovered.size(), 1);
        QCOMPARE(recovered[0][0].toString(), autoSavePath);
        QVERIFY(recovered[0][1].toBool());
    } else {
        document->autoSaveOnPause();
    }
    QTRY_VERIFY(!document->isSaving());
    QVERIFY2(QFileInfo::exists(autoSavePath), qPrintable(document->errorMessage()));
    QFile checkpoint(autoSavePath);
    QVERIFY(checkpoint.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(checkpoint);
    QCOMPARE(loaded.code, Kra::Status::Success);
    const auto metadata = Kra::RasterCodec::readImageMetadata(loaded.document.description, loaded.document.files);
    QCOMPARE(metadata.code, Kra::Status::Success);
    QCOMPARE(metadata.metadata.resolution, QPointF(300, 150));
    QCOMPARE(metadata.metadata.iccProfile, document->image()->profile()->rawData());
    QVERIFY(metadata.metadata.documentInfo.contains("Autosave title"));
    QVERIFY(metadata.metadata.documentInfo.contains("2026-01-02T00:00:00"));
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("editing-cycles")), QStringLiteral("7"));
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("date")), QStringLiteral("2026-01-02T00:00:00"));
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QVERIFY(checkpoint.seek(0));
        QSaveFile artifact(QDir(resultDirectory).filePath(QStringLiteral("autosave-") +
                           QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kra")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        const auto bytes = checkpoint.readAll();
        QCOMPARE(artifact.write(bytes), bytes.size());
        QVERIFY(artifact.commit());
    }
    const auto reopened = Gpu::PaintImageSP::create(execution, loaded.document.image);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), QRect(5, 5, 1, 1), 4).bytes,
             QByteArray::fromHex("112233ff"));
    QCOMPARE(owner->operations().snapshot(), version);
    QCOMPARE(owner->operations().undoCount(), history);
    QCOMPARE(owner->operations().savedHistoryIndex(), savedIndex);
    QCOMPARE(document->path(), path);
    QVERIFY(document->isModified());
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), originalBytes);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!document->isModified());
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(document->isModified());
}

void KisDocumentReplaceTest::testGpuAutoSaveFailureKeepsCheckpointAndCanRetry_data()
{
    QTest::addColumn<int>("failure");
    QTest::newRow("pending-input-then-cancel") << 0;
    QTest::newRow("write-failure-then-retry") << 1;
    QTest::newRow("readback-capacity") << 2;
}

void KisDocumentReplaceTest::testGpuAutoSaveFailureKeepsCheckpointAndCanRetry()
{
    QFETCH(int, failure);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device, 128 * 1024 * 1024, 1, failure == 2 ? 1 : 64 * 1024 * 1024);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setAutoSaveActive(false);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Autosave retry")));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("original.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write("original file");
    original.close();
    document->setPath(path);
    document->setLocalFilePath(path);
    const auto checkpointPath = Krita::Document::KisDocumentAutoSaveFiles::filePath(
        path, document->objectName(), KisConfig(true).readEntry<bool>("autosavefileshidden"));
    if (failure == 1) {
        QVERIFY(QDir().mkdir(checkpointPath));
    } else {
        QFile checkpoint(checkpointPath);
        QVERIFY(checkpoint.open(QIODevice::WriteOnly));
        checkpoint.write("previous checkpoint");
    }
    Gpu::Brush brush;
    brush.diameter = 4;
    brush.rgba = {17, 34, 51, 255};
    const auto layer = owner->operations().activeLayer();
    auto stroke = owner->beginStroke(layer, brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{QPointF(5, 5), 1, 0, 0, 0, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    const auto committed = owner->operations().snapshot();
    if (failure == 0) {
        stroke = owner->beginStroke(layer, brush, 9);
        QVERIFY(!stroke.isNull());
        QCOMPARE(owner->appendSamples(stroke, {{QPointF(10, 5), 1, 0, 0, 0, 1}}), Gpu::Result::Success);
    }
    const auto version = owner->operations().snapshot();
    const auto history = owner->operations().undoCount();
    QSignalSpy messages(document.data(), &KisDocument::statusBarMessage);
    for (int attempt = 0; attempt < 5; ++attempt) {
        QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
        QTRY_VERIFY(!document->isSaving());
        QCOMPARE(owner->operations().snapshot(), version);
        QCOMPARE(owner->operations().undoCount(), history);
        QVERIFY(document->isModified());
        QCOMPARE(document->path(), path);
        if (failure == 1) {
            QVERIFY(QFileInfo(checkpointPath).isDir());
        } else {
            QFile checkpoint(checkpointPath);
            QVERIFY(checkpoint.open(QIODevice::ReadOnly));
            QCOMPARE(checkpoint.readAll(), QByteArrayLiteral("previous checkpoint"));
        }
    }
    if (failure == 0) {
        QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot(), committed);
    } else if (failure == 1) {
        QVERIFY(QDir().rmdir(checkpointPath));
    } else {
        QVERIFY(!document->errorMessage().isEmpty());
        return;
    }
    QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
    QTRY_VERIFY(!document->isSaving());
    QStringList diagnostics{document->errorMessage(), checkpointPath};
    for (const auto &message : messages) diagnostics.append(message[0].toString());
    QVERIFY2(Krita::Document::KisDocumentAutoSaveFiles::isUsable(checkpointPath), qPrintable(diagnostics.join(QLatin1Char('\n'))));
    QFile checkpoint(checkpointPath);
    QVERIFY(checkpoint.open(QIODevice::ReadOnly));
    QCOMPARE(LibrePaint::ImportExport::Kra::RasterCodec::read(checkpoint).code,
             LibrePaint::ImportExport::Kra::Status::Success);
    QCOMPARE(owner->operations().snapshot(), committed);
    QCOMPARE(owner->operations().undoCount(), history);
    QVERIFY(document->isModified());
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArrayLiteral("original file"));
}

void KisDocumentReplaceTest::testGpuRecoveryAutoSaveWaitsForLatestEdit_data()
{
    QTest::addColumn<bool>("joinTimer");
    QTest::addColumn<bool>("pause");
    QTest::newRow("edit-during-recovery-completion") << false << false;
    QTest::newRow("join-timer-and-edit") << true << false;
    QTest::newRow("pause-during-recovery") << false << true;
    QTest::newRow("pause-during-timer") << true << true;
}

void KisDocumentReplaceTest::testGpuRecoveryAutoSaveWaitsForLatestEdit()
{
    QFETCH(bool, joinTimer);
    QFETCH(bool, pause);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device, 128 * 1024 * 1024, 2);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setAutoSaveActive(false);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Latest recovery")));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("original.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write("original file");
    original.close();
    document->setPath(path);
    document->setLocalFilePath(path);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Before request"));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    const auto checkpointPath = Krita::Document::KisDocumentAutoSaveFiles::filePath(
        path, document->objectName(), KisConfig(true).readEntry<bool>("autosavefileshidden"));
    int messages = 0;
    bool firstCheckpointInspected = false;
    connect(document.data(), &KisDocument::statusBarMessage, this, [&](const QString &, int) {
        if (++messages != 2) return;
        QFile checkpoint(checkpointPath);
        QVERIFY(checkpoint.open(QIODevice::ReadOnly));
        const auto loaded = Kra::RasterCodec::read(checkpoint);
        QCOMPARE(loaded.code, Kra::Status::Success);
        const auto metadata = Kra::RasterCodec::readImageMetadata(loaded.document.description, loaded.document.files);
        QCOMPARE(metadata.code, Kra::Status::Success);
        QVERIFY(metadata.metadata.documentInfo.contains("Before request"));
        QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
                 QByteArray::fromHex("112233ff"));
        firstCheckpointInspected = true;
    });
    QSignalSpy finished(document.data(), &KisDocument::sigRecoveryAutoSaveFinished);
    if (joinTimer) {
        QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
    }
    QCOMPARE(document->requestRecoveryAutoSave(), joinTimer ? KisDocument::RecoveryAutoSaveStartResult::AlreadySaving :
                                                            KisDocument::RecoveryAutoSaveStartResult::Started);
    QVERIFY(finished.isEmpty());
    QVERIFY(!QFileInfo::exists(checkpointPath));
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("After request"));
    document->setModified(true);
    if (pause) {
        document->autoSaveOnPause();
        QFile checkpoint(checkpointPath);
        QVERIFY(checkpoint.open(QIODevice::ReadOnly));
        const auto loaded = Kra::RasterCodec::read(checkpoint);
        QCOMPARE(loaded.code, Kra::Status::Success);
        QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((5 * 256 + 5) * 4, 4),
                 QByteArray::fromHex("445566ff"));
    }
    QTRY_COMPARE(finished.size(), 1);
    QVERIFY(firstCheckpointInspected);
    QVERIFY(finished[0][1].toBool());
    QFile checkpoint(finished[0][0].toString());
    QVERIFY(checkpoint.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(checkpoint);
    QCOMPARE(loaded.code, Kra::Status::Success);
    const auto metadata = Kra::RasterCodec::readImageMetadata(loaded.document.description, loaded.document.files);
    QCOMPARE(metadata.code, Kra::Status::Success);
    QVERIFY(metadata.metadata.documentInfo.contains("After request"));
    const auto reopened = Gpu::PaintImageSP::create(execution, loaded.document.image);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), QRect(5, 5, 1, 1), 4).bytes,
             QByteArray::fromHex("445566ff"));
    QCOMPARE(document->requestRecoveryAutoSave(), KisDocument::RecoveryAutoSaveStartResult::NoChanges);
    QCOMPARE(owner->operations().undoCount(), 2);
    QVERIFY(document->isModified());
    QCOMPARE(document->path(), path);
}

void KisDocumentReplaceTest::testGpuAutoSaveNotificationLifetimes_data()
{
    QTest::addColumn<bool>("recovery");
    QTest::addColumn<int>("reaction");
    QTest::newRow("timer-start-destruction") << false << 0;
    QTest::newRow("timer-finish-destruction") << false << 1;
    QTest::newRow("recovery-start-destruction") << true << 0;
    QTest::newRow("recovery-finish-destruction") << true << 1;
    QTest::newRow("recovery-image-replacement") << true << 2;
    QTest::newRow("recovery-finish-image-replacement") << true << 3;
    QTest::newRow("timer-failure-destruction") << false << 4;
    QTest::newRow("timer-postponement-destruction") << false << 5;
    QTest::newRow("recovery-close-pending-save") << true << 6;
}

void KisDocumentReplaceTest::testGpuAutoSaveNotificationLifetimes()
{
    QFETCH(bool, recovery);
    QFETCH(int, reaction);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device, 128 * 1024 * 1024, 2);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setAutoSaveActive(false);
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Lifetime recovery")));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("original.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write("original file");
    original.close();
    document->setPath(path);
    document->setLocalFilePath(path);
    if (reaction == 4) {
        QVERIFY(QDir().mkdir(Krita::Document::KisDocumentAutoSaveFiles::filePath(
            path, document->objectName(), KisConfig(true).readEntry<bool>("autosavefileshidden"))));
    }
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    if (reaction == 5) {
        const auto stroke = owner->beginStroke(layer, Gpu::Brush{}, 11);
        QVERIFY(!stroke.isNull());
        QCOMPARE(owner->appendSamples(stroke, {{QPointF(10, 5), 1, 0, 0, 0, 1}}), Gpu::Result::Success);
    }
    Gpu::PaintImageSP replacement;
    if (reaction == 2 || reaction == 3) replacement = Gpu::PaintImageSP::create(execution, QSize(16, 8));
    int messages = 0;
    connect(document.data(), &KisDocument::statusBarMessage, this, [&](const QString &, int) {
        ++messages;
        if ((reaction == 0 && messages == 1) ||
            ((reaction == 1 || reaction == 4 || reaction == 5) && messages == 2)) document.reset();
        if (reaction == 3 && messages == 2) {
            document->setCurrentImage(KisImage::fromGpuImage(replacement, QStringLiteral("Replacement")));
        }
    });
    QSignalSpy finished(document.data(), &KisDocument::sigRecoveryAutoSaveFinished);
    if (recovery) {
        const auto result = document->requestRecoveryAutoSave();
        QCOMPARE(result, reaction == 0 ? KisDocument::RecoveryAutoSaveStartResult::Failed :
                                        KisDocument::RecoveryAutoSaveStartResult::Started);
    } else {
        QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
    }
    if (reaction == 6) {
        const auto checkpointPath = Krita::Document::KisDocumentAutoSaveFiles::filePath(
            path, document->objectName(), KisConfig(true).readEntry<bool>("autosavefileshidden"));
        QVERIFY(!QFileInfo::exists(checkpointPath));
        document.reset();
        QFile checkpoint(checkpointPath);
        QVERIFY(checkpoint.open(QIODevice::ReadOnly));
        QCOMPARE(LibrePaint::ImportExport::Kra::RasterCodec::read(checkpoint).code,
                 LibrePaint::ImportExport::Kra::Status::Success);
    }
    if (reaction < 2 || reaction >= 4) {
        QTRY_VERIFY(!document);
        if (recovery && (reaction == 1 || reaction == 6)) {
            QCOMPARE(finished.size(), 1);
            QVERIFY(!finished[0][1].toBool());
        } else {
            QVERIFY(finished.isEmpty());
        }
    } else {
        if (reaction == 2) document->setCurrentImage(KisImage::fromGpuImage(replacement, QStringLiteral("Replacement")));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished[0][1].toBool());
        QVERIFY(!document->isModified());
        QVERIFY(!document->isSaving());
        QCOMPARE(messages, reaction == 2 ? 1 : 2);
        QCOMPARE(replacement->replaceRgbaPixels(replacement->operations().activeLayer(), QRect(1, 1, 1, 1),
                                               QByteArray::fromHex("aabbccff")), Gpu::Result::Success);
        QCOMPARE(document->requestRecoveryAutoSave(), KisDocument::RecoveryAutoSaveStartResult::Started);
        QTRY_COMPARE(finished.size(), 2);
        QVERIFY(finished[1][1].toBool());
    }
    QCOMPARE(owner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
}

void KisDocumentReplaceTest::testGpuReplacementKeepsReferencesAndRejectsCapacityBeforeMutation_data()
{
    QTest::addColumn<bool>("capacityFailure");
    QTest::newRow("independent-replacement") << false;
    QTest::newRow("full-document-capacity") << true;
}

void KisDocumentReplaceTest::testGpuReplacementKeepsReferencesAndRejectsCapacityBeforeMutation()
{
    QFETCH(bool, capacityFailure);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device, 128 * 1024 * 1024, capacityFailure ? 2 : 16);
    const auto sourceOwner = Gpu::PaintImageSP::create(execution, QSize(64, 32));
    const auto oldOwner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    const auto layer = sourceOwner->operations().activeLayer();
    QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    QCOMPARE(oldOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("aabbccff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    QScopedPointer<KisDocument> target(KisPart::instance()->createDocument());
    source->setCurrentImage(KisImage::fromGpuImage(sourceOwner, QStringLiteral("Source image")));
    target->setCurrentImage(KisImage::fromGpuImage(oldOwner, QStringLiteral("Target image")));
    source->setPath(QStringLiteral("/tmp/source-image.kra"));
    target->setPath(QStringLiteral("/tmp/target-image.kra"));
    source->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Source title"));
    target->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Target title"));
    target->setModified(true);
    const auto image = KisImageSP(target->image());
    const auto root = image->root();
    const auto retained = root->firstChild()->paintDevice();
    const auto previous = oldOwner->operations().snapshot();
    if (capacityFailure) {
        QVERIFY_EXCEPTION_THROWN(target->copyFromDocument(*source), std::length_error);
        QCOMPARE(target->image().data(), image.data());
        QCOMPARE(image->root(), root);
        QCOMPARE(target->path(), QStringLiteral("/tmp/target-image.kra"));
        QCOMPARE(target->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Target title"));
        QVERIFY(target->isModified());
        QCOMPARE(image->gpuImage(), oldOwner);
        QCOMPARE(oldOwner->operations().snapshot(), previous);
        QCOMPARE(execution.requested(oldOwner->handle()), previous);
    } else {
        target->copyFromDocument(*source);
        QCOMPARE(target->image().data(), image.data());
        QVERIFY(image->root() != root);
        QCOMPARE(image->bounds().size(), QSize(64, 32));
        QCOMPARE(target->path(), source->path());
        QCOMPARE(target->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Source title"));
        QVERIFY(target->isModified());
        const auto copied = image->gpuImage();
        QVERIFY(copied != sourceOwner);
        QVERIFY(copied != oldOwner);
        QCOMPARE(copied->operations().undoCount(), 0);
        QCOMPARE(copied->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
        target->setModified(false);
        QSignalSpy changes(image.data(), &KisImage::sigImageModified);
        QCOMPARE(oldOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("ddeeffff")), Gpu::Result::Success);
        QVERIFY(changes.isEmpty());
        QVERIFY(!target->isModified());
        QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("445566ff")), Gpu::Result::Success);
        QCOMPARE(copied->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
        QCOMPARE(copied->replaceRgbaPixels(layer, QRect(5, 5, 1, 1), QByteArray::fromHex("778899ff")), Gpu::Result::Success);
        QVERIFY(target->isModified());
        QCOMPARE(sourceOwner->readRgbaPixels(layer, QRect(5, 5, 1, 1), 4).bytes, QByteArray::fromHex("445566ff"));
        QByteArray bytes(4, Qt::Uninitialized);
        retained->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 5, 5, 1, 1);
        QCOMPARE(bytes, QByteArray::fromHex("ffeeddff"));
    }
    KisPart::instance()->removeDocument(source.data(), false);
    KisPart::instance()->removeDocument(target.data(), false);
}

void KisDocumentReplaceTest::testDocumentIdentityDelegation()
{
    init();
    QSignalSpy pathChangedSpy(m_doc, &KisDocument::sigPathChanged);

    const QString displayPath = QStringLiteral("content://provider/document/42");
    const QString localPath = QStringLiteral("/tmp/librepaint-open-42.kra");
    m_doc->setPath(displayPath);
    m_doc->setPath(displayPath);
    m_doc->setLocalFilePath(localPath);
    m_doc->setMimeType("application/x-krita");

    QCOMPARE(pathChangedSpy.count(), 1);
    QCOMPARE(m_doc->path(), displayPath);
    QCOMPARE(m_doc->localFilePath(), localPath);
    QCOMPARE(m_doc->mimeType(), QByteArray("application/x-krita"));

    QScopedPointer<KisDocument> snapshot(m_doc->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QCOMPARE(snapshot->path(), displayPath);
    QCOMPARE(snapshot->localFilePath(), localPath);
    QCOMPARE(snapshot->mimeType(), QByteArray("application/x-krita"));

    QString localPathDuringResetSignal;
    connect(m_doc, &KisDocument::sigPathChanged, this, [&](const QString &path) {
        if (path.isEmpty()) {
            localPathDuringResetSignal = m_doc->localFilePath();
        }
    });
    m_doc->resetPath();
    QCOMPARE(pathChangedSpy.count(), 2);
    QCOMPARE(localPathDuringResetSignal, localPath);
    QVERIFY(m_doc->path().isEmpty());
    QVERIFY(m_doc->localFilePath().isEmpty());

    finalize();
}

void KisDocumentReplaceTest::testDocumentModificationStateDelegation()
{
    init();
    QSignalSpy modifiedSpy(m_doc, &KisDocument::modified);

    QVERIFY(!m_doc->isModified());

    m_doc->setModified(true);
    m_doc->setModified(true);

    QCOMPARE(modifiedSpy.count(), 1);
    QCOMPARE(modifiedSpy.at(0).at(0).toBool(), true);
    QVERIFY(m_doc->isModified());

    QScopedPointer<KisDocument> snapshot(m_doc->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QVERIFY(!snapshot->isModified());

    m_doc->setModified(false);

    QCOMPARE(modifiedSpy.count(), 2);
    QCOMPARE(modifiedSpy.at(1).at(0).toBool(), false);
    QVERIFY(!m_doc->isModified());

    finalize();
}

void KisDocumentReplaceTest::testQueuedHistoryNotificationsUseCurrentState_data()
{
    QTest::addColumn<bool>("deliveredClean");
    QTest::addColumn<bool>("untrackedEdit");
    QTest::newRow("dirty-notification-after-clean") << false << false;
    QTest::newRow("clean-notification-after-dirty") << true << false;
    QTest::newRow("clean-notification-keeps-untracked-edit") << true << true;
}

void KisDocumentReplaceTest::testQueuedHistoryNotificationsUseCurrentState()
{
    QFETCH(bool, deliveredClean);
    QFETCH(bool, untrackedEdit);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Deferred history"), 64, 64, cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    auto *stack = document->undoStack();
    stack->push(new KUndo2Command);
    stack->setClean();
    bool queued = false;
    bool scheduled = false;
    const auto connection = connect(stack, &KUndo2Stack::cleanChanged, document.data(), [&](bool clean) {
        if (queued || clean != deliveredClean) return;
        queued = true;
        scheduled = QMetaObject::invokeMethod(document.data(), "slotUndoStackCleanChanged",
                                             Qt::QueuedConnection, Q_ARG(bool, clean));
    });
    const auto disconnectHistory = qScopeGuard([&] { disconnect(connection); });
    stack->push(new KUndo2Command);
    stack->undo();
    if (deliveredClean && !untrackedEdit) stack->redo();
    if (untrackedEdit) {
        QVERIFY(QMetaObject::invokeMethod(document.data(), "setImageModifiedWithoutUndo", Qt::DirectConnection));
    }
    QVERIFY(queued && scheduled);
    const bool expected = !stack->isClean() || untrackedEdit;
    QCOMPARE(document->isModified(), expected);
    QApplication::sendPostedEvents(document.data(), QEvent::MetaCall);
    QCOMPARE(document->isModified(), expected);
}

void KisDocumentReplaceTest::testDocumentRecoveryStatusDelegation()
{
    init();
    QSignalSpy recoveredSpy(m_doc, &KisDocument::sigRecoveredChanged);

    QVERIFY(!m_doc->isRecovered());

    m_doc->setRecovered(true);
    m_doc->setRecovered(true);

    QCOMPARE(recoveredSpy.count(), 1);
    QCOMPARE(recoveredSpy.at(0).at(0).toBool(), true);
    QVERIFY(m_doc->isRecovered());

    QScopedPointer<KisDocument> snapshot(m_doc->lockAndCreateSnapshot());
    QVERIFY(snapshot);
    QVERIFY(!snapshot->isRecovered());

    m_doc->setRecovered(false);
    m_doc->setRecovered(false);

    QCOMPARE(recoveredSpy.count(), 2);
    QCOMPARE(recoveredSpy.at(1).at(0).toBool(), false);
    QVERIFY(!m_doc->isRecovered());

    finalize();
}

void KisDocumentReplaceTest::testTemplateRootLayerNameTranslation()
{
    init();

    KisNodeSP layer = m_doc->image()->rootLayer();
    QVERIFY(layer);
    layer->setName(QStringLiteral("Background"));

    const QMap<QString, QString> dictionary {
        {QStringLiteral("Background"), QStringLiteral("Translated Background")}
    };
    m_doc->translateTemplateRootLayerName(dictionary);

    QCOMPARE(layer->name(), QStringLiteral("Translated Background"));

    finalize();
}

void KisDocumentReplaceTest::testImageStateDelegation()
{
    init();

    QVERIFY(m_doc->hasImage());
    QCOMPARE(m_doc->imageObjectName(), QStringLiteral("test"));
    QVERIFY(m_doc->imageMemorySize() >= 0);
    QCOMPARE(m_doc->imageUndoAdapter(), m_doc->image()->undoAdapter());

    RecordingProgressProxy progressProxy;
    m_doc->addImageProgressProxy(&progressProxy);
    m_doc->image()->compositeProgressProxy()->setValue(37);
    QCOMPARE(progressProxy.value(), 37);
    m_doc->removeImageProgressProxy(&progressProxy);
    m_doc->image()->compositeProgressProxy()->setValue(82);
    QCOMPARE(progressProxy.value(), 37);

    QScopedPointer<KisDocument> lockedClone(m_doc->cloneWithImageReadLock());
    QVERIFY(lockedClone);
    QVERIFY(lockedClone->hasImage());
    QCOMPARE(lockedClone->imageObjectName(), m_doc->imageObjectName());

    m_doc->setAnimationTiming(12, 2, 8);
    QCOMPARE(m_doc->image()->animationInterface()->framerate(), 12);
    QCOMPARE(m_doc->image()->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(2, 8));

    QCOMPARE(m_doc->animationLength(), 9);
    if (m_doc->animationLength() < 4) {
        m_doc->setAnimationRange(0, 4);
    }
    QCOMPARE(m_doc->image()->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(2, 8));
    m_doc->setAnimationRange(0, 10);
    QCOMPARE(m_doc->image()->animationInterface()->documentPlaybackRange(), KisTimeSpan::fromTimeToTime(0, 10));

    m_doc->refreshProjectionAndWait();

    finalize();
}

void KisDocumentReplaceTest::testApplicationIdleWatcherIdentity()
{
    KisPart *part = KisPart::instance();
    KisIdleWatcher *idleWatcher = part->idleWatcher();

    QVERIFY(idleWatcher);
    QCOMPARE(part->idleWatcher(), idleWatcher);
}

KISTEST_MAIN(KisDocumentReplaceTest)
