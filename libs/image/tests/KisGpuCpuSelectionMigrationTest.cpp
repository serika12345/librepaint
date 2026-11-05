/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testimage.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_selection.h>
#include <kis_selection_mask.h>
#include <kis_pixel_selection.h>
#include <commands/KisChangeDeselectedMaskCommand.h>
#include <commands/kis_reselect_global_selection_command.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <QSignalSpy>

namespace Gpu = LibrePaint::Image::Gpu;

namespace {
QByteArray coverage(KisSelectionSP selection, QRect area)
{
    QByteArray result(area.width() * area.height(), Qt::Uninitialized);
    selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(result.data()), area);
    return result;
}
}

class KisGpuCpuSelectionMigrationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void deselectedMaskKeepsNativeReselect_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8-deselect-and-reselect") << false;
        QTest::newRow("u16-inverted-gpu-alpha-insertion-and-rejection") << true;
    }
    void deselectedMaskKeepsNativeReselect();
};

void KisGpuCpuSelectionMigrationTest::deselectedMaskKeepsNativeReselect()
{
    QFETCH(bool, boundary);
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    constexpr qsizetype capacity = 8 * 1024 * 1024;
    Gpu::RasterImage raster;
    raster.size = {270, 48};
    raster.format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.colorProfile = space->profile()->rawData();
    Gpu::RasterLayer layer;
    layer.name = QStringLiteral("Retained paint");
    raster.layers = {layer};
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Selection migration"), space);
    const auto root = image->rootLayer();
    const auto paint = root->firstChild();
    const QRect area(-8, -8, 288, 72);
    if (boundary) {
        const QRect softArea(252, 3, 13, 19);
        QByteArray alpha(softArea.width() * softArea.height(), Qt::Uninitialized);
        const quint8 values[] = {0, 1, 63, 127, 128, 193, 254, 255};
        for (qsizetype i = 0; i < alpha.size(); ++i) alpha[i] = char(values[i % 8]);
        const auto literal = Gpu::SelectionMask::fromAlphaCoverage(softArea, alpha, image->bounds());
        const auto generated = owner->translatedSelectionMask(literal, {1, 0}, image->bounds());
        QVERIFY2(generated, qPrintable(owner->lastRequestOutcome().diagnostic));
        QVERIFY(generated && generated->isGpuGenerated());
        QVERIFY(image->setGpuSelectionMask(generated, true));
    } else {
        QVERIFY(image->setGpuSelectionRect({2, 3, 263, 27}));
    }
    const auto remembered = image->globalSelection();
    auto rememberedMask = root->selectionMask();
    const auto pixels = remembered->pixelSelection();
    const auto original = coverage(remembered, area);
    if (boundary) QVERIFY(original.contains(char(127)));
    const auto defaultPixel = pixels->defaultPixel();
    const auto outline = remembered->outlineCache();
    rememberedMask->setName(QStringLiteral("Remembered selection"));
    if (boundary) {
        QVERIFY(pixels->prepareCpuMigration(capacity));
        KisPaintLayerSP inserted = new KisPaintLayer(image, QStringLiteral("Inserted paint"), 137);
        QVERIFY(image->insertGpuLayers({inserted}, root->childCount(), {}, true));
        QCOMPARE(root->lastChild(), KisNodeSP(inserted));
    } else {
        QVERIFY(image->clearGpuSelection());
    }
    QVERIFY(!image->globalSelection());
    QVERIFY(image->canReselectGlobalSelection());
    QCOMPARE(coverage(remembered, area), original);

    if (boundary) {
        QVERIFY(image->setGpuSelectionRect({9, 11, 7, 5}, true));
        const auto operation = owner->beginPixelEdit();
        QVERIFY(!operation.isNull());
        QVERIFY(!image->clearGpuSelection());
        QCOMPARE(owner->cancelPixelEdit(operation), Gpu::Result::Success);
        QVERIFY(image->reselectGpuSelection());
        QCOMPARE(coverage(image->globalSelection(), area), original);
        QVERIFY(image->setGpuSelectionRect({9, 11, 7, 5}, true));
    }
    const auto active = image->globalSelection();
    const auto activeCoverage = active ? coverage(active, area) : QByteArray();
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    QSignalSpy changed(owner.data(), &Gpu::PaintImage::changed);
    QSignalSpy modified(image.data(), &KisImage::sigImageModified);
    image->barrierLock();
    qsizetype bytes = 0;
    auto prepared = image->prepareCpuMigration(capacity, &bytes);
    QVERIFY(prepared);
    // Replacing a CPU-owned remembered mask need not change a GPU version.
    KisChangeDeselectedMaskCommand forget(image, {});
    forget.redo();
    QCOMPARE(owner->operations().snapshot(), before);
    QVERIFY(!image->applyCpuMigration(prepared));
    qsizetype withoutRemembered = 0;
    QVERIFY(image->prepareCpuMigration(capacity, &withoutRemembered));
    forget.undo();
    if (boundary) {
        QVERIFY(bytes > withoutRemembered);
        QVERIFY(!image->prepareCpuMigration(withoutRemembered));
        QCOMPARE(image->gpuImage(), owner);
        QCOMPARE(coverage(remembered, area), original);
    }
    prepared = image->prepareCpuMigration(capacity);
    QVERIFY(prepared);
    QVERIFY(image->applyCpuMigration(prepared));
    image->unlock();
    QVERIFY(!image->gpuImage());
    QVERIFY(!pixels->prepareCpuMigration(capacity));
    QCOMPARE(image->rootLayer(), root);
    QCOMPARE(root->firstChild(), paint);
    QCOMPARE(image->globalSelection(), active);
    if (active) QCOMPARE(coverage(active, area), activeCoverage);
    QVERIFY(image->canReselectGlobalSelection());
    QCOMPARE(coverage(remembered, area), original);
    QCOMPARE(pixels->defaultPixel(), defaultPixel);
    QCOMPARE(remembered->outlineCache(), outline);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(modified.size(), 0);

    KisReselectGlobalSelectionCommand reselect(image);
    reselect.redo();
    QCOMPARE(image->globalSelection(), remembered);
    QCOMPARE(root->selectionMask(), rememberedMask);
    QCOMPARE(rememberedMask->name(), QStringLiteral("Remembered selection"));
    QCOMPARE(remembered->pixelSelection(), pixels);
    QCOMPARE(coverage(image->globalSelection(), area), original);
    QVERIFY(!image->canReselectGlobalSelection());
    reselect.undo();
    QCOMPARE(image->globalSelection(), active);
    if (active) QCOMPARE(coverage(active, area), activeCoverage);
    QVERIFY(image->canReselectGlobalSelection());
    reselect.redo();
    QCOMPARE(image->globalSelection(), remembered);
    QCOMPARE(coverage(image->globalSelection(), area), original);
    image->waitForDone();
    QCOMPARE(owner->operations().snapshot(), before);
    QVERIFY(!owner->operations().isModified());
    QCOMPARE(changed.size(), 0);
}

KISTEST_MAIN(KisGpuCpuSelectionMigrationTest)
#include "KisGpuCpuSelectionMigrationTest.moc"
