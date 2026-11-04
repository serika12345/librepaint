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

KISTEST_MAIN(KisAnimationImporterTest)
