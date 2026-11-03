/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_group_layer_test.h"
#include <simpletest.h>

#include <testutil.h>

#include <KoColor.h>

#include "kis_group_layer.h"
#include "kis_paint_layer.h"
#include "kis_types.h"
#include "KoColorSpaceRegistry.h"
#include "kis_image.h"

#include "kis_undo_stores.h"
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <memory>
#include <stdexcept>


void KisGroupLayerTest::testProjection()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, colorSpace, "merge test");

}

void KisGroupLayerTest::testPassThroughChildrenComposeIntoTheirParent_data()
{
    QTest::addColumn<int>("children");
    QTest::addColumn<bool>("nestedPassThrough");
    QTest::addColumn<bool>("isolatedParent");
    QTest::addColumn<bool>("visible");
    QTest::newRow("one-child") << 1 << false << false << true;
    QTest::newRow("two-children") << 2 << false << false << true;
    QTest::newRow("nested-pass-through") << 2 << true << false << true;
    QTest::newRow("inside-isolated-group") << 2 << false << true << true;
    QTest::newRow("hidden-pass-through") << 2 << false << false << false;
}

void KisGroupLayerTest::testPassThroughChildrenComposeIntoTheirParent()
{
    QFETCH(int, children);
    QFETCH(bool, nestedPassThrough);
    QFETCH(bool, isolatedParent);
    QFETCH(bool, visible);
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    const QRect bounds(0, 0, 4, 4);
    KisImageSP grouped = new KisImage(nullptr, 4, 4, cs, "Pass-through group");
    KisImageSP flat = new KisImage(nullptr, 4, 4, cs, "Equivalent layer stack");
    KisNodeSP parent = grouped->root();
    if (isolatedParent) {
        KisGroupLayerSP isolated = new KisGroupLayer(grouped, "Isolated parent", 255);
        QVERIFY(grouped->addNode(isolated, parent));
        parent = isolated;
    }
    KisGroupLayerSP outer = new KisGroupLayer(grouped, "Pass-through", 255);
    outer->setPassThroughMode(true);
    outer->setVisible(visible);
    QVERIFY(grouped->addNode(outer, parent));
    parent = outer;
    if (nestedPassThrough) {
        KisGroupLayerSP inner = new KisGroupLayer(grouped, "Nested pass-through", 171);
        inner->setPassThroughMode(true);
        QVERIFY(grouped->addNode(inner, parent));
        parent = inner;
    }
    const QColor colors[] = {QColor(201, 41, 93, 137), QColor(17, 211, 77, 189)};
    const quint8 opacities[] = {153, 211};
    for (int index = 0; index < children; ++index) {
        KisPaintLayerSP layer = new KisPaintLayer(grouped, "Grouped child", opacities[index]);
        layer->paintDevice()->fill(bounds, KoColor(colors[index], cs));
        QVERIFY(grouped->addNode(layer, parent));
        const quint8 effectiveOpacity = nestedPassThrough ? int(opacities[index]) * 171 / 255 : opacities[index];
        KisPaintLayerSP reference = new KisPaintLayer(flat, "Flat child", effectiveOpacity);
        reference->setVisible(visible);
        reference->paintDevice()->fill(bounds, KoColor(colors[index], cs));
        QVERIFY(flat->addNode(reference, flat->root()));
    }
    grouped->initialRefreshGraph();
    flat->initialRefreshGraph();
    QByteArray actual(bounds.width() * bounds.height() * 4, Qt::Uninitialized);
    QByteArray expected(actual.size(), Qt::Uninitialized);
    grouped->projection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), bounds);
    flat->projection()->readBytes(reinterpret_cast<quint8 *>(expected.data()), bounds);
    if (visible) QVERIFY(expected != QByteArray(expected.size(), '\0'));
    QCOMPARE(actual, expected);
}

#include "commands/kis_image_layer_remove_command.h"

void KisGroupLayerTest::testRemoveAndUndo()
{
    KisSurrogateUndoStore *undoStore = new KisSurrogateUndoStore();

//    QRect transpRect(50,50,300,300);
//    QRect blurRect(66,66,300,300);
//    QPoint blurShift(34,34);
//    QPoint cloneShift(75,75);

    QRect imageRect = QRect(0, 0, 64, 64);
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(undoStore, imageRect.width(), imageRect.height(), cs, "merge test");

    QRect rect1(10, 10, 30, 30);
    QRect rect2(30, 30, 30, 30);

    KisPaintLayerSP paintLayer0 = new KisPaintLayer(image, "paint0", OPACITY_OPAQUE_U8);
    paintLayer0->paintDevice()->fill(imageRect, KoColor(Qt::white, cs));

    KisPaintLayerSP paintLayer1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8);
    paintLayer1->paintDevice()->fill(rect1, KoColor(Qt::red, cs));

    KisPaintLayerSP paintLayer2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8);
    paintLayer2->paintDevice()->fill(rect2, KoColor(Qt::blue, cs));


    KisGroupLayerSP groupLayer1 = new KisGroupLayer(image, "group1", OPACITY_OPAQUE_U8);

    image->addNode(paintLayer0, image->root());
    image->addNode(groupLayer1, image->root());
    image->addNode(paintLayer1, groupLayer1);
    image->addNode(paintLayer2, groupLayer1);

    image->initialRefreshGraph();
    QVERIFY(TestUtil::checkQImage(image->projection()->convertToQImage(0, imageRect),
                                  "group_layer_test",
                                  "undo_removal",
                                  "0_initial"));

    // touch original to cause clearing of the projection
    groupLayer1->original();

    image->undoAdapter()->addCommand(new KisImageLayerRemoveCommand(image, groupLayer1));
    image->waitForDone();

    // touch original again
    groupLayer1->original();

    QVERIFY(TestUtil::checkQImage(image->projection()->convertToQImage(0, imageRect),
                                  "group_layer_test",
                                  "undo_removal",
                                  "1_deleted"));

    undoStore->undo();
    image->waitForDone();

    // hey, man! don't sleep!
    groupLayer1->original();

    QVERIFY(TestUtil::checkQImage(image->projection()->convertToQImage(0, imageRect),
                                  "group_layer_test",
                                  "undo_removal",
                                  "2_undone"));
}

void KisGroupLayerTest::testGpuGroupViewsReuseTheOnlyDocumentAttachment()
try {
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device, 128 * 1024 * 1024, 1);
    Gpu::RasterLayer leaf{QStringLiteral("Leaf"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}};
    Gpu::RasterLayer inner{QStringLiteral("Inner"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}};
    Gpu::RasterLayer outer{QStringLiteral("Outer"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}};
    inner.kind = outer.kind = Gpu::LayerKind::Group;
    inner.opacity = 128;
    outer.opacity = 64;
    leaf.parentUuid = inner.uuid;
    inner.parentUuid = outer.uuid;
    QByteArray tile(Gpu::RgbaTile::ByteCount, '\0');
    tile.replace((3 * 256 + 3) * 4, 4, QByteArray::fromHex("00ff00ff"));
    leaf.tiles = {{{0, 0}, tile}};
    Gpu::RasterImage raster;
    raster.size = {8, 8};
    raster.layers = {leaf, inner, outer};
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    Gpu::OperationDocument probe({8, 8});
    QCOMPARE(execution->open(probe.snapshot()).code, Gpu::ExecutionCode::CapacityExceeded);
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Attached group views"));
    const auto outerView = qobject_cast<const KisGroupLayer *>(image->root()->firstChild().data());
    QVERIFY(outerView);
    const auto innerView = qobject_cast<const KisGroupLayer *>(outerView->firstChild().data());
    QVERIFY(innerView);
    const auto pixel = [](KisPaintDeviceSP pixels) {
        QByteArray bytes(4, '\0');
        pixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 3, 3, 1, 1);
        return bytes;
    };
    QCOMPARE(pixel(innerView->original()), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(pixel(outerView->original()), QByteArray::fromHex("00ff0080"));
    QCOMPARE(pixel(image->projection()), QByteArray::fromHex("00ff0020"));
    QCOMPARE(owner->replaceRgbaPixels(1, {3, 3, 1, 1}, QByteArray::fromHex("ff0000ff")), Gpu::Result::Success);
    QCOMPARE(image->root()->firstChild().data(), outerView);
    QCOMPARE(outerView->firstChild().data(), innerView);
    QCOMPARE(pixel(innerView->original()), QByteArray::fromHex("0000ffff"));
    QCOMPARE(pixel(outerView->original()), QByteArray::fromHex("0000ff80"));
    QCOMPARE(pixel(image->projection()), QByteArray::fromHex("0000ff20"));
}
catch (const std::exception &error)
{
    QFAIL(error.what());
}

SIMPLE_TEST_MAIN(KisGroupLayerTest)


