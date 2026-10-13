/* SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "TestNode.h"
#include <simpletest.h>
#include <QColor>
#include <QDataStream>
#include <kundo2stack.h>

#include <KritaVersionWrapper.h>
#include <Node.h>
#include <Document.h>
#include <Krita.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>

#include <kis_image.h>
#include <kis_fill_painter.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_shape_layer.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>

#include <testui.h>

void TestNode::testSetColorSpace()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QStringList profiles = Krita().profiles("GRAYA", "U16");
    node->setColorSpace("GRAYA", "U16", profiles.first());
    QCOMPARE(layer->colorSpace()->colorModelId().id() , "GRAYA");
    QCOMPARE(layer->colorSpace()->colorDepthId().id() , "U16");
    QCOMPARE(layer->colorSpace()->profile()->name(), "Gray-D50-elle-V2-g10.icc");
}

void TestNode::testSetColorProfile()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QStringList profiles = Krita().profiles("RGBA", "U8");
    Q_FOREACH(const QString &profile, profiles) {
        node->setColorProfile(profile);
        QVERIFY(layer->colorSpace()->profile()->name() == profile);
    }
}

void TestNode::testPixelData()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QByteArray ba = node->pixelData(0, 0, 100, 100);
    QDataStream ds(ba);
    do {
        quint8 channelvalue;
        ds >> channelvalue;
        QVERIFY(channelvalue == 0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 255);
        ds >> channelvalue;
        QVERIFY(channelvalue == 255);
    } while (!ds.atEnd());

    QDataStream ds2(&ba, QIODevice::WriteOnly);
    for (int i = 0; i < 100 * 100; i++) {
        ds2 << 255;
        ds2 << 255;
        ds2 << 255;
        ds2 << 255;
    }

    node->setPixelData(ba, 0, 0, 100, 100);
    for (int i = 0; i < 100 ; i++) {
        for (int j = 0; j < 100 ; j++) {
            QColor pixel;
            layer->paintDevice()->pixel(i, j, &pixel);
            QVERIFY(pixel == QColor(Qt::black));
        }
    }
}

void TestNode::testProjectionPixelData()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QByteArray ba = node->projectionPixelData(0, 0, 100, 100);
    QDataStream ds(ba);
    for (int i = 0; i < 100 * 100; i++) {
        quint8 channelvalue;
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xA4);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xA0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xA0);
        ds >> channelvalue;
        QVERIFY(channelvalue == 0xFF);
    }
}

void TestNode::testRawPixelWritesPreserveFormatHistoryAndFailures()
{
    QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(document->createUndoStore(), 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "Pixels", 255);
    image->addNode(layer);
    document->setCurrentImage(image);
    image->waitForDone();
    NodeSP node(Node::createNode(image, layer));
    const auto raw = QByteArray::fromHex("120a2700197fafe00000ffffff000080");
    const auto count = document->undoStack()->count();
    const auto index = document->undoStack()->index();
    const auto modified = document->isModified();
    QVERIFY(node->setPixelData(raw + QByteArrayLiteral("ignored suffix"), -1, -1, 2, 2));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QCOMPARE(node->pixelData(20, 20, 2, 2), QByteArray(16, '\0'));
    QVERIFY(!node->setPixelData(raw.first(15), -1, -1, 2, 2));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QVERIFY(node->setPixelData({}, 0, 0, 0, 0));
    QCOMPARE(node->pixelData(-1, -1, 2, 2), raw);
    QCOMPARE(document->undoStack()->count(), count);
    QCOMPARE(document->undoStack()->index(), index);
    QCOMPARE(document->isModified(), modified);
    NodeSP group(Node::createNode(image, image->rootLayer()));
    QVERIFY(!group->setPixelData(raw, 0, 0, 2, 2));
    KisPart::instance()->removeDocument(document.data(), false);
}

void TestNode::testPixelCopiesAndNodesOutliveTheImageOwner()
{
    KisImageSP image = new KisImage(nullptr, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "Pixels", 255);
    image->addNode(layer);
    NodeSP node(Node::createNode(image, layer));
    const auto before = QByteArray::fromHex("197fafe0");
    const auto after = QByteArray::fromHex("120a2700");
    QVERIFY(node->setPixelData(before, 8, 12, 1, 1));
    const auto retained = node->pixelData(8, 12, 1, 1);
    QVERIFY(node->setPixelData(after, 8, 12, 1, 1));
    QCOMPARE(retained, before);
    QCOMPARE(node->pixelData(8, 12, 1, 1), after);
    image->waitForDone();
    layer.clear();
    image.clear();
    QCOMPARE(node->pixelData(8, 12, 1, 1), after);
    QCOMPARE(retained, before);
}

void TestNode::testLayerAndDocumentProjectionsRemainDistinct()
{
    QScopedPointer<KisDocument> owner(KisPart::instance()->createDocument());
    KisImageSP image = new KisImage(owner->createUndoStore(), 64, 64, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP lower = new KisPaintLayer(image, "Lower", 255);
    KisNodeSP upper = new KisPaintLayer(image, "Upper", 255);
    image->addNode(lower);
    image->addNode(upper);
    owner->setCurrentImage(image);
    NodeSP lowerNode(Node::createNode(image, lower));
    NodeSP upperNode(Node::createNode(image, upper));
    const auto red = QByteArray::fromHex("0000ffff").repeated(4);
    const auto blue = QByteArray::fromHex("120a2700ff0000ffff0000ffff0000ff");
    QVERIFY(lowerNode->setPixelData(red, 8, 12, 2, 2));
    QVERIFY(upperNode->setPixelData(blue, 8, 12, 2, 2));
    upperNode->setVisible(false);
    Document document(owner.data(), false);
    document.refreshProjection();
    QCOMPARE(document.pixelData(8, 12, 2, 2), red);
    QCOMPARE(upperNode->pixelData(8, 12, 2, 2), blue);
    QCOMPARE(upperNode->projectionPixelData(8, 12, 2, 2), blue);
    upperNode->setVisible(true);
    document.refreshProjection();
    QCOMPARE(document.pixelData(8, 12, 2, 2), red.first(4) + blue.mid(4));
    QCOMPARE(lowerNode->pixelData(8, 12, 2, 2), red);
    QCOMPARE(upperNode->pixelData(8, 12, 2, 2), blue);
    KisPart::instance()->removeDocument(owner.data(), false);
}

void TestNode::testThumbnail()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QImage thumb = node->thumbnail(10, 10);
    thumb.save("thumb.png");
    QVERIFY(thumb.width() == 10);
    QVERIFY(thumb.height() == 10);
    // Our thumbnail calculator in KisPaintDevice cannot make a filled 10x10 thumbnail from a 100x100 device,
    // it makes it 10x10 empty, then puts 8x8 pixels in there... Not a bug in the Node class
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            QVERIFY(thumb.pixelColor(i, j) == QColor(Qt::gray));
        }
    }
}

void TestNode::testMergeDown()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");

    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    {
        KisFillPainter gc(layer->paintDevice());
        gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer->colorSpace()));
    }
    image->addNode(layer);

    KisNodeSP layer2 = new KisPaintLayer(image, "test2", 255);
    {
        KisFillPainter gc(layer2->paintDevice());
        gc.fillRect(0, 0, 100, 100, KoColor(Qt::gray, layer2->colorSpace()));
    }
    image->addNode(layer2);
    NodeSP n1 = NodeSP(Node::createNode(image, layer));
    Node *n2 = n1->mergeDown();
    delete n2;
}

void TestNode::testFindChildNodes()
{
    QScopedPointer<KisDocument> kisdoc(KisPart::instance()->createDocument());

    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP gLayer1 = new KisGroupLayer(image, "gLayer1", OPACITY_OPAQUE_U8);
    KisNodeSP pLayer1 = new KisPaintLayer(image, "pLayer1", 255);
    KisNodeSP pLayer2 = new KisPaintLayer(image, "pLayer2", 255);
    KisNodeSP vLayer1 = new KisShapeLayer(kisdoc->shapeController(), image, "vLayer1", OPACITY_OPAQUE_U8);
    KisNodeSP vLayer2 = new KisShapeLayer(kisdoc->shapeController(), image, "vLayer2", OPACITY_OPAQUE_U8);

    gLayer1->setColorLabelIndex(2);

    NodeSP rootNode = NodeSP(Node::createNode(image, image->rootLayer()));

    image->addNode(pLayer1);
    image->addNode(vLayer1);
    image->addNode(pLayer2, gLayer1, gLayer1->childCount());
    image->addNode(vLayer2, gLayer1, gLayer1->childCount());
    image->addNode(gLayer1);

    kisdoc->setCurrentImage(image);

    {
        QStringList test = {"pLayer1", "vLayer1", "gLayer1"};
        QList<Node *> nodeList = rootNode->findChildNodes();

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"pLayer1", "vLayer1", "gLayer1", "vLayer2", "pLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("", true);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"vLayer1"};
        QList<Node *> nodeList = rootNode->findChildNodes("vLayer1");

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"vLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("vLayer2", true);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"vLayer1", "vLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("vLayer", true, true);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"pLayer1", "pLayer2"};
        QList<Node *> nodeList = rootNode->findChildNodes("", true, false, "paintlayer");

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

    {
        QStringList test = {"gLayer1"};
        QList<Node *> nodeList = rootNode->findChildNodes("", true, false, "", 2);

        QVERIFY(test.size() == nodeList.size());
        for (Node* n : nodeList) {
            QVERIFY(test.contains(n->name()) == true);
            delete n;
        }
    }

}


KISTEST_MAIN(TestNode)

