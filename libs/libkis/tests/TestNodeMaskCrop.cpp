/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestNode.h"
#include <KisGpuPixelTolerance.h>
#include <Document.h>
#include <Node.h>
#include <KisGlobalResourcesInterface.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <kis_adjustment_layer.h>
#include <kis_crop_saved_extra_data.h>
#include <kis_filter_mask.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_layer_utils.h>
#include <kis_paint_layer.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_transparency_mask.h>
#include <kis_undo_adapter.h>
#include <commands/kis_set_global_selection_command.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <filter/kis_filter_registry.h>
#include <gpu/KisGpuPaintImage.h>
#include <kundo2stack.h>
#include <simpletest.h>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <memory>

void TestNode::testMaskCropsKeepPixelsSelectionHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("maskDefault");
    QTest::addColumn<QString>("operation");
    QTest::newRow("byte-default0-image") << false << 0 << QStringLiteral("image");
    QTest::newRow("word-default255-filter") << true << 255 << QStringLiteral("filter");
}

void TestNode::testMaskCropsKeepPixelsSelectionHistoryAndKra()
{
    QFETCH(bool, words); QFETCH(int, maskDefault); QFETCH(QString, operation);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    QTemporaryDir files; QVERIFY(files.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            files.setAutoRemove(false);
            qInfo() << "Mask crop artifacts retained at" << files.path();
        }
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    std::unique_ptr<KisDocument> source(KisPart::instance()->createDocument());
    source->setFileBatchMode(true);
    KisImageSP image = new KisImage(source->createUndoStore(), 64, 48, cs, "Mask crop");
    source->setCurrentImage(image);
    KisGroupLayerSP group = new KisGroupLayer(image, "Group", 255); image->addNode(group, image->root());
    KisPaintLayerSP paint = new KisPaintLayer(image, "Paint", 255); image->addNode(paint, group);
    paint->paintDevice()->moveTo(17, -9);
    const KoColor color(QColor(61, 137, 219, 197), cs);
    paint->paintDevice()->fill(-24, -16, 112, 96, color.data());
    const auto selection = [&](int seed, quint8 alpha, QPoint position) {
        KisSelectionSP result = new KisSelection;
        auto pixels = result->pixelSelection();
        pixels->moveTo(position);
        pixels->setDefaultPixel(KoColor(&alpha, pixels->colorSpace()));
        QByteArray values(19 * 15, Qt::Uninitialized);
        for (int i = 0; i < values.size(); ++i) values[i] = char((i * 43 + seed) % 256);
        pixels->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), 4 + seed, 3 + seed, 19, 15);
        pixels->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), -25, 67, 19, 15);
        return result;
    };
    const auto filter = KisFilterRegistry::instance()->value("maximize"); QVERIFY(filter);
    const auto config = filter->defaultConfiguration(KisGlobalResourcesInterface::instance())->cloneWithResourcesSnapshot();
    KisFilterMaskSP filterMask = new KisFilterMask(image, "Filter");
    filterMask->initSelection(selection(1, maskDefault, {-65, 73}), paint);
    filterMask->setFilter(config); image->addNode(filterMask, paint);
    KisAdjustmentLayerSP adjustment = new KisAdjustmentLayer(image, "Adjustment", config->cloneWithResourcesSnapshot(), selection(2, maskDefault, {77, -69}));
    image->addNode(adjustment, group, paint);
    KisTransparencyMaskSP child = new KisTransparencyMask(image, "Child");
    child->setSelection(selection(3, 255, {-31, 19})); image->addNode(child, adjustment);
    KisTransparencyMaskSP groupMask = new KisTransparencyMask(image, "Group mask");
    groupMask->setSelection(selection(4, 255, {21, -17})); image->addNode(groupMask, group);
    KisPaintLayerSP outside = new KisPaintLayer(image, "Outside", 255); image->addNode(outside, image->root());
    outside->paintDevice()->fill(52, 36, 12, 12, color.data());
    image->undoAdapter()->addCommand(new KisSetGlobalSelectionCommand(image, selection(5, 0, {-7, 13})));
    image->refreshGraphAsync(); image->waitForDone();
    const QList<QUuid> identities{paint->uuid(), filterMask->uuid(), adjustment->uuid(), child->uuid(), groupMask->uuid(), outside->uuid()};
    const QUuid targetId = operation == QLatin1String("group") ? group->uuid() :
        operation == QLatin1String("adjustment") ? adjustment->uuid() :
        operation == QLatin1String("filter") ? filterMask->uuid() : child->uuid();
    const auto input = files.filePath("input.kra");
    QVERIFY2(source->saveAs(input, KisDocument::nativeFormatMimeType(), false), qPrintable(source->errorMessage()));
    source->waitForSavingToComplete();
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        const auto fixture = QDir(resultDirectory).filePath(QStringLiteral("mask-crop-input-%1.kra").arg(QString::fromLatin1(QTest::currentDataTag())));
        QFile::remove(fixture);
        QVERIFY(QFile::copy(input, fixture));
    }
    const QRect frame(-96, -96, 256, 256);
    QList<QList<QByteArray>> references;
    QList<QList<QPoint>> positions;
    QList<QSize> sizes;
    QString nativeHistoryLabel;
    for (const bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument()); document->setFileBatchMode(true);
        QVERIFY2(document->openPath(input, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
        image = document->image();
        QCOMPARE(bool(image->gpuImage()), gpu);
        int stage = 0;
        const auto inspect = [&] {
            QList<QByteArray> planes;
            QList<int> channelBytes, channelCounts;
            QList<QPoint> offsets;
            for (const auto &uuid : identities) {
                const auto node = KisLayerUtils::findNodeByUuid(image->root(), uuid); QVERIFY(node);
                const auto device = node->paintDevice(); QVERIFY(device);
                QByteArray bytes(frame.width() * frame.height() * device->pixelSize(), Qt::Uninitialized);
                device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), frame);
                planes.append(bytes); offsets.append(device->offset());
                channelBytes.append(device->pixelSize() == 1 ? 1 : words ? 2 : 1);
                channelCounts.append(device->pixelSize() == 1 ? 1 : 4);
            }
            QByteArray selected(frame.width() * frame.height(), '\0');
            if (image->globalSelection()) image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(selected.data()), frame);
            planes.append(selected); channelBytes.append(1); channelCounts.append(1);
            Document api(document.get(), false);
            planes.append(api.pixelData(0, 0, image->width(), image->height()));
            channelBytes.append(words ? 2 : 1); channelCounts.append(4);
            if (!gpu) { references.append(planes); positions.append(offsets); sizes.append(image->size()); }
            else {
                QCOMPARE(image->size(), sizes[stage]); QCOMPARE(offsets, positions[stage]);
                for (int i = 0; i < planes.size(); ++i) {
                    if (planes[i] != references[stage][i]) {
                        qsizetype byte = 0;
                        while (byte < qMin(planes[i].size(), references[stage][i].size()) && planes[i][byte] == references[stage][i][byte]) ++byte;
                        qWarning() << "Mask crop difference" << operation << "stage" << stage << "plane" << i << "byte" << byte;
                        const auto destination = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                        if (!destination.isEmpty()) {
                            const auto prefix = destination + QStringLiteral("/mask-crop-%1-%2-%3").arg(QString::fromLatin1(QTest::currentDataTag())).arg(stage).arg(i);
                            QFile actual(prefix + "-actual.bin"), expected(prefix + "-native.bin");
                            if (actual.open(QIODevice::WriteOnly)) actual.write(planes[i]);
                            if (expected.open(QIODevice::WriteOnly)) expected.write(references[stage][i]);
                        }
                    }
                    const auto difference = pixelStepDifference(planes[i], references[stage][i], channelBytes[i], channelCounts[i]);
                    QVERIFY2(difference.isEmpty(), qPrintable(difference));
                }
            }
            ++stage;
        };
        const auto crop = [&](QRect rect) {
            Document api(document.get(), false);
            if (operation == QLatin1String("image")) api.crop(rect.x(), rect.y(), rect.width(), rect.height());
            else if (operation == QLatin1String("canvas")) api.resizeImage(rect.x(), rect.y(), rect.width(), rect.height());
            else {
                const auto node = KisLayerUtils::findNodeByUuid(image->root(), targetId); QVERIFY(node);
                if (operation == QLatin1String("group") || operation == QLatin1String("adjustment")) {
                    QScopedPointer<Node> caller(Node::createNode(image, node));
                    caller->cropNode(rect.x(), rect.y(), rect.width(), rect.height());
                } else image->cropNode(node, rect);
                image->waitForDone();
            }
        };
        inspect(); QVERIFY(!QTest::currentTestFailed());
        const int history = document->undoStack()->index();
        const QRect rect(8, 6, 40, 28);
        crop(rect); QVERIFY(!QTest::currentTestFailed());
        QCOMPARE(document->undoStack()->index(), history + 1);
        if (!gpu) nativeHistoryLabel = document->undoStack()->command(history)->text().toString();
        else QCOMPARE(document->undoStack()->command(history)->text().toString(), nativeHistoryLabel);
        const auto *saved = dynamic_cast<const KisCropSavedExtraData *>(document->undoStack()->command(history)->extraData()); QVERIFY(saved);
        QCOMPARE(saved->cropRect(), rect);
        QCOMPARE(saved->cropNode() ? saved->cropNode()->uuid() : QUuid{},
            operation == QLatin1String("image") || operation == QLatin1String("canvas") ? QUuid{} : targetId);
        inspect(); QVERIFY(!QTest::currentTestFailed());
        document->undoStack()->undo(); image->waitForDone(); inspect(); QVERIFY(!QTest::currentTestFailed());
        document->undoStack()->redo(); image->waitForDone(); inspect(); QVERIFY(!QTest::currentTestFailed());
        // Direct writes edit the current cropped version. Version navigation
        // after writes is checked by the mask pixel-write test.
        const auto writeTargetId = operation == QLatin1String("child") ? child->uuid() :
            operation == QLatin1String("adjustment") ? adjustment->uuid() : filterMask->uuid();
        const auto writeTarget = KisLayerUtils::findNodeByUuid(image->root(), writeTargetId); QVERIFY(writeTarget);
        QScopedPointer<Node> writer(Node::createNode(image, writeTarget));
        QVERIFY(writer->setPixelData(QByteArray(1, char(71)), 12, 14, 1, 1));
        QVERIFY(writer->setPixelData(QByteArray(1, char(117)), 145, 30, 1, 1));
        image->refreshGraphAsync(); image->waitForDone();
        QCOMPARE(document->undoStack()->index(), history + 1);
        inspect(); QVERIFY(!QTest::currentTestFailed());
        const auto path = files.filePath(gpu ? "gpu.kra" : "native.kra");
        QVERIFY2(document->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
        document->waitForSavingToComplete();
        const int reopenStage = stage;
        for (const bool readerGpu : {false, true}) {
            if (!gpu && readerGpu) continue;
            qputenv("LIBREPAINT_GPU_ENGINE", readerGpu ? "1" : "0");
            std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
            QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
            document.swap(reopened); image = document->image();
            // Both readers must expose the same saved coordinates, raw masks and merged pixels.
            stage = reopenStage; inspect(); QVERIFY(!QTest::currentTestFailed());
            crop({1, -2, 24, 22}); QVERIFY(!QTest::currentTestFailed());
            inspect(); QVERIFY(!QTest::currentTestFailed());
            const auto edited = files.filePath(QStringLiteral("edited-%1-%2.kra").arg(gpu).arg(readerGpu));
            QVERIFY2(document->saveAs(edited, KisDocument::nativeFormatMimeType(), false), qPrintable(document->errorMessage()));
            document->waitForSavingToComplete();
            document.swap(reopened); image = document->image();
        }
    }
}
