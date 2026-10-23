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
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_properties_configuration.h>
#include <KoColorSpaceRegistry.h>
#include <metadata/KoDocumentInfo.h>
#include <kundo2stack.h>
#include <QDateTime>
#include <QColor>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>

namespace {
KisNodeSP namedLayer(KisImageSP image, const QString &name)
{
    KisNodeList pending{image->root()};
    for (qsizetype i = 0; i < pending.size(); ++i) {
        if (pending[i]->name() == name) return pending[i];
        for (auto child = pending[i]->firstChild(); child; child = child->nextSibling()) pending.append(child);
    }
    return {};
}

QMap<QString, bool> collapsedLayers(KisImageSP image)
{
    QMap<QString, bool> states;
    KisNodeList pending{image->root()};
    for (qsizetype i = 0; i < pending.size(); ++i) {
        if (i) states.insert(pending[i]->name(), pending[i]->collapsed());
        for (auto child = pending[i]->firstChild(); child; child = child->nextSibling()) pending.append(child);
    }
    return states;
}

QMap<QString, bool> collapsedLayers(Document &document)
{
    QMap<QString, bool> states;
    for (const auto &name : {QStringLiteral("Outer"), QStringLiteral("Inner"), QStringLiteral("Paint")}) {
        QScopedPointer<Node> node(document.nodeByName(name));
        if (!node) return {};
        states.insert(name, node->collapsed());
    }
    return states;
}

KisPropertiesConfigurationSP saveSettings()
{
    KisPropertiesConfigurationSP settings(new KisPropertiesConfiguration);
    settings->setProperty("documentSaveTime", QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate));
    return settings;
}

bool fixDocumentInformation(KisDocument *document)
{
    QDomDocument info;
    if (!info.setContent(QByteArrayLiteral("<document-info><about><title>Native collapsed layers</title>"
        "<creation-date>2000-01-02T03:04:05Z</creation-date><date>2001-02-03T04:05:06Z</date>"
        "<editing-cycles>7</editing-cycles><editing-time>10</editing-time><language>en</language></about>"
        "<author><full-name>LibrePaint fixture</full-name></author></document-info>")) ||
        !document->documentInfo()->load(info)) return false;
    document->documentInfo()->setAuthorInfo("creator", QStringLiteral("LibrePaint fixture"));
    document->setColorHistoryColors({});
    document->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    document->setPreActivatedNode(namedLayer(document->image(), QStringLiteral("Paint")));
    return true;
}

bool createNativeSource(const QString &path, bool collapsed)
{
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    KisImageSP image = new KisImage(document->createUndoStore(), 2, 1, KoColorSpaceRegistry::instance()->rgb8(), QStringLiteral("Collapsed source"));
    KisGroupLayerSP outer = new KisGroupLayer(image, QStringLiteral("Outer"), 255);
    outer->setUuid(QUuid(QStringLiteral("{22a00001-1122-3344-5566-778899aabbcc}")));
    outer->setCollapsed(collapsed);
    image->addNode(outer);
    KisGroupLayerSP inner = new KisGroupLayer(image, QStringLiteral("Inner"), 255);
    inner->setUuid(QUuid(QStringLiteral("{22a00002-1122-3344-5566-778899aabbcc}")));
    image->addNode(inner, outer);
    KisPaintLayerSP paint = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
    paint->setUuid(QUuid(QStringLiteral("{22a00003-1122-3344-5566-778899aabbcc}")));
    paint->setPinnedToTimeline(false);
    paint->setCollapsed(collapsed);
    const auto pixels = QByteArray::fromHex("332211ff66554480");
    paint->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), 0, 0, 2, 1);
    image->addNode(paint, inner);
    document->setCurrentImage(image);
    image->initialRefreshGraph();
    image->waitForDone();
    if (!fixDocumentInformation(document.data()) ||
        !document->saveAs(path, KisDocument::nativeFormatMimeType(), false, saveSettings())) return false;
    document->waitForSavingToComplete();
    return QFile::exists(path) && !document->isModified();
}

QByteArray fileBytes(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
}

void TestDocument::testCollapsedStateSurvivesDocumentCopies_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("operation");
    for (bool gpu : {false, true}) {
        for (const auto &operation : {QStringLiteral("snapshot"), QStringLiteral("saving-clone"), QStringLiteral("public-clone")}) {
            const QByteArray tag = (gpu ? QByteArrayLiteral("gpu-") : QByteArrayLiteral("cpu-")) + operation.toLatin1();
            QTest::newRow(tag.constData()) << gpu << operation;
        }
    }
}

void TestDocument::testCollapsedStateSurvivesDocumentCopies()
{
    QFETCH(bool, gpu);
    QFETCH(QString, operation);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] { if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous); });
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    const auto source = temporary.filePath(QStringLiteral("expanded.kra"));
    QVERIFY(createNativeSource(source, false));
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY2(document->openPath(source, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
    document->image()->waitForDone();
    Document api(document.data(), false);
    QScopedPointer<Node> outer(Node::createNode(document->image(), namedLayer(document->image(), QStringLiteral("Outer"))));
    QScopedPointer<Node> paint(Node::createNode(document->image(), namedLayer(document->image(), QStringLiteral("Paint"))));
    outer->setCollapsed(true);
    paint->setCollapsed(true);
    const auto expected = collapsedLayers(document->image());
    QScopedPointer<KisDocument> copy;
    QScopedPointer<Document> publicCopy;
    if (operation == QLatin1String("public-clone")) publicCopy.reset(api.clone());
    else if (operation == QLatin1String("snapshot")) copy.reset(document->lockAndCreateSnapshot());
    else copy.reset(document->lockAndCloneForSaving());
    QVERIFY(copy || publicCopy);
    if (copy) publicCopy.reset(new Document(copy.data(), false));
    QCOMPARE(collapsedLayers(*publicCopy), expected);
    QCOMPARE(publicCopy->pixelData(0, 0, 2, 1), api.pixelData(0, 0, 2, 1));
    outer->setCollapsed(false);
    QCOMPARE(collapsedLayers(*publicCopy), expected);
    QVERIFY(!document->isModified());
    QCOMPARE(document->undoStack()->count(), 0);
}

void TestDocument::testNativeCollapsedLayersSurviveEditing_data()
{
    QTest::addColumn<bool>("compress");
    QTest::newRow("raw") << false;
    QTest::newRow("compressed") << true;
}

void TestDocument::testNativeCollapsedLayersSurviveEditing()
{
    QFETCH(bool, compress);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const bool previousCompression = KisConfig(true).compressKra();
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", previous);
        KisConfig(false).setCompressKra(previousCompression);
    });
    KisConfig(false).setCompressKra(compress);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QDir output(QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(
        QStringLiteral("native-collapsed-%1").arg(QString::fromLatin1(QTest::currentDataTag()))));
    QVERIFY(QDir().mkpath(output.path()));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    const auto source = output.filePath(QStringLiteral("source.kra"));
    QVERIFY(createNativeSource(source, true));
    QMap<QString, QByteArray> referenceFiles;
    for (bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const QString prefix = gpu ? QStringLiteral("gpu") : QStringLiteral("cpu");
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true);
        document->setAutoSaveActive(false);
        QVERIFY2(document->openPath(source, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
        document->image()->waitForDone();
        Document api(document.data(), false);
        QCOMPARE(bool(document->image()->gpuImage()), gpu);
        QScopedPointer<Node> outer(Node::createNode(document->image(), namedLayer(document->image(), QStringLiteral("Outer"))));
        QScopedPointer<Node> inner(Node::createNode(document->image(), namedLayer(document->image(), QStringLiteral("Inner"))));
        QScopedPointer<Node> paint(Node::createNode(document->image(), namedLayer(document->image(), QStringLiteral("Paint"))));
        QVERIFY(outer->collapsed());
        QVERIFY(!inner->collapsed());
        QVERIFY(paint->collapsed());
        const auto pixels = api.pixelData(0, 0, 2, 1);
        QCOMPARE(pixels, QByteArray::fromHex("332211ff66554480"));
        QSignalSpy modified(document.data(), &KisDocument::modified);
        outer->setCollapsed(false);
        inner->setCollapsed(true);
        paint->setCollapsed(false);
        const auto editedStates = collapsedLayers(document->image());
        QCOMPARE(api.pixelData(0, 0, 2, 1), pixels);
        QVERIFY(!document->isModified());
        QCOMPARE(document->undoStack()->count(), 0);
        QCOMPARE(modified.count(), 0);
        api.resizeImage(0, 0, 3, 1);
        document->image()->waitForDone();
        QCOMPARE(document->image()->size(), QSize(3, 1));
        document->undoStack()->undo();
        document->image()->waitForDone();
        QCOMPARE(document->image()->size(), QSize(2, 1));
        document->undoStack()->redo();
        document->image()->waitForDone();
        QCOMPARE(collapsedLayers(document->image()), editedStates);
        QVERIFY(outer->remove());
        document->image()->waitForDone();
        document->undoStack()->undo();
        document->image()->waitForDone();
        QCOMPARE(collapsedLayers(document->image()), editedStates);
        QVERIFY(fixDocumentInformation(document.data()));
        document->setModified(true);
        const auto saved = output.filePath(prefix + QStringLiteral(".kra"));
        QVERIFY2(document->saveAs(saved, KisDocument::nativeFormatMimeType(), false, saveSettings()), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        reopened->setAutoSaveActive(false);
        QVERIFY2(reopened->openPath(saved, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
        QCOMPARE(collapsedLayers(reopened->image()), editedStates);
        Document reopenedApi(reopened.data(), false);
        QScopedPointer<Node> reopenedOuter(Node::createNode(reopened->image(), namedLayer(reopened->image(), QStringLiteral("Outer"))));
        reopenedOuter->setCollapsed(true);
        reopenedApi.resizeImage(0, 0, 4, 1);
        reopened->image()->waitForDone();
        QVERIFY(fixDocumentInformation(reopened.data()));
        reopened->setModified(true);
        const auto after = output.filePath(prefix + QStringLiteral("-after.kra"));
        QVERIFY2(reopened->saveAs(after, KisDocument::nativeFormatMimeType(), false, saveSettings()), qPrintable(reopened->errorMessage()));
        reopened->waitForSavingToComplete();
        InfoObject options;
        options.setProperty("quality", 90);
        options.setProperty("forceSRGB", false);
        options.setProperty("saveSRGBProfile", true);
        options.setProperty("alpha", true);
        options.setProperty("compression", 3);
        options.setProperty("storeMetaData", true);
        options.setProperty("storeAuthor", false);
        options.setProperty("exif", true);
        options.setProperty("iptc", true);
        options.setProperty("xmp", true);
        for (const auto &extension : {QStringLiteral("png"), QStringLiteral("jpg")})
            QVERIFY(reopenedApi.exportImage(output.filePath(prefix + QLatin1Char('.') + extension), options));
        for (const auto &suffix : {QStringLiteral(".kra"), QStringLiteral("-after.kra"), QStringLiteral(".png"), QStringLiteral(".jpg")}) {
            const auto bytes = fileBytes(output.filePath(prefix + suffix));
            QVERIFY(!bytes.isEmpty());
            if (!gpu) referenceFiles.insert(suffix, bytes);
            else QCOMPARE(bytes, referenceFiles.value(suffix));
        }
    }
}
