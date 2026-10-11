/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuKraLoadJob.h"
#include "KisGpuKraSaveJob.h"
#include "KisGpuDocumentDevice.h"
#include "KisGpuTestDevice.h"
#include <testui.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <metadata/KoDocumentInfo.h>
#include <canvas/kis_grid_config.h>
#include <canvas/kis_guides_config.h>
#include <canvas/KisMirrorAxisConfig.h>
#include <QDomDocument>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpRegistry.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <QElapsedTimer>
#include <QTemporaryDir>

namespace {
using Store = KisGpuTileStore;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
using Load = KisGpuKraLoadJob;
using Save = KisGpuKraSaveJob;
const QRect Bounds(0, 0, 32, 32);
template<class Job> bool finish(Job &job)
{
    QElapsedTimer timer; timer.start();
    while (job.state() != Job::State::Succeeded && job.state() != Job::State::Failed
           && timer.elapsed() < 5000) { job.poll(); QTest::qWait(1); }
    return job.state() == Job::State::Succeeded;
}
bool paint(Document &document, Document::LayerId id, quint32 rgba,
           Document::Target target = Document::Target::Pixels)
{
    const auto edit = document.begin(id, target);
    Store::DabCommand command;
    command.center = {8, 8}; command.diameter = {12, 12}; command.rgba = rgba;
    if (edit.result != Document::Result::Accepted
        || edit.session->append(edit.token, {command}, Bounds) != Session::Result::Accepted
        || document.commit(edit.token) != Document::Result::Accepted) return false;
    QElapsedTimer timer; timer.start();
    while (document.state() == Document::State::Committing && timer.elapsed() < 5000) {
        document.poll(); QTest::qWait(1);
    }
    return document.state() == Document::State::Idle;
}
bool nativeFixture(const QString &path, int variant = 0)
{
    auto registry = KoColorSpaceRegistry::instance();
    const auto cs = variant == 4 ? registry->rgb16(registry->p709SRGBProfile())
                                : registry->rgb8(registry->p709SRGBProfile());
    KisImageSP image = new KisImage(nullptr, Bounds.width(), Bounds.height(), cs, QStringLiteral("Native"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
    layer->paintDevice()->fill(Bounds, KoColor(Qt::red, cs));
    image->addNode(layer, image->root());
    if (variant == 1) layer->setCompositeOpId(COMPOSITE_MULT);
    if (variant == 2) image->addNode(new KisGroupLayer(image, QStringLiteral("Group"), 255), image->root());
    if (variant == 3) layer->setVisible(false);
    if (variant == 5 || variant == 10 || variant == 11) {
        KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Mask"));
        mask->initSelection(layer);
        if (variant != 5) {
            auto selection = mask->selection()->pixelSelection();
            selection->setDefaultPixel(KoColor::createTransparent(selection->colorSpace()));
            selection->select(Bounds);
            if (variant == 10) mask->setVisible(false);
            else mask->setUserLocked(true);
        }
        image->addNode(mask, layer);
    }
    if (variant == 6) image->setDefaultProjectionColor(KoColor(Qt::white, cs));
    if (variant == 7) layer->setAlphaLocked(true);
    if (variant == 8) {
        QBitArray channels(4, true);
        channels.setBit(0, false);
        layer->setChannelLockFlags(channels);
    }
    if (variant == 9) layer->setUserLocked(true);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setCurrentImage(image);
    if (variant == 12 || variant == 15) {
        auto grid = document->gridConfig();
        grid.setShowGrid(true);
        grid.setSpacing({17, 19});
        document->setGridConfig(grid);
    }
    if (variant == 13 || variant == 15) {
        auto guides = document->guidesConfig();
        guides.setVerticalGuideLines({8.5, 21.25});
        document->setGuidesConfig(guides);
    }
    if (variant == 14 || variant == 15) {
        auto mirror = document->mirrorAxisConfig();
        mirror.setMirrorHorizontal(true);
        mirror.setAxisPosition({7.5, 11.25});
        document->setMirrorAxisConfig(mirror);
    }
    image->waitForDone();
    const bool saved = document->exportDocumentSync(path, "application/x-krita");
    image.clear();
    return saved;
}
}

class KisGpuKraLoadJobTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void registeredDocumentSurvivesSuspensionDuringLoad();
    void restoresFrozenNativePixels();
    void unsupportedNativeContentIsRefusedBeforeGpuWork();
    void readAndBudgetFailuresPublishNoDocument();
    void lostDeviceCanReloadOnAnotherDevice();
    void preservesSparseCoordinatesAndEmptyDocuments();
    void preservesNativeVisibilityAndLock_data();
    void preservesNativeVisibilityAndLock();
    void documentInformationSurvivesGpuRoundTrip();
    void documentSettingsSurviveGpuRoundTrip_data();
    void documentSettingsSurviveGpuRoundTrip();
};

void KisGpuKraLoadJobTest::registeredDocumentSurvivesSuspensionDuringLoad()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("registered.kra"));
    QVERIFY(nativeFixture(path));
    KisGpuDocumentDevice owner;
    const auto id = owner.createDocument({});
    auto document = owner.document(id);
    Load load(document, path);
    load.poll();
    QCOMPARE(load.state(), Load::State::Restoring);
    QVERIFY(document->layers().isEmpty());
    QCOMPARE(owner.store(id)->statistics().pixelUploadBytes, quint64(0));
    QVERIFY(owner.suspend());
    load.poll();
    QCOMPARE(load.state(), Load::State::Restoring);
    QVERIFY(!load.metadata());
    QVERIFY(owner.resume());
    QVERIFY(finish(load));
    QCOMPARE(owner.document(id), document);
    auto store = owner.store(id);
    QCOMPARE(store->statistics().pixelUploadBytes, Store::TileBytes);
    auto pixels = store->readback(document->layers().first().paint.pixels, Bounds);
    QTRY_COMPARE((store->poll(), pixels.completion.status()), Store::Status::Succeeded);
    QCOMPARE(pixels.bytes().left(4), QByteArray::fromHex("ff0000ff"));
    const auto before = store->statistics();
    Load overwrite(document, path);
    QCOMPARE(overwrite.error(), Load::Error::InvalidInput);
    QCOMPARE(document->layers().size(), qsizetype(1));
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(owner.device()->errorCount(), 0);
}
void KisGpuKraLoadJobTest::documentInformationSurvivesGpuRoundTrip()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("metadata.kra"));
    const auto savedPath = directory.filePath(QStringLiteral("roundtrip.kra"));
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document source(store, 8);
    source.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(source, 1, 0xCC112233));
    const auto expected = gpu.read(source.layers()[0].paint.pixels, Bounds);
    KisGpuKraMetadata metadata{Bounds.size(), QStringLiteral("Snapshot"), 1.25, 1.5};
    metadata.documentInfoXml = QByteArray(R"(<document-info><about><title>Fixture title 雪🖌</title>
        <abstract><![CDATA[<p>Fixture & description</p>]]></abstract><license>CC0</license>
        <creation-date>2001-02-03T04:05:06</creation-date><editing-cycles>7</editing-cycles>
        </about><author><full-name>Fixture Artist</full-name>
        <contact type="email">fixture@example.test</contact></author></document-info>)");
    {
        Save save(store, source.layers(), 2, metadata, path);
        QVERIFY2(finish(save), qPrintable(save.errorMessage()));
    }
    std::shared_ptr<Document> restored;
    {
        restored = std::make_shared<Document>(store, 8);
        Load load(restored, path);
        QVERIFY(finish(load));
        metadata = *load.metadata();
        QVERIFY(!metadata.documentInfoXml.isEmpty());
        QDomDocument xml;
        QVERIFY(xml.setContent(metadata.documentInfoXml));
        KoDocumentInfo info;
        QVERIFY(info.load(xml));
        QCOMPARE(info.aboutInfo("title"), QStringLiteral("Fixture title 雪🖌"));
        QCOMPARE(info.aboutInfo("editing-cycles"), QStringLiteral("8"));
    }
    QCOMPARE(gpu.read(restored->layers()[0].paint.pixels, Bounds), expected);
    const auto before = store->statistics();
    for (const quint64 budget : {quint64(metadata.documentInfoXml.size() - 1),
                                quint64(metadata.documentInfoXml.size()) + Store::TileBytes}) {
        auto target = std::make_shared<Document>(store, 8);
        Load tooSmall(target, path, budget);
        QVERIFY(!finish(tooSmall));
        QCOMPARE(tooSmall.error(), Load::Error::SnapshotBudgetExceeded);
        QVERIFY(!tooSmall.metadata());
        QVERIFY(target->layers().isEmpty());
    }
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    {
        Save save(store, restored->layers(), restored->recovery().nextId, metadata, savedPath);
        restored.reset();
        metadata.documentInfoXml.clear();
        QVERIFY2(finish(save), qPrintable(save.errorMessage()));
    }
    QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
    native->setFileBatchMode(true);
    QVERIFY(native->loadNativeFormat(savedPath));
    native->image()->waitForDone();
    const auto info = native->documentInfo();
    QCOMPARE(info->aboutInfo("title"), QStringLiteral("Fixture title 雪🖌"));
    QCOMPARE(info->aboutInfo("abstract"), QStringLiteral("<p>Fixture & description</p>"));
    QCOMPARE(info->aboutInfo("license"), QStringLiteral("CC0"));
    QCOMPARE(info->aboutInfo("creation-date"), QStringLiteral("2001-02-03T04:05:06"));
    QCOMPARE(info->aboutInfo("editing-cycles"), QStringLiteral("9"));
    QCOMPARE(info->authorInfo("creator"), QStringLiteral("Fixture Artist"));
    QCOMPARE(info->authorContactInfo(), QStringList{QStringLiteral("fixture@example.test")});
    auto layer = dynamic_cast<KisPaintLayer *>(native->image()->root()->at(0).data());
    QVERIFY(layer);
    QByteArray actual(expected.size(), '\0');
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), Bounds);
    for (qsizetype i = 0; i < actual.size(); i += 4) std::swap(actual[i], actual[i + 2]);
    QCOMPARE(actual, expected);
}

void KisGpuKraLoadJobTest::restoresFrozenNativePixels()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("snapshot.kra"));
    QByteArray expectedFirst, expectedSecond, expectedProjection;
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
        Document document(store, 8);
        document.addLayer(QStringLiteral("Paint 雪🖌"));
        document.addLayer(QStringLiteral("Top"));
        QVERIFY(paint(document, 1, 0xCC112233));
        QVERIFY(paint(document, 1, 0x80123456, Document::Target::Mask));
        QVERIFY(paint(document, 2, 0xFF778899));
        document.setLayerProperties(1, 177, Store::CompositeOp::Over);
        document.setLayerProperties(2, 77, Store::CompositeOp::Erase);
        document.setLayerLocked(1, true);
        document.setLayerVisible(2, false);
        expectedFirst = gpu.read(document.layers()[0].paint.pixels, Bounds);
        expectedSecond = gpu.read(document.layers()[1].paint.pixels, Bounds);
        const auto projection = store->project(store->emptyVersion(), document.projectionLayers(false), Bounds);
        expectedProjection = gpu.read(projection.version, Bounds);
        Save save(store, document.layers(), 3, {Bounds.size(), QStringLiteral("Snapshot"), 1.25, 1.5}, path);
        QVERIFY(finish(save));
    }
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    auto document = std::make_shared<Document>(store, 8);
    Load load(document, path);
    QVERIFY(document->layers().isEmpty());
    QVERIFY(!load.metadata());
    QVERIFY(finish(load));
    QCOMPARE(load.error(), Load::Error::None);
    QCOMPARE(load.metadata()->canvasSize, Bounds.size());
    QCOMPARE(load.metadata()->xResolution, 1.25);
    QCOMPARE(load.metadata()->yResolution, 1.5);
    QCOMPARE(document->historySize(), qsizetype(1));
    const auto layers = document->layers();
    QCOMPARE(layers.size(), qsizetype(2));
    QCOMPARE(layers[0].name, QStringLiteral("Paint 雪🖌"));
    QCOMPARE(layers[1].name, QStringLiteral("Top"));
    QCOMPARE(layers[0].paint.opacity, quint8(177));
    QCOMPARE(layers[1].paint.operation, Store::CompositeOp::Erase);
    QVERIFY(layers[0].locked);
    QVERIFY(!layers[1].visible);
    QCOMPARE(gpu.read(layers[0].paint.pixels, Bounds), expectedFirst);
    QCOMPARE(gpu.read(layers[1].paint.pixels, Bounds), expectedSecond);
    const auto projection = store->project(store->emptyVersion(), document->projectionLayers(false), Bounds);
    QCOMPARE(gpu.read(projection.version, Bounds), expectedProjection);
    const auto recovery = document->recovery();
    for (const auto &record : recovery.history[0]) {
        QVERIFY(record->pixels->edits.isEmpty());
        if (record->mask) QVERIFY(record->mask->edits.isEmpty());
    }
    const auto before = store->statistics();
    QCOMPARE(document->setLayerLocked(layers[0].id, false), Document::Result::Accepted);
    QVERIFY(paint(*document, layers[0].id, 0xFF0099AA));
    QVERIFY(document->undo());
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(gpu.read(document->layers()[0].paint.pixels, Bounds), expectedFirst);
    QCOMPARE(document->addLayer(QStringLiteral("New")).id, quint64(3));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuKraLoadJobTest::unsupportedNativeContentIsRefusedBeforeGpuWork()
{
    QTemporaryDir directory;
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    const auto before = store->statistics();
    for (int variant : {1, 2, 4, 5, 6, 7, 8, 10, 11}) {
        const auto path = directory.filePath(QString::number(variant) + QStringLiteral(".kra"));
        QVERIFY(nativeFixture(path, variant));
        QScopedPointer<KisDocument> original(KisPart::instance()->createDocument());
        original->setFileBatchMode(true);
        QVERIFY(original->loadNativeFormat(path));
        original->image()->waitForDone();
        auto document = std::make_shared<Document>(store, 8);
        Load load(document, path);
        QVERIFY(!finish(load));
        QCOMPARE(load.state(), Load::State::Failed);
        QCOMPARE(load.error(), Load::Error::UnsupportedDocument);
        QVERIFY(document->layers().isEmpty());
        QVERIFY(!load.metadata());
        QCOMPARE(store->statistics().submissions, before.submissions);
        QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    }
}

void KisGpuKraLoadJobTest::documentSettingsSurviveGpuRoundTrip_data()
{
    QTest::addColumn<int>("variant");
    QTest::newRow("grid") << 12;
    QTest::newRow("guides") << 13;
    QTest::newRow("mirror-axis") << 14;
    QTest::newRow("combined") << 15;
}

void KisGpuKraLoadJobTest::documentSettingsSurviveGpuRoundTrip()
{
    QFETCH(int, variant);
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("settings.kra"));
    const auto savedPath = directory.filePath(QStringLiteral("resaved.kra"));
    QVERIFY(nativeFixture(path, variant));
    QScopedPointer<KisDocument> original(KisPart::instance()->createDocument());
    original->setFileBatchMode(true);
    QVERIFY(original->loadNativeFormat(path));
    original->image()->waitForDone();
    if (variant == 12 || variant == 15) {
        QVERIFY(original->gridConfig().showGrid());
        QCOMPARE(original->gridConfig().spacing(), QPoint(17, 19));
    }
    if (variant == 13 || variant == 15)
        QCOMPARE(original->guidesConfig().verticalGuideLines(), QList<qreal>({8.5, 21.25}));
    if (variant == 14 || variant == 15) {
        QVERIFY(original->mirrorAxisConfig().mirrorHorizontal());
        QCOMPARE(original->mirrorAxisConfig().axisPosition(), QPointF(7.5, 11.25));
    }
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    auto document = std::make_shared<Document>(store, 8);
    Load load(document, path);
    QVERIFY(finish(load));
    QVERIFY(load.metadata());
    auto metadata = *load.metadata();
    QVERIFY(!metadata.documentSettingsXml.isEmpty());
    const auto before = store->statistics();
    auto refusedDocument = std::make_shared<Document>(store, 8);
    Load tooSmall(refusedDocument, path,
        quint64(metadata.documentInfoXml.size() + metadata.documentSettingsXml.size()) - 1);
    QVERIFY(!finish(tooSmall));
    QCOMPARE(tooSmall.error(), Load::Error::SnapshotBudgetExceeded);
    QVERIFY(refusedDocument->layers().isEmpty());
    QVERIFY(!tooSmall.metadata());
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    Save save(store, document->layers(), document->recovery().nextId, metadata, savedPath);
    metadata.documentSettingsXml.fill('X');
    QVERIFY(finish(save));
    QScopedPointer<KisDocument> restored(KisPart::instance()->createDocument());
    restored->setFileBatchMode(true);
    QVERIFY(restored->loadNativeFormat(savedPath));
    restored->image()->waitForDone();
    QCOMPARE(restored->gridConfig(), original->gridConfig());
    QCOMPARE(restored->guidesConfig(), original->guidesConfig());
    QCOMPARE(restored->mirrorAxisConfig(), original->mirrorAxisConfig());
    QCOMPARE(restored->image()->size(), original->image()->size());
    QCOMPARE(gpu.read(document->layers().first().paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuKraLoadJobTest::preservesNativeVisibilityAndLock_data()
{
    QTest::addColumn<int>("variant");
    QTest::newRow("hidden") << 3;
    QTest::newRow("locked") << 9;
}

void KisGpuKraLoadJobTest::preservesNativeVisibilityAndLock()
{
    QFETCH(int, variant);
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("flags.kra"));
    QVERIFY(nativeFixture(path, variant));
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    auto document = std::make_shared<Document>(store, 8);
    Load load(document, path);
    QVERIFY(finish(load));
    QCOMPARE(document->layers().size(), 1);
    QCOMPARE(document->layers()[0].visible, variant != 3);
    QCOMPARE(document->layers()[0].locked, variant == 9);
    QCOMPARE(document->projectionLayers().size(), variant == 3 ? 0 : 1);
    QCOMPARE(document->begin(1).result, Document::Result::LayerNotEditable);
    QCOMPARE(document->begin(1, Document::Target::Mask).result, Document::Result::LayerNotEditable);
    QCOMPARE(gpu.read(document->layers()[0].paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    const auto before = store->statistics();
    QCOMPARE(document->setLayerVisible(1, true), Document::Result::Accepted);
    QCOMPARE(document->setLayerLocked(1, false), Document::Result::Accepted);
    const auto edit = document->begin(1);
    QCOMPARE(edit.result, Document::Result::Accepted);
    QCOMPARE(document->cancel(edit.token), Document::Result::Accepted);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuKraLoadJobTest::readAndBudgetFailuresPublishNoDocument()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("snapshot.kra"));
    QVERIFY(nativeFixture(path));
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Load invalid({}, path);
    QCOMPARE(invalid.error(), Load::Error::InvalidInput);
    auto missingTarget = std::make_shared<Document>(store, 8);
    Load missing(missingTarget, directory.filePath(QStringLiteral("missing.kra")));
    QVERIFY(!finish(missing));
    QCOMPARE(missing.error(), Load::Error::LoadFailed);
    auto smallTarget = std::make_shared<Document>(store, 8);
    Load small(smallTarget, path, 1024);
    QVERIFY(!finish(small));
    QCOMPARE(small.error(), Load::Error::SnapshotBudgetExceeded);
    QVERIFY(smallTarget->layers().isEmpty());
    QCOMPARE(store->statistics().submissions, quint64(0));
    QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
    auto tight = std::make_shared<Store>(gpu.owner, 4096);
    auto refusedTarget = std::make_shared<Document>(tight, 8);
    Load refused(refusedTarget, path);
    QVERIFY(!finish(refused));
    QCOMPARE(refused.error(), Load::Error::GpuRejected);
    QCOMPARE(refused.gpuError(), Store::Error::BudgetExceeded);
    QVERIFY(refusedTarget->layers().isEmpty());
    QCOMPARE(tight->statistics().residentBytes, quint64(0));
    auto retryTarget = std::make_shared<Document>(store, 8);
    Load retry(retryTarget, path);
    QVERIFY(finish(retry));
    QCOMPARE(retryTarget->layers().size(), qsizetype(1));
}

void KisGpuKraLoadJobTest::lostDeviceCanReloadOnAnotherDevice()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("snapshot.kra"));
    QVERIFY(nativeFixture(path));
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
        auto document = std::make_shared<Document>(store, 8);
        Load load(document, path);
        load.poll();
        QCOMPARE(load.state(), Load::State::Restoring);
        gpu.owner.destroy();
        QVERIFY(!finish(load));
        QCOMPARE(load.error(), Load::Error::GpuRejected);
        QCOMPARE(load.gpuError(), Store::Error::DeviceLost);
        QVERIFY(document->layers().isEmpty());
    }
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    auto document = std::make_shared<Document>(store, 8);
    Load retry(document, path);
    QVERIFY(finish(retry));
    const auto pixels = gpu.read(document->layers()[0].paint.pixels, Bounds);
    QCOMPARE(pixels.size(), Bounds.width() * Bounds.height() * 4);
    QCOMPARE(pixels.left(4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuKraLoadJobTest::preservesSparseCoordinatesAndEmptyDocuments()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("sparse.kra"));
    const QRect comparison(-64, -64, 256, 256);
    {
        QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
        native->setFileBatchMode(true);
        auto cs = KoColorSpaceRegistry::instance()->rgb8(KoColorSpaceRegistry::instance()->p709SRGBProfile());
        KisImageSP image = new KisImage(nullptr, 128, 128, cs, QStringLiteral("Sparse"));
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        auto device = layer->paintDevice();
        device->setX(7); device->setY(9);
        const auto raw = QByteArray::fromHex("11223300445566ff");
        for (const auto point : {QPoint(-1, -1), QPoint(0, 0), QPoint(63, 63), QPoint(64, 64), QPoint(117, 122)}) {
            device->writeBytes(reinterpret_cast<const quint8 *>(raw.constData()), point.x(), point.y(), 2, 1);
        }
        image->addNode(layer, image->root());
        native->setCurrentImage(image);
        image->waitForDone();
        QVERIFY(native->exportDocumentSync(path, "application/x-krita"));
    }
    QByteArray expected(comparison.width() * comparison.height() * 4, '\0');
    {
        QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
        native->setFileBatchMode(true);
        QVERIFY(native->loadNativeFormat(path));
        native->image()->waitForDone();
        auto layer = dynamic_cast<KisPaintLayer *>(native->image()->root()->at(0).data());
        QVERIFY(layer);
        layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(expected.data()), comparison);
        for (qsizetype i = 0; i < expected.size(); i += 4) std::swap(expected[i], expected[i + 2]);
    }
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    auto document = std::make_shared<Document>(store, 8);
    Load load(document, path);
    QVERIFY(finish(load));
    QCOMPARE(gpu.read(document->layers()[0].paint.pixels, comparison), expected);
    const auto emptyPath = directory.filePath(QStringLiteral("empty.kra"));
    const auto before = store->statistics();
    Save save(store, {}, 1, {Bounds.size(), QStringLiteral("Empty")}, emptyPath);
    QVERIFY(finish(save));
    auto emptyDocument = std::make_shared<Document>(store, 8);
    Load empty(emptyDocument, emptyPath);
    QVERIFY(finish(empty));
    QVERIFY(emptyDocument->layers().isEmpty());
    QCOMPARE(emptyDocument->historySize(), qsizetype(1));
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
}
KISTEST_MAIN(KisGpuKraLoadJobTest)
#include "KisGpuKraLoadJobTest.moc"
