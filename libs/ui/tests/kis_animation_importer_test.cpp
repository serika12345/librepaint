/*
 *  SPDX-FileCopyrightText: 2015 Jouni Pentikäinen <joupent@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_animation_importer_test.h"

#include "application/ui/orchestration/KisPart.h"
#include "kis_animation_importer.h"
#include "document/KisDocument.h"
#include <testutil.h>
#include "kis_keyframe_channel.h"
#include "kis_image_animation_interface.h"
#include "kis_group_layer.h"
#include "kis_transaction.h"
#include "kis_config.h"
#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <QImage>
#include <QTemporaryDir>
#include <QPoint>
#include <KoUpdater.h>
#include <KisPaintingEngineTestScope.h>

#include "testui.h"

void KisAnimationImporterTest::testImport()
{
    const TestUtil::PaintingEngineScope gpuDefault(true);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    TestUtil::MaskParent mp(QRect(0,0,512,512));
    document->setCurrentImage(mp.image);

    KisAnimationImporter importer(document->image());

    QStringList files;
    files.append(QString(FILES_DATA_DIR) + '/' + "file_layer_source.png");
    files.append(QString(FILES_DATA_DIR) + '/' + "carrot.png");
    files.append(QString(FILES_DATA_DIR) + '/' + "hakonepa.png");

    QVERIFY(importer.import(files, 7, 3).isOk());
    QVERIFY(!document->image()->gpuImage());

    KisGroupLayerSP root = mp.image->rootLayer();
    KisNodeSP importedLayer = root->lastChild();

    KisKeyframeChannel* contentChannel = importedLayer->getKeyframeChannel(KisKeyframeChannel::Raster.id());

    QVERIFY(contentChannel != 0);
    QCOMPARE(contentChannel->keyframeCount(), 4); // Three imported ones + blank at time 0
    QVERIFY(!contentChannel->keyframeAt(7).isNull());
    QVERIFY(!contentChannel->keyframeAt(10).isNull());
    QVERIFY(!contentChannel->keyframeAt(13).isNull());

    mp.image->animationInterface()->switchCurrentTimeAsync(7);
    mp.image->waitForDone();
    QImage imported1 = importedLayer->projection()->convertToQImage(importedLayer->colorSpace()->profile());

    mp.image->animationInterface()->switchCurrentTimeAsync(10);
    mp.image->waitForDone();
    QImage imported2 = importedLayer->projection()->convertToQImage(importedLayer->colorSpace()->profile());

    mp.image->animationInterface()->switchCurrentTimeAsync(13);
    mp.image->waitForDone();
    QImage imported3 = importedLayer->projection()->convertToQImage(importedLayer->colorSpace()->profile());

    QImage source1(QString(FILES_DATA_DIR) + '/' + "file_layer_source.png");
    QImage source2(QString(FILES_DATA_DIR) + '/' + "carrot.png");
    QImage source3(QString(FILES_DATA_DIR) + '/' + "hakonepa.png");

    QPoint pt;
    QVERIFY(TestUtil::compareQImages(pt, source1, imported1));
    QVERIFY(TestUtil::compareQImages(pt, source2, imported2));
    QVERIFY(TestUtil::compareQImages(pt, source3, imported3));

    KisPart::instance()->removeDocument(document.data(), false);
}

void KisAnimationImporterTest::testFailedImportClosesUndoMacro()
{
    const TestUtil::PaintingEngineScope gpuDefault(true);
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    TestUtil::MaskParent mp(QRect(0, 0, 512, 512));
    document->setCurrentImage(mp.image);
    KisAnimationImporter importer(document->image());
    const auto children = mp.image->root()->childCount();
    QVERIFY(!importer.import({QString(FILES_DATA_DIR) + "/missing-frame.png"}, 7, 3).isOk());
    QCOMPARE(mp.image->root()->childCount(), children);
    QVERIFY(!mp.image->hasOpenStrokes());
    QVERIFY(importer.import({QString(FILES_DATA_DIR) + "/carrot.png"}, 7, 3).isOk());
    mp.image->waitForDone();
    auto *channel = mp.image->root()->lastChild()->getKeyframeChannel(KisKeyframeChannel::Raster.id());
    QVERIFY(channel);
    QVERIFY(!channel->keyframeAt(7).isNull());
    KisPart::instance()->removeDocument(document.data(), false);
}

void KisAnimationImporterTest::testGpuDocumentMigratesForAnimationStartAndImport()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto framePath = directory.filePath(QStringLiteral("frame.png"));
    QImage source(64, 48, QImage::Format_ARGB32);
    source.fill(QColor(41, 137, 211));
    QVERIFY(source.save(framePath));
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();

    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true);
        document->setAutoSaveActive(false);
        QVERIFY(document->newImage(QStringLiteral("Animation start"), 64, 48, cs,
            KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        QVERIFY(document->image()->gpuImage());
        auto node = document->image()->root()->firstChild();
        QVERIFY(node);
        node->enableAnimation();
        QVERIFY(!document->image()->gpuImage());
        QVERIFY(node->isAnimated());
        auto *channel = node->getKeyframeChannel(KisKeyframeChannel::Raster.id(), true);
        QVERIFY(channel);
        QVERIFY(!channel->keyframeAt(0).isNull());
        KisPart::instance()->removeDocument(document.data(), false);
    }

    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY(document->newImage(QStringLiteral("Frame import"), 64, 48, cs,
        KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    QVERIFY(document->image()->gpuImage());
    KisAnimationImporter importer(document.data());
    QVERIFY(importer.import({framePath}, 7, 1).isOk());
    auto image = document->image();
    QVERIFY(!image->gpuImage());
    auto imported = image->root()->lastChild();
    QVERIFY(imported);
    auto *channel = imported->getKeyframeChannel(KisKeyframeChannel::Raster.id());
    QVERIFY(channel);
    QVERIFY(!channel->keyframeAt(7).isNull());
    image->animationInterface()->switchCurrentTimeAsync(7);
    image->waitForDone();
    const QImage actual = imported->projection()->convertToQImage(imported->colorSpace()->profile());
    QPoint mismatch;
    QVERIFY(TestUtil::compareQImages(mismatch, source, actual));

    const QRect edited(4, 4, 12, 10);
    const QColor fill(223, 71, 29);
    image->barrierLock();
    {
        KisTransaction transaction(imported->paintDevice());
        imported->paintDevice()->fill(edited, KoColor(fill, cs));
        transaction.commit(image->undoAdapter());
    }
    imported->setDirty(edited);
    image->unlock();
    image->waitForDone();

    const auto path = directory.filePath(QStringLiteral("animation.kra"));
    QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    document->waitForSavingToComplete();
    {
        QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
        reopened->setFileBatchMode(true);
        QVERIFY(reopened->openPath(path, KisDocument::DontAddToRecent | KisDocument::UseCpuEngine));
        auto reopenedNode = reopened->image()->root()->lastChild();
        QVERIFY(reopenedNode);
        auto *reopenedChannel = reopenedNode->getKeyframeChannel(KisKeyframeChannel::Raster.id());
        QVERIFY(reopenedChannel);
        QVERIFY(!reopenedChannel->keyframeAt(7).isNull());
        reopened->image()->animationInterface()->switchCurrentTimeAsync(7);
        reopened->image()->waitForDone();
        KoColor pixel;
        reopenedNode->paintDevice()->pixel(edited.x() + 1, edited.y() + 1, &pixel);
        QCOMPARE(pixel.toQColor(), fill);
    }
    KisPart::instance()->removeDocument(document.data(), false);
}

void KisAnimationImporterTest::testBusyGpuDocumentKeepsGpuStorageWhenMigrationFails()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto framePath = directory.filePath(QStringLiteral("frame.png"));
    QImage frame(32, 24, QImage::Format_ARGB32);
    frame.fill(QColor(17, 83, 149));
    QVERIFY(frame.save(framePath));
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    QVERIFY(document->newImage(QStringLiteral("Busy migration"), 32, 24, cs,
        KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    auto image = document->image();
    auto owner = image->gpuImage();
    QVERIFY(owner);
    auto node = image->root()->firstChild();
    QVERIFY(node);
    const auto path = directory.filePath(QStringLiteral("busy.kra"));
    QVERIFY(document->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    document->waitForSavingToComplete();
    document->setModified(true);
    QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
    QVERIFY(document->isSaving());
    const int children = image->root()->childCount();

    node->enableAnimation();
    QVERIFY(!node->isAnimated());
    QVERIFY(!node->getKeyframeChannel(KisKeyframeChannel::Raster.id()));
    QCOMPARE(image->gpuImage(), owner);
    QCOMPARE(image->root()->childCount(), children);

    KisAnimationImporter importer(document.data());
    QVERIFY(!importer.import({framePath}, 7, 1).isOk());
    QCOMPARE(image->gpuImage(), owner);
    QCOMPARE(image->root()->childCount(), children);

    document->waitForSavingToComplete();
    node->enableAnimation();
    QVERIFY(!image->gpuImage());
    QVERIFY(node->isAnimated());
    KisPart::instance()->removeDocument(document.data(), false);
}

KISTEST_MAIN(KisAnimationImporterTest)