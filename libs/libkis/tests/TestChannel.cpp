/* SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "TestChannel.h"
#include <simpletest.h>
#include <QColor>
#include <QDataStream>
#include <QLoggingCategory>
#include <QDir>
#include <QFile>
#include <QScopeGuard>

#include <KritaVersionWrapper.h>
#include <Node.h>
#include <Channel.h>
#include <Krita.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>

#include <kis_fill_painter.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <testui.h>

void TestChannel::testProjectionChannelVisibility_data()
{
    QTest::addColumn<int>("mask");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<bool>("group");
    for (const bool group : {false, true}) for (const int opacity : {255, 128}) {
        for (int mask = 0; mask < 16; ++mask) {
            const QByteArray name = QByteArray(group ? "group-" : "paint-") + QByteArray::number(mask, 16) +
                '-' + QByteArray::number(opacity);
            QTest::newRow(name.constData()) << mask << opacity << group;
        }
    }
}

void TestChannel::testProjectionChannelVisibility()
{
    // The public channel visibility and inherit-alpha setters control a layer's
    // contribution to its parent, while retaining its editable pixel values.
    QFETCH(int, mask);
    QFETCH(int, opacity);
    QFETCH(bool, group);
    namespace Gpu = LibrePaint::Image::Gpu;
    const QSize size(256, 256);
    QByteArray background(size.width() * size.height() * 4, '\0');
    QByteArray foreground(background.size(), '\0');
    for (int destinationAlpha = 0; destinationAlpha < 256; ++destinationAlpha) {
        for (int sourceAlpha = 0; sourceAlpha < 256; ++sourceAlpha) {
            const auto offset = (destinationAlpha * 256 + sourceAlpha) * 4;
            background[offset] = '\x33'; background[offset + 1] = '\x66'; background[offset + 2] = '\x99';
            background[offset + 3] = char(destinationAlpha);
            foreground[offset] = '\x12'; foreground[offset + 1] = '\x34'; foreground[offset + 2] = '\x56';
            foreground[offset + 3] = char(sourceAlpha);
        }
    }
    QByteArray ordinary;
    for (const bool gpu : {false, true}) {
        QString diagnostic;
        const auto device = gpu ? Gpu::Device::createNative(diagnostic) : Gpu::DevicePtr{};
        QVERIFY2(!gpu || device, qPrintable(diagnostic));
        std::unique_ptr<Gpu::Execution> execution;
        Gpu::PaintImageSP owner;
        KisImageSP image;
        if (gpu) {
            execution = std::make_unique<Gpu::Execution>(device);
            Gpu::RasterImage input;
            input.size = size;
            input.layers = {{QStringLiteral("Base"), true, QUuid::createUuid(), {}},
                            {QStringLiteral("Top"), true, QUuid::createUuid(), {}}};
            if (group) {
                input.layers.last().kind = Gpu::LayerKind::Group;
                Gpu::RasterLayer child{QStringLiteral("Child"), true, QUuid::createUuid(), {}};
                child.parentUuid = input.layers.last().uuid;
                input.layers.append(child);
            }
            owner = Gpu::PaintImageSP::create(*execution, input);
            image = KisImage::fromGpuImage(owner, QStringLiteral("Projection channel visibility"));
        } else {
            image = new KisImage(nullptr, size.width(), size.height(), KoColorSpaceRegistry::instance()->rgb8(), "Projection channel visibility");
            image->addNode(new KisPaintLayer(image, QStringLiteral("Base"), 255));
            if (group) {
                KisNodeSP parent = new KisGroupLayer(image, QStringLiteral("Top"), 255);
                image->addNode(parent);
                image->addNode(new KisPaintLayer(image, QStringLiteral("Child"), 255), parent);
            } else {
                image->addNode(new KisPaintLayer(image, QStringLiteral("Top"), 255));
            }
        }
        NodeSP base(Node::createNode(image, image->root()->firstChild()));
        const auto targetLayer = image->root()->lastChild();
        NodeSP target(Node::createNode(image, targetLayer));
        NodeSP paint(Node::createNode(image, group ? targetLayer->firstChild() : targetLayer));
        QVERIFY(base->setPixelData(background, 0, 0, size.width(), size.height()));
        QVERIFY(paint->setPixelData(foreground, 0, 0, size.width(), size.height()));
        target->setOpacity(opacity);
        const auto channels = target->channels();
        const auto deleteChannels = qScopeGuard([&] { qDeleteAll(channels); });
        QCOMPARE(channels.size(), 4);
        if (mask == 7) {
            target->setInheritAlpha(true);
            QVERIFY(target->inheritAlpha());
        } else {
            for (auto *channel : channels) channel->setVisible(mask & (1 << channel->position()));
        }
        for (auto *channel : channels) QCOMPARE(channel->visible(), bool(mask & (1 << channel->position())));
        image->refreshGraphAsync();
        image->waitForDone();
        QByteArray projection(background.size(), '\0');
        image->projection()->readBytes(reinterpret_cast<quint8*>(projection.data()), 0, 0, size.width(), size.height());
        QCOMPARE(paint->pixelData(0, 0, size.width(), size.height()), foreground);
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!directory.isEmpty()) {
            QVERIFY(QDir().mkpath(directory));
            QFile artifact(QDir(directory).filePath(QStringLiteral("projection-channels-%1-%2.bgra")
                .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("cpu"))));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(projection), projection.size());
        }
        if (!gpu) ordinary = projection;
        else {
            QCOMPARE(owner->operations().undoCount(), 0);
            QCOMPARE(projection, ordinary);
        }
    }
}

void TestChannel::testPixelDataU8()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb8(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QList<Channel*> channels = node->channels();
    Q_FOREACH(Channel *channel, channels) {
        QVERIFY(channel->channelSize() == 1);
    }

}

void TestChannel::testPixelDataU16()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->rgb16(), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QList<Channel*> channels = node->channels();
    Q_FOREACH(Channel *channel, channels) {
        QVERIFY(channel->channelSize() == 2);
    }
}

void TestChannel::testPixelDataF16()
{
#ifdef HAVE_OPENEXR
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->colorSpace("RGBA", "F16", "");
    KisImageSP image = new KisImage(0, 100, 100, cs, "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QList<Channel*> channels = node->channels();
    Q_FOREACH(Channel *channel, channels) {
        qDebug() << "channelsize" << channel->channelSize();
        QVERIFY(channel->channelSize() == 2);
    }
#endif
}

void TestChannel::testPixelDataF32()
{
    KisImageSP image = new KisImage(0, 100, 100, KoColorSpaceRegistry::instance()->colorSpace("RGBA", "F32", ""), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 100, 100, KoColor(Qt::red, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QList<Channel*> channels = node->channels();
    Q_FOREACH(Channel *channel, channels) {
        QVERIFY(channel->channelSize() == 4);
    }
}

void TestChannel::testReadWritePixelData()
{
    KisImageSP image = new KisImage(0, 2, 2, KoColorSpaceRegistry::instance()->colorSpace("RGBA", "U8", ""), "test");
    KisNodeSP layer = new KisPaintLayer(image, "test1", 255);
    KisFillPainter gc(layer->paintDevice());
    gc.fillRect(0, 0, 2, 2, KoColor(Qt::yellow, layer->colorSpace()));
    NodeSP node = NodeSP(Node::createNode(image, layer));
    QList<Channel*> channels = node->channels();
    Channel *greenChan = channels[1];
    QVERIFY(greenChan->name() == "Green");
    QRect rc = greenChan->bounds();
    QVERIFY(rc == QRect(0, 0, 2, 2));
    QByteArray ba = greenChan->pixelData(rc);
    ba.fill('\x80', 4);
    greenChan->setPixelData(ba, rc);
    image->refreshGraphAsync();
    image->waitForDone();
    QColor c;
    layer->paintDevice()->pixel(0, 0, &c);
    QVERIFY(c == QColor(255, 128, 0));

}


KISTEST_MAIN(TestChannel)

