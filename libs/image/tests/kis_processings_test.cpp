/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_processings_test.h"

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <simpletest.h>

#include "kis_paint_layer.h"
#include "kis_group_layer.h"
#include "kis_undo_stores.h"
#include "kis_processing_applicator.h"
#include "processing/kis_crop_processing_visitor.h"
#include "processing/kis_mirror_processing_visitor.h"

#include "kistest.h"

#define USE_DOCUMENT 0
#include "qimage_based_test.h"

#include "kis_filter_strategy.h"
#include "kis_transform_worker.h"
#include "processing/kis_transform_processing_visitor.h"


class BaseProcessingTest : public TestUtil::QImageBasedTest
{
public:
    BaseProcessingTest()
        : QImageBasedTest("processings")
    {
    }

    void test(const QString &testname, KisProcessingVisitorSP visitor) {
        KisSurrogateUndoStore *undoStore = new KisSurrogateUndoStore();
        KisImageSP image = createImage(undoStore);
        image->initialRefreshGraph();

        QVERIFY(checkLayersInitial(image));

        KisProcessingApplicator applicator(image, image->root(),
                                           KisProcessingApplicator::RECURSIVE);

        applicator.applyVisitor(visitor);
        applicator.end();
        image->waitForDone();

        /**
         * NOTE: after a change in KisLayer::changeRect(), which now
         * crops change rect for layers with COMPOSITE_COPY
         * composition, the clone layer will have some ghost pixels
         * outside main projection rect. That is ok, because these
         * pixels will never be painted due to a Filter Layer above,
         * which crops the change rect.
         */
        QVERIFY(checkLayers(image, testname));

        undoStore->undo();
        image->waitForDone();

        if (!checkLayersInitial(image)) {
            warnKrita << "NOTE: undo is not completely identical to the original image. Falling back to projection comparison";
            QVERIFY(checkLayersInitialRootOnly(image));
        }
    }
};

void KisProcessingsTest::testCropVisitor()
{
    KisProcessingVisitorSP visitor =
        new KisCropProcessingVisitor(QRect(45,45,410,410), true, true);

    BaseProcessingTest tester;
    tester.test("crop", visitor);
}

void KisProcessingsTest::testTransformVisitorScale()
{
    BaseProcessingTest tester;

    KisFilterStrategy * filter = new KisBoxFilterStrategy();

    KisProcessingVisitorSP visitor =
        new KisTransformProcessingVisitor(0.5, 0.5,
                                          0,0,
                                          0,
                                          0,0,
                                          filter);

    tester.test("transform_scale", visitor);
}

void KisProcessingsTest::testTransformVisitorScaleRotate()
{
    BaseProcessingTest tester;

    KisFilterStrategy * filter = new KisBoxFilterStrategy();

    KisProcessingVisitorSP visitor =
        new KisTransformProcessingVisitor(0.5, 0.5,
                                          0,0,
                                          M_PI,
                                          320,220.5,
                                          filter);

    tester.test("transform_scale_rotate", visitor);
}

void KisProcessingsTest::testMirrorNodesOperation()
{
    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisSurrogateUndoStore *undoStore = new KisSurrogateUndoStore();
    KisImageSP image = new KisImage(undoStore, 4, 1, colorSpace, "mirror test");
    KisPaintLayerSP layer = new KisPaintLayer(image, "paint", OPACITY_OPAQUE_U8);

    layer->paintDevice()->fill(QRect(0, 0, 1, 1), KoColor(Qt::red, colorSpace));
    layer->paintDevice()->fill(QRect(3, 0, 1, 1), KoColor(Qt::blue, colorSpace));
    image->addNode(layer, image->rootLayer());
    image->initialRefreshGraph();
    image->waitForDone();

    KisMirrorProcessingVisitor::applyToNodes(
        image,
        KisNodeList {layer},
        Qt::Horizontal,
        KisSelectionSP(),
        kundo2_noi18n("Mirror Nodes"));
    image->waitForDone();

    QImage mirrored = layer->paintDevice()->convertToQImage(0, 0, 0, 4, 1);
    QCOMPARE(mirrored.pixelColor(0, 0), QColor(Qt::blue));
    QCOMPARE(mirrored.pixelColor(3, 0), QColor(Qt::red));

    undoStore->undo();
    image->waitForDone();

    QImage restored = layer->paintDevice()->convertToQImage(0, 0, 0, 4, 1);
    QCOMPARE(restored.pixelColor(0, 0), QColor(Qt::red));
    QCOMPARE(restored.pixelColor(3, 0), QColor(Qt::blue));
}

KISTEST_MAIN(KisProcessingsTest)
