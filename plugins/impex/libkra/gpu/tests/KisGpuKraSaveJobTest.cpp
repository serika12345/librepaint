/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuKraSaveJob.h"
#include "KisGpuDocumentReadback.h"
#include "KisGpuTestDevice.h"
#include <testui.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <metadata/KoDocumentInfo.h>
#include <canvas/kis_grid_config.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoCompositeOpRegistry.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <limits>
#include <utility>

namespace {
using Store = KisGpuTileStore;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
using Save = KisGpuKraSaveJob;
const QRect Bounds(0, 0, 32, 32);
bool settle(Document &document)
{
    QElapsedTimer timer; timer.start();
    while (document.state() == Document::State::Committing && timer.elapsed() < 5000) {
        document.poll(); QTest::qWait(1);
    }
    return document.state() == Document::State::Idle;
}
bool paint(Document &document, Document::LayerId id, quint32 rgba, Document::Target target = Document::Target::Pixels)
{
    const auto edit = document.begin(id, target);
    if (edit.result != Document::Result::Accepted) return false;
    Store::DabCommand command;
    command.center = {8, 8}; command.diameter = {12, 12}; command.rgba = rgba;
    return edit.session->append(edit.token, {command}, Bounds) == Session::Result::Accepted
        && document.commit(edit.token) == Document::Result::Accepted && settle(document);
}
bool finish(Save &save)
{
    QElapsedTimer timer; timer.start();
    while (save.state() != Save::State::Succeeded && save.state() != Save::State::Failed
           && timer.elapsed() < 15000) { save.poll(); QTest::qWait(1); }
    return save.state() == Save::State::Succeeded;
}
QByteArray rgba(KisPaintDeviceSP device)
{
    QByteArray result(Bounds.width() * Bounds.height() * 4, '\0');
    device->readBytes(reinterpret_cast<quint8 *>(result.data()), Bounds);
    for (qsizetype i = 0; i < result.size(); i += 4) std::swap(result[i], result[i + 2]);
    return result;
}
QByteArray alpha(const QByteArray &rgba)
{
    QByteArray result(rgba.size() / 4, '\0');
    for (qsizetype i = 0; i < result.size(); ++i) result[i] = rgba[i * 4 + 3];
    return result;
}
}

/** Frozen GPU layers round-trip through the existing native KRA parser and writer. */
class KisGpuKraSaveJobTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void savesFrozenLayersAndMaskAfterGpuClose();
    void invalidInputAndSnapshotRefusalPreserveExistingFile();
    void unrepresentableNamesAreRejectedBeforeTransfer();
    void invalidDocumentInformationPreservesDestination();
    void invalidDocumentSettingsPreserveDestination();
    void deviceLossPreservesDestinationAndWritingDrainsOnClose();
};

void KisGpuKraSaveJobTest::savesFrozenLayersAndMaskAfterGpuClose()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("snapshot.kra"));
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    auto document = std::make_unique<Document>(store, 8);
    const auto first = document->addLayer(QStringLiteral("Paint 雪🖌")).id;
    const auto second = document->addLayer(QStringLiteral("Top")).id;
    QVERIFY(paint(*document, first, 0xCC112233));
    QVERIFY(paint(*document, first, 0x80123456, Document::Target::Mask));
    QVERIFY(paint(*document, second, 0xFF778899));
    document->setLayerProperties(first, 77, Store::CompositeOp::Erase);
    document->setLayerLocked(first, true);
    document->setLayerVisible(second, false);
    const auto expectedFirst = gpu.read(document->layers()[0].paint.pixels, Bounds);
    const auto expectedMask = alpha(gpu.read(document->layers()[0].paint.mask, Bounds));
    const auto expectedSecond = gpu.read(document->layers()[1].paint.pixels, Bounds);
    Save::Metadata metadata{Bounds.size(), QStringLiteral("GPU snapshot"), 1.25, 1.5};
    metadata.documentInfoXml = QByteArray(R"(<document-info><about>
        <title>Fixture title 雪🖌</title><abstract><![CDATA[<p>Fixture & description</p>]]></abstract>
        <license>CC0</license><creation-date>2001-02-03T04:05:06</creation-date>
        <editing-cycles>7</editing-cycles></about><author><full-name>Fixture Artist</full-name>
        <contact type="email">fixture@example.test</contact></author></document-info>)");
    Save save(store, document->layers(), document->recovery().nextId, metadata, path);
    metadata.documentInfoXml.fill('X');
    document->renameLayer(first, QStringLiteral("Changed"));
    document->removeLayer(second);
    document.reset();
    store.reset();
    QElapsedTimer timer; timer.start();
    while (save.state() == Save::State::Reading && timer.elapsed() < 5000) {
        save.poll(); QTest::qWait(1);
    }
    QVERIFY(save.state() != Save::State::Reading);
    gpu.owner.destroy();
    QVERIFY2(finish(save), qPrintable(save.errorMessage()));
    QCOMPARE(save.error(), Save::Error::None);
    QScopedPointer<KisDocument> loaded(KisPart::instance()->createDocument());
    loaded->setFileBatchMode(true);
    QVERIFY(loaded->loadNativeFormat(path));
    loaded->image()->waitForDone();
    const auto image = loaded->image();
    QCOMPARE(image->bounds(), Bounds);
    QCOMPARE(image->xRes(), 1.25);
    QCOMPARE(image->yRes(), 1.5);
    QCOMPARE(image->colorSpace()->profile()->rawData(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->rawData());
    QCOMPARE(loaded->documentInfo()->aboutInfo("title"), QStringLiteral("Fixture title 雪🖌"));
    QCOMPARE(loaded->documentInfo()->aboutInfo("abstract"), QStringLiteral("<p>Fixture & description</p>"));
    QCOMPARE(loaded->documentInfo()->aboutInfo("license"), QStringLiteral("CC0"));
    QCOMPARE(loaded->documentInfo()->aboutInfo("creation-date"), QStringLiteral("2001-02-03T04:05:06"));
    QCOMPARE(loaded->documentInfo()->aboutInfo("editing-cycles"), QStringLiteral("8"));
    QCOMPARE(loaded->documentInfo()->authorInfo("creator"), QStringLiteral("Fixture Artist"));
    QCOMPARE(loaded->documentInfo()->authorContactInfo(), QStringList{QStringLiteral("fixture@example.test")});
    QCOMPARE(image->root()->childCount(), quint32(2));
    const auto firstLayer = dynamic_cast<KisPaintLayer *>(image->root()->at(0).data());
    const auto secondLayer = dynamic_cast<KisPaintLayer *>(image->root()->at(1).data());
    QVERIFY(firstLayer); QVERIFY(secondLayer);
    QCOMPARE(firstLayer->name(), QStringLiteral("Paint 雪🖌"));
    QCOMPARE(firstLayer->opacity(), quint8(77));
    QCOMPARE(firstLayer->compositeOpId(), QString(COMPOSITE_ERASE));
    QCOMPARE(secondLayer->name(), QStringLiteral("Top"));
    QVERIFY(firstLayer->userLocked());
    QVERIFY(firstLayer->visible());
    QVERIFY(!secondLayer->visible());
    QVERIFY(!secondLayer->userLocked());
    QCOMPARE(rgba(firstLayer->paintDevice()), expectedFirst);
    QCOMPARE(rgba(secondLayer->paintDevice()), expectedSecond);
    QCOMPARE(firstLayer->childCount(), quint32(1));
    const auto mask = dynamic_cast<KisTransparencyMask *>(firstLayer->at(0).data());
    QVERIFY(mask);
    QByteArray actualMask(Bounds.width() * Bounds.height(), '\0');
    mask->selection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actualMask.data()), Bounds);
    QCOMPARE(actualMask, expectedMask);
}

void KisGpuKraSaveJobTest::invalidInputAndSnapshotRefusalPreserveExistingFile()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("existing.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    QCOMPARE(original.write("KEEP"), qint64(4));
    original.close();
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    const auto before = store->statistics();
    Save invalid(store, document.layers(), 2, {{0, 32}, QStringLiteral("Invalid")}, path);
    QCOMPARE(invalid.state(), Save::State::Failed);
    QCOMPARE(invalid.error(), Save::Error::InvalidInput);
    Save badResolution(store, document.layers(), 2,
        {Bounds.size(), QStringLiteral("Invalid"), std::numeric_limits<double>::quiet_NaN(), 1}, path);
    QCOMPARE(badResolution.error(), Save::Error::InvalidInput);
    Save small(store, document.layers(), 2, {Bounds.size(), QStringLiteral("Small")}, path, 1024);
    QVERIFY(!finish(small));
    QCOMPARE(small.state(), Save::State::Failed);
    QCOMPARE(small.error(), Save::Error::ReadbackFailed);
    QCOMPARE(small.readbackError(), KisGpuDocumentReadback::Error::BudgetExceeded);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("KEEP"));
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuKraSaveJobTest::unrepresentableNamesAreRejectedBeforeTransfer()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("existing.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    QCOMPARE(original.write("KEEP"), qint64(4));
    original.close();
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    const auto before = store->statistics();
    for (const auto invalid : {QChar(0), QChar(1), QChar(0xD800), QChar(0xDC00), QChar(0xFFFE)}) {
        document.renameLayer(1, QStringLiteral("Invalid") + invalid);
        Save save(store, document.layers(), 2, {Bounds.size(), QStringLiteral("Snapshot")}, path);
        QCOMPARE(save.state(), Save::State::Failed);
        QCOMPARE(save.error(), Save::Error::InvalidInput);
    }
    Save invalidTitle(store, {}, 1, {Bounds.size(), QStringLiteral("Invalid") + QChar(0)}, path);
    QCOMPARE(invalidTitle.error(), Save::Error::InvalidInput);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("KEEP"));
}

void KisGpuKraSaveJobTest::invalidDocumentInformationPreservesDestination()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("existing.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    QCOMPARE(original.write("KEEP"), qint64(4));
    original.close();
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    const auto before = store->statistics();
    for (const QByteArray &xml : {QByteArray("<document-info>"),
            QByteArray("<document-info><about/></document-info>"),
            QByteArray("<other><about/><author/></other>"),
            QByteArray("<document-info><about><title>&#0;</title></about><author/></document-info>")}) {
        Save::Metadata metadata{Bounds.size(), QStringLiteral("Snapshot")};
        metadata.documentInfoXml = xml;
        Save save(store, document.layers(), 2, metadata, path);
        QCOMPARE(save.state(), Save::State::Failed);
        QCOMPARE(save.error(), Save::Error::InvalidInput);
    }
    Save::Metadata metadata{Bounds.size(), QStringLiteral("Snapshot")};
    metadata.documentInfoXml = "<document-info><about/><author/></document-info>";
    Save tooSmall(store, document.layers(), 2, metadata, path, metadata.documentInfoXml.size() - 1);
    QCOMPARE(tooSmall.state(), Save::State::Failed);
    QCOMPARE(tooSmall.error(), Save::Error::ReadbackFailed);
    QCOMPARE(tooSmall.readbackError(), KisGpuDocumentReadback::Error::BudgetExceeded);
    KisGpuDocumentReadback probe(store, document.layers(), 2);
    const auto imageBudget = probe.reservedRecoveryBytes();
    Save combined(store, document.layers(), 2, metadata, path, imageBudget);
    QCOMPARE(combined.state(), Save::State::Failed);
    QCOMPARE(combined.readbackError(), KisGpuDocumentReadback::Error::BudgetExceeded);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("KEEP"));
}

void KisGpuKraSaveJobTest::invalidDocumentSettingsPreserveDestination()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("settings.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    QCOMPARE(original.write("KEEP"), qint64(4));
    original.close();
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    const auto before = store->statistics();
    QDomDocument xml;
    auto root = xml.createElement(QStringLiteral("document-settings"));
    xml.appendChild(root);
    KisGridConfig grid;
    grid.setShowGrid(true);
    root.appendChild(grid.saveDynamicDataToXml(xml, QStringLiteral("grid")));
    const auto valid = xml.toByteArray();
    root.appendChild(root.firstChildElement().cloneNode(true));
    for (const auto &invalid : {QByteArray("<document-settings>"), QByteArray("<other/>"),
            QByteArray("<document-settings><unknown/></document-settings>"),
            QByteArray("<document-settings><grid/></document-settings>"), xml.toByteArray()}) {
        Save::Metadata metadata{Bounds.size(), QStringLiteral("Snapshot")};
        metadata.documentSettingsXml = invalid;
        Save save(store, document.layers(), 2, metadata, path);
        QCOMPARE(save.state(), Save::State::Failed);
        QCOMPARE(save.error(), Save::Error::InvalidInput);
    }
    Save::Metadata metadata{Bounds.size(), QStringLiteral("Snapshot")};
    metadata.documentSettingsXml = valid;
    Save tooSmall(store, document.layers(), 2, metadata, path, valid.size() - 1);
    QCOMPARE(tooSmall.error(), Save::Error::ReadbackFailed);
    QCOMPARE(tooSmall.readbackError(), KisGpuDocumentReadback::Error::BudgetExceeded);
    KisGpuDocumentReadback probe(store, document.layers(), 2);
    Save combined(store, document.layers(), 2, metadata, path, probe.reservedRecoveryBytes() + valid.size() - 1);
    QCOMPARE(combined.error(), Save::Error::ReadbackFailed);
    QCOMPARE(combined.readbackError(), KisGpuDocumentReadback::Error::BudgetExceeded);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("KEEP"));
}

void KisGpuKraSaveJobTest::deviceLossPreservesDestinationAndWritingDrainsOnClose()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("existing.kra"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    QCOMPARE(original.write("KEEP"), qint64(4));
    original.close();
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
        Document document(store, 2);
        document.addLayer(QStringLiteral("Paint"));
        QVERIFY(paint(document, 1, 0xFF112233));
        Save save(store, document.layers(), 2, {Bounds.size(), QStringLiteral("Snapshot")}, path);
        save.poll();
        gpu.owner.destroy();
        QVERIFY(!finish(save));
        QCOMPARE(save.error(), Save::Error::ReadbackFailed);
        QCOMPARE(save.readbackError(), KisGpuDocumentReadback::Error::DeviceLost);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), QByteArray("KEEP"));
        original.close();
    }
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
        Document document(store, 2);
        document.addLayer(QStringLiteral("Paint"));
        QVERIFY(paint(document, 1, 0xFF112233));
        Save save(store, document.layers(), 2, {Bounds.size(), QStringLiteral("Snapshot")}, path);
        QElapsedTimer timer; timer.start();
        while ((save.state() == Save::State::Reading || save.state() == Save::State::Preparing)
               && timer.elapsed() < 5000) { save.poll(); QTest::qWait(1); }
        QVERIFY(save.state() == Save::State::Writing || save.state() == Save::State::Succeeded);
    }
    QScopedPointer<KisDocument> loaded(KisPart::instance()->createDocument());
    loaded->setFileBatchMode(true);
    QVERIFY(loaded->loadNativeFormat(path));
    loaded->image()->waitForDone();
    QCOMPARE(loaded->image()->root()->childCount(), quint32(1));
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    Save unavailable(store, document.layers(), 2, {Bounds.size(), QStringLiteral("Snapshot")},
                     directory.filePath(QStringLiteral("absent/snapshot.kra")));
    QVERIFY(!finish(unavailable));
    QCOMPARE(unavailable.error(), Save::Error::SaveFailed);
    QVERIFY(!unavailable.errorMessage().isEmpty());
}

KISTEST_MAIN(KisGpuKraSaveJobTest)
#include "KisGpuKraSaveJobTest.moc"
