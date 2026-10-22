/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestDocument.h"
#include <QtTest>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QScopeGuard>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QSignalSpy>
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <KoStore.h>
#include <Document.h>
#include <Node.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <kis_config.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_properties_configuration.h>
#include <kundo2stack.h>
#include <metadata/KoDocumentInfo.h>
#include <metadata/kis_meta_data_store.h>
#include <metadata/kis_meta_data_entry.h>
#include <metadata/kis_meta_data_value.h>
#include <metadata/kis_meta_data_schema_registry.h>

namespace {
QMap<QString, KisNodeSP> namedNodes(KisImageSP image)
{
    QMap<QString, KisNodeSP> result;
    KisNodeList nodes{image->root()};
    for (qsizetype i = 0; i < nodes.size(); ++i) {
        result.insert(nodes[i]->name(), nodes[i]);
        for (auto child = nodes[i]->firstChild(); child; child = child->nextSibling()) nodes.append(child);
    }
    return result;
}

KisMetaData::Store *imageMetadata(KisNodeSP node)
{
    auto *layer = qobject_cast<KisLayer *>(node.data());
    return layer ? layer->metaData() : nullptr;
}

const QString tiffUri = QStringLiteral("http://ns.adobe.com/tiff/1.0/");

bool setCamera(KisNodeSP node, const QString &value)
{
    auto *store = imageMetadata(node);
    const auto schema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(tiffUri);
    if (!store || !schema) return false;
    store->removeEntry(schema, QStringLiteral("Make"));
    return value.isEmpty() || store->addEntry(KisMetaData::Entry(schema, QStringLiteral("Make"), KisMetaData::Value(QVariant(value))));
}

QString camera(KisNodeSP node)
{
    auto *store = imageMetadata(node);
    if (!store || !store->containsEntry(tiffUri, QStringLiteral("Make"))) return {};
    return store->getValue(tiffUri, QStringLiteral("Make")).asVariant().toString();
}

KisPropertiesConfigurationSP nativeSettings()
{
    KisPropertiesConfigurationSP result(new KisPropertiesConfiguration);
    result->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    return result;
}

bool prepareNativeSource(KisDocument *document)
{
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (!document->newImage(QStringLiteral("Native layer metadata"), 16, 8, cs,
                           KoColor(QColor(255, 255, 255, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1)) return false;
    document->setFileBatchMode(true);
    document->setColorHistoryColors({});
    document->documentInfo()->setAboutInfo("initial-creator", QStringLiteral("LibrePaint fixture"));
    document->documentInfo()->setAboutInfo("language", QStringLiteral("en"));
    document->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    KisImageSP image = document->image();
    auto paint = image->root()->firstChild();
    paint->setUuid(QUuid(QStringLiteral("{6a080001-1122-3344-5566-778899aabbcc}")));
    paint->setPinnedToTimeline(false);
    const auto pixels = QByteArray::fromHex("332211ff").repeated(128);
    paint->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), 0, 0, 16, 8);
    KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Metadata group"), 255);
    group->setUuid(QUuid(QStringLiteral("{6a080002-1122-3344-5566-778899aabbcc}")));
    group->setPinnedToTimeline(false);
    image->addNode(group);
    KisPaintLayerSP nested = new KisPaintLayer(image, QStringLiteral("Nested paint"), 255);
    nested->setUuid(QUuid(QStringLiteral("{6a080003-1122-3344-5566-778899aabbcc}")));
    nested->setPinnedToTimeline(false);
    image->addNode(nested, group);
    image->initialRefreshGraph();
    image->waitForDone();
    const auto schema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(tiffUri);
    if (!schema || !nested->metaData()->addEntry(KisMetaData::Entry(schema, QStringLiteral("Artist"), KisMetaData::Value(QVariant(QStringLiteral("Fixed artist"))))) ||
        !nested->metaData()->addEntry(KisMetaData::Entry(schema, QStringLiteral("Model"), KisMetaData::Value(QVariant(QStringLiteral("Fixed model")))))) return false;
    return setCamera(image->root(), QStringLiteral("Root camera")) &&
        setCamera(group, QStringLiteral("Group camera")) && setCamera(nested, QStringLiteral("Nested camera"));
}

void addBackendRows()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("cpu") << false;
    QTest::newRow("gpu") << true;
}

bool keepArtifact(const QString &name, const QByteArray &bytes)
{
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (results.isEmpty()) return true;
    if (!QDir().mkpath(results)) return false;
    const QString tag = QTest::currentDataTag() ? QString::fromLatin1(QTest::currentDataTag()) : QStringLiteral("gpu-autosave");
    QSaveFile output(QDir(results).filePath(QStringLiteral("native-layer-metadata-") + tag + QLatin1Char('-') + name));
    return output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size() && output.commit();
}

QByteArray fileBytes(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
}

void TestDocument::testNativeLayerImageMetadataSurvivesEditing_data() { addBackendRows(); }

void TestDocument::testNativeLayerImageMetadataSurvivesEditing()
{
    QFETCH(bool, gpu);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] { if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous); });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto retain = qScopeGuard([&] { if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "Native layer metadata artifacts" << directory.path(); } });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    QVERIFY(prepareNativeSource(source.data()));
    const auto original = directory.filePath(QStringLiteral("original.kra"));
    QVERIFY2(source->saveAs(original, KisDocument::nativeFormatMimeType(), false, nativeSettings()), qPrintable(source->errorMessage()));
    source->waitForSavingToComplete();
    QVERIFY(keepArtifact(QStringLiteral("source.kra"), fileBytes(original)));
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    bool metadataObserved = false;
    bool completeMetadataAtNotification = true;
    QMetaObject::Connection metadataObserver;
    if (gpu) {
        metadataObserver = connect(document->documentInfo(), &KoDocumentInfo::infoUpdated, document.data(), [&] {
            metadataObserved = true;
            const auto image = document->image().toStrongRef();
            if (!image) {
                completeMetadataAtNotification = false;
                return;
            }
            const auto loaded = namedNodes(image);
            completeMetadataAtNotification = completeMetadataAtNotification &&
                camera(loaded.value(QStringLiteral("root"))) == QLatin1String("Root camera") &&
                camera(loaded.value(QStringLiteral("Metadata group"))) == QLatin1String("Group camera") &&
                camera(loaded.value(QStringLiteral("Nested paint"))) == QLatin1String("Nested camera");
        });
    }
    QVERIFY2(document->openPath(original, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    disconnect(metadataObserver);
    if (gpu) {
        QVERIFY(metadataObserved);
        QVERIFY(completeMetadataAtNotification);
    }
    QCOMPARE(bool(document->image()->gpuImage()), gpu);
    const auto nodes = namedNodes(document->image());
    QCOMPARE(camera(nodes.value(QStringLiteral("root"))), QStringLiteral("Root camera"));
    QCOMPARE(camera(nodes.value(QStringLiteral("Metadata group"))), QStringLiteral("Group camera"));
    QCOMPARE(camera(nodes.value(QStringLiteral("Nested paint"))), QStringLiteral("Nested camera"));
    QCOMPARE(nodes.value(QStringLiteral("Nested paint"))->uuid(), QUuid(QStringLiteral("{6a080003-1122-3344-5566-778899aabbcc}")));
    for (auto *item : {source.data(), document.data()}) {
        const auto edited = namedNodes(item->image());
        QVERIFY(setCamera(edited.value(QStringLiteral("root")), QStringLiteral("Updated root")));
        QVERIFY(setCamera(edited.value(QStringLiteral("Metadata group")), {}));
        QVERIFY(setCamera(edited.value(QStringLiteral("Nested paint")), QStringLiteral("Updated camera")));
        item->documentInfo()->setAboutInfo("date", QStringLiteral("2001-02-03T04:05:06Z"));
        item->setModified(true);
    }
    KisPropertiesConfigurationSP options(new KisPropertiesConfiguration);
    options->setProperty("alpha", true);
    options->setProperty("indexed", false);
    options->setProperty("interlaced", false);
    options->setProperty("compression", 3);
    options->setProperty("saveSRGBProfile", true);
    options->setProperty("forceSRGB", false);
    options->setProperty("storeMetaData", true);
    options->setProperty("storeAuthor", false);
    options->setProperty("exif", true);
    options->setProperty("iptc", true);
    options->setProperty("xmp", true);
    for (const auto &extension : {QStringLiteral("png"), QStringLiteral("jpg")}) {
        const QByteArray mime = extension == QLatin1String("png") ? QByteArrayLiteral("image/png") : QByteArrayLiteral("image/jpeg");
        const auto referencePath = directory.filePath(QStringLiteral("reference.") + extension);
        QVERIFY(source->exportDocumentSync(referencePath, mime, options));
        const auto outputPath = directory.filePath(QStringLiteral("export.") + extension);
        QVERIFY2(document->exportDocument(outputPath, mime, false, false, options), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        const auto reference = fileBytes(referencePath);
        QVERIFY(!reference.isEmpty());
        QCOMPARE(fileBytes(outputPath), reference);
        QVERIFY(keepArtifact(QStringLiteral("export.") + extension, reference));
    }
    const auto saved = directory.filePath(QStringLiteral("updated.kra"));
    QVERIFY2(document->saveAs(saved, KisDocument::nativeFormatMimeType(), false, nativeSettings()), qPrintable(document->errorMessage()));
    document->waitForSavingToComplete();
    QVERIFY(keepArtifact(QStringLiteral("updated.kra"), fileBytes(saved)));
    for (bool gpuRead : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpuRead ? "1" : "0");
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY2(reopened->openPath(saved, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        const auto loaded = namedNodes(reopened->image());
        QCOMPARE(camera(loaded.value(QStringLiteral("root"))), QStringLiteral("Updated root"));
        QCOMPARE(camera(loaded.value(QStringLiteral("Metadata group"))), QString());
        QCOMPARE(camera(loaded.value(QStringLiteral("Nested paint"))), QStringLiteral("Updated camera"));
        QCOMPARE(imageMetadata(loaded.value(QStringLiteral("Nested paint")))->getValue(tiffUri, QStringLiteral("Artist")).asVariant().toString(), QStringLiteral("Fixed artist"));
        QCOMPARE(imageMetadata(loaded.value(QStringLiteral("Nested paint")))->getValue(tiffUri, QStringLiteral("Model")).asVariant().toString(), QStringLiteral("Fixed model"));
        Document api(reopened.data(), false);
        QCOMPARE(api.pixelData(0, 0, 16, 8), QByteArray::fromHex("332211ff").repeated(128));
    }
}

void TestDocument::testNativeLayerRemovalDoesNotReassignMetadata_data() { addBackendRows(); }

void TestDocument::testNativeLayerRemovalDoesNotReassignMetadata()
{
    QFETCH(bool, gpu);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] { if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous); });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto retain = qScopeGuard([&] { if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "Removed layer metadata artifacts" << directory.path(); } });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    QVERIFY(prepareNativeSource(source.data()));
    const auto path = directory.filePath(QStringLiteral("source.kra"));
    QVERIFY(source->saveAs(path, KisDocument::nativeFormatMimeType(), false, nativeSettings()));
    source->waitForSavingToComplete();
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY(document->openPath(path, KisDocument::DontAddToRecent));
    const auto victim = namedNodes(document->image()).value(QStringLiteral("Nested paint"));
    QScopedPointer<Node> publicVictim(Node::createNode(document->image(), victim));
    QVERIFY(publicVictim->remove());
    document->undoStack()->undo();
    document->image()->waitForDone();
    QCOMPARE(camera(namedNodes(document->image()).value(QStringLiteral("Nested paint"))), QStringLiteral("Nested camera"));
    document->undoStack()->redo();
    document->image()->waitForDone();
    QVERIFY(!namedNodes(document->image()).contains(QStringLiteral("Nested paint")));
    const auto deleted = directory.filePath(QStringLiteral("deleted.kra"));
    QVERIFY(document->saveAs(deleted, KisDocument::nativeFormatMimeType(), false, nativeSettings()));
    document->waitForSavingToComplete();
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(deleted, KisDocument::DontAddToRecent));
    Document api(reopened.data(), false);
    QScopedPointer<Node> root(api.rootNode());
    for (int i = 0; i < 4; ++i) {
        KisPaintLayerSP candidate = new KisPaintLayer(reopened->image(), QStringLiteral("Replacement %1").arg(i), 255);
        candidate->setUuid(QUuid(QStringLiteral("{6a08001%1-1122-3344-5566-778899aabbcc}").arg(i)));
        candidate->setPinnedToTimeline(false);
        QScopedPointer<Node> replacement(Node::createNode(reopened->image(), candidate));
        QVERIFY(replacement);
        QVERIFY(root->addChildNode(replacement.data(), nullptr));
        if (i == 3) QVERIFY(setCamera(namedNodes(reopened->image()).value(QStringLiteral("Replacement 3")), QStringLiteral("New camera")));
    }
    reopened->setModified(true);
    const auto added = directory.filePath(QStringLiteral("added.kra"));
    QVERIFY2(reopened->saveAs(added, KisDocument::nativeFormatMimeType(), false, nativeSettings()), qPrintable(reopened->errorMessage()));
    reopened->waitForSavingToComplete();
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> check(KisPart::instance()->createDocument());
    check->setFileBatchMode(true);
    QVERIFY(check->openPath(added, KisDocument::DontAddToRecent));
    const auto loaded = namedNodes(check->image());
    for (int i = 0; i < 4; ++i) {
        const auto replacement = loaded.value(QStringLiteral("Replacement %1").arg(i));
        QVERIFY(replacement);
        if (i == 3) {
            QCOMPARE(camera(replacement), QStringLiteral("New camera"));
            QVERIFY(!imageMetadata(replacement)->containsEntry(tiffUri, QStringLiteral("Artist")));
            QVERIFY(!imageMetadata(replacement)->containsEntry(tiffUri, QStringLiteral("Model")));
        }
        else QVERIFY2(imageMetadata(replacement)->isEmpty(), qPrintable(replacement->name()));
    }
    QVERIFY(keepArtifact(QStringLiteral("replacements.kra"), fileBytes(added)));
}

void TestDocument::testNativeMalformedLayerMetadataKeepsWarningAndOtherValues_data() { addBackendRows(); }

void TestDocument::testNativeMalformedLayerMetadataKeepsWarningAndOtherValues()
{
    QFETCH(bool, gpu);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] { if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous); });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto retain = qScopeGuard([&] { if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "Malformed layer metadata artifacts" << directory.path(); } });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    QVERIFY(prepareNativeSource(source.data()));
    const auto path = directory.filePath(QStringLiteral("source.kra"));
    QVERIFY(source->saveAs(path, KisDocument::nativeFormatMimeType(), false, nativeSettings()));
    source->waitForSavingToComplete();
    QMap<QString, QByteArray> files;
    {
        QScopedPointer<KoStore> store(KoStore::createStore(path, KoStore::Read));
        QVERIFY(store && !store->bad());
        for (const auto &name : store->directoryList()) {
            if (name.endsWith(QLatin1Char('/'))) continue;
            QVERIFY(store->open(name));
            files.insert(name, store->read(store->size()));
            QVERIFY(store->close());
        }
    }
    QDomDocument body;
    QVERIFY(body.setContent(files.value(QStringLiteral("maindoc.xml"))));
    const auto image = body.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    const auto elements = image.elementsByTagName(QStringLiteral("layer"));
    QString payload;
    for (int i = 0; i < elements.size(); ++i) {
        const auto element = elements.at(i).toElement();
        if (element.attribute(QStringLiteral("name")) == QLatin1String("Nested paint"))
            payload = image.attribute(QStringLiteral("name")) + QStringLiteral("/layers/") + element.attribute(QStringLiteral("filename")) + QStringLiteral(".xmp.metadata");
    }
    QVERIFY(!payload.isEmpty() && files.contains(payload));
    files[payload] = QByteArrayLiteral("not an XMP packet");
    const auto malformed = directory.filePath(QStringLiteral("malformed.kra"));
    {
        QScopedPointer<KoStore> store(KoStore::createStore(malformed, KoStore::Write, KisDocument::nativeFormatMimeType(), KoStore::Zip));
        QVERIFY(store && !store->bad());
        for (auto it = files.cbegin(); it != files.cend(); ++it) {
            if (it.key() == QLatin1String("mimetype")) continue;
            QVERIFY(store->open(it.key()));
            QCOMPARE(store->write(it.value()), qint64(it.value().size()));
            QVERIFY(store->close());
        }
        QVERIFY(store->finalize());
    }
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    QVERIFY2(document->openPath(malformed, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    QCOMPARE(bool(document->image()->gpuImage()), gpu);
    QVERIFY(document->errorMessage().isEmpty());
    QVERIFY(document->warningMessage().contains(QStringLiteral("Nested paint")));
    const auto nodes = namedNodes(document->image());
    QCOMPARE(camera(nodes.value(QStringLiteral("root"))), QStringLiteral("Root camera"));
    QCOMPARE(camera(nodes.value(QStringLiteral("Metadata group"))), QStringLiteral("Group camera"));
    QCOMPARE(camera(nodes.value(QStringLiteral("Nested paint"))), QString());
}

void TestDocument::testGpuNativeAutoSavePinsMetadataAndRecoveryKeepsLatest()
{
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] { if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous); });
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto retain = qScopeGuard([&] { if (QTest::currentTestFailed()) { directory.setAutoRemove(false); qInfo() << "Pinned layer metadata artifacts" << directory.path(); } });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    QVERIFY(prepareNativeSource(source.data()));
    const auto path = directory.filePath(QStringLiteral("source.kra"));
    QVERIFY(source->saveAs(path, KisDocument::nativeFormatMimeType(), false, nativeSettings()));
    source->waitForSavingToComplete();
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY(document->openPath(path, KisDocument::DontAddToRecent));
    QDomDocument fixedInfo;
    QVERIFY(fixedInfo.setContent(QByteArrayLiteral("<document-info><about><title>Native recovery metadata</title>"
        "<creation-date>2000-01-02T03:04:05Z</creation-date><date>2001-02-03T04:05:06Z</date>"
        "<editing-cycles>7</editing-cycles><editing-time>10</editing-time></about>"
        "<author><full-name>LibrePaint fixture</full-name></author></document-info>")));
    QVERIFY(document->documentInfo()->load(fixedInfo));
    const auto nodes = namedNodes(document->image());
    QVERIFY(setCamera(nodes.value(QStringLiteral("root")), QStringLiteral("Captured root")));
    QVERIFY(setCamera(nodes.value(QStringLiteral("Metadata group")), {}));
    QVERIFY(setCamera(nodes.value(QStringLiteral("Nested paint")), QStringLiteral("Captured camera")));
    document->setModified(true);
    const auto originalBytes = fileBytes(path);
    const auto originalFiles = QDir(directory.path()).entryList(QDir::Files | QDir::Hidden);
    QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
    QVERIFY(document->isSaving());
    QVERIFY(setCamera(nodes.value(QStringLiteral("root")), QStringLiteral("Later root")));
    QVERIFY(setCamera(nodes.value(QStringLiteral("Metadata group")), QStringLiteral("Later group")));
    QVERIFY(setCamera(nodes.value(QStringLiteral("Nested paint")), QStringLiteral("Later camera")));
    document->setModified(true);
    QTRY_VERIFY(!document->isSaving());
    QVERIFY(document->isModified());
    QCOMPARE(fileBytes(path), originalBytes);
    QStringList checkpoints = QDir(directory.path()).entryList(QDir::Files | QDir::Hidden);
    for (const auto &name : originalFiles) checkpoints.removeAll(name);
    QCOMPARE(checkpoints.size(), 1);
    const auto checkpoint = directory.filePath(checkpoints.first());
    QVERIFY(!fileBytes(checkpoint).isEmpty());
    QVERIFY(keepArtifact(QStringLiteral("autosave.kra"), fileBytes(checkpoint)));
    for (bool gpuRead : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpuRead ? "1" : "0");
        QScopedPointer<KisDocument> recovered(KisPart::instance()->createDocument());
        recovered->setFileBatchMode(true);
        QVERIFY2(recovered->openPath(checkpoint, KisDocument::DontAddToRecent), qPrintable(recovered->errorMessage()));
        const auto saved = namedNodes(recovered->image());
        QCOMPARE(camera(saved.value(QStringLiteral("root"))), QStringLiteral("Captured root"));
        QCOMPARE(camera(saved.value(QStringLiteral("Metadata group"))), QString());
        QCOMPARE(camera(saved.value(QStringLiteral("Nested paint"))), QStringLiteral("Captured camera"));
        Document api(recovered.data(), false);
        QCOMPARE(api.pixelData(0, 0, 16, 8), QByteArray::fromHex("332211ff").repeated(128));
    }
    QCOMPARE(camera(nodes.value(QStringLiteral("Nested paint"))), QStringLiteral("Later camera"));
    QSignalSpy finished(document.data(), &KisDocument::sigRecoveryAutoSaveFinished);
    QCOMPARE(document->requestRecoveryAutoSave(), KisDocument::RecoveryAutoSaveStartResult::Started);
    QCOMPARE(finished.size(), 0);
    QVERIFY(setCamera(nodes.value(QStringLiteral("root")), QStringLiteral("Newest root")));
    QVERIFY(setCamera(nodes.value(QStringLiteral("Metadata group")), QStringLiteral("Newest group")));
    QVERIFY(setCamera(nodes.value(QStringLiteral("Nested paint")), QStringLiteral("Newest camera")));
    document->setModified(true);
    QTRY_COMPARE(finished.size(), 1);
    QVERIFY2(finished[0][1].toBool(), qPrintable(document->errorMessage()));
    QVERIFY(!document->isSaving());
    QVERIFY(document->isModified());
    QCOMPARE(fileBytes(path), originalBytes);
    QCOMPARE(finished[0][0].toString(), checkpoint);
    QVERIFY(keepArtifact(QStringLiteral("recovery.kra"), fileBytes(checkpoint)));
    for (bool gpuRead : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpuRead ? "1" : "0");
        QScopedPointer<KisDocument> recovered(KisPart::instance()->createDocument());
        recovered->setFileBatchMode(true);
        QVERIFY2(recovered->openPath(checkpoint, KisDocument::DontAddToRecent), qPrintable(recovered->errorMessage()));
        const auto saved = namedNodes(recovered->image());
        QCOMPARE(camera(saved.value(QStringLiteral("root"))), QStringLiteral("Newest root"));
        QCOMPARE(camera(saved.value(QStringLiteral("Metadata group"))), QStringLiteral("Newest group"));
        QCOMPARE(camera(saved.value(QStringLiteral("Nested paint"))), QStringLiteral("Newest camera"));
        Document api(recovered.data(), false);
        QCOMPARE(api.pixelData(0, 0, 16, 8), QByteArray::fromHex("332211ff").repeated(128));
    }
}
