/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QGuiApplication>
#include <QTest>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>

using namespace LibrePaint::Image::Gpu;

class KisGpuCloneLayersTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void referenceCyclesLeaveTheDocumentAndHistoryUnchanged();
    void sourceEditsKeepOffsetsPrecisionAndPinnedVersions_data();
    void sourceEditsKeepOffsetsPrecisionAndPinnedVersions();
    void sourceProjectionAndOriginalRemainDistinct_data();
    void sourceProjectionAndOriginalRemainDistinct();
    void deletingSourcesKeepsPixelsAndRestoresLiveReferences_data();
    void deletingSourcesKeepsPixelsAndRestoresLiveReferences();
    void groupAndCloneReferencesKeepSourcePlacementAndDefaults();
};

void KisGpuCloneLayersTest::referenceCyclesLeaveTheDocumentAndHistoryUnchanged()
{
    RasterImage input;
    input.size = {64, 64};
    RasterLayer group, paint;
    group.kind = LayerKind::Group;
    group.name = QStringLiteral("Group");
    group.uuid = QUuid::createUuid();
    paint.name = QStringLiteral("Source");
    paint.uuid = QUuid::createUuid();
    paint.parentUuid = group.uuid;
    input.layers = {paint, group};
    OperationDocument document(input);
    RasterLayer clone;
    clone.name = QStringLiteral("Clone");
    clone.kind = LayerKind::Clone;
    clone.cloneSourceUuid = paint.uuid;
    const auto first = document.addCloneLayer(clone, 2);
    QVERIFY(first);
    clone.cloneSourceUuid = document.snapshot()->layers[2].uuid;
    const auto second = document.addCloneLayer(clone, 3);
    QVERIFY(second);
    const auto before = document.snapshot();
    const auto count = document.undoCount();
    QCOMPARE(document.setCloneSource(first, before->layers[3].uuid, false), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.undoCount(), count);
    clone.parentUuid = group.uuid;
    clone.cloneSourceUuid = group.uuid;
    QCOMPARE(document.addCloneLayer(clone, 4), LayerId(0));
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.undoCount(), count);
    const auto edit = document.beginCloneSourceEdit({first});
    QVERIFY(!edit.isNull());
    QCOMPARE(document.updateCloneSourceEdit(edit, {}), Result::Success);
    QCOMPARE(document.commitCloneSourceEdit(edit), Result::Success);
    QCOMPARE(document.preview(), document.snapshot());
    QVERIFY(document.preview()->isCommitted());
    QVERIFY(document.preview()->pendingOperation().isNull());
}

void KisGpuCloneLayersTest::sourceEditsKeepOffsetsPrecisionAndPinnedVersions_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("rgba8") << false;
    QTest::newRow("rgba16") << true;
}

void KisGpuCloneLayersTest::sourceEditsKeepOffsetsPrecisionAndPinnedVersions()
{
    QFETCH(bool, words);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const auto maximum = maximumComponent(format);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {512, 512};
    input.format = format;
    RasterLayer source, clone;
    source.name = QStringLiteral("Source");
    source.uuid = QUuid::createUuid();
    source.visible = false;
    clone.name = QStringLiteral("Clone");
    clone.kind = LayerKind::Clone;
    clone.cloneSourceUuid = source.uuid;
    clone.position = {257, -3};
    input.layers = {source, clone};
    PaintImage image(execution, input);
    const QPoint position(-1, 255), copied = position + clone.position;
    const QRect from(position, QSize(1, 1)), to(copied, QSize(1, 1));
    const auto first = rgbaBytes({1, 17, quint16(maximum - 1), maximum}, format);
    const auto second = rgbaBytes({quint16(maximum - 1), 1, 17, maximum}, format);
    QCOMPARE(image.writeRgbaPixels(1, from, first), Result::Success);
    const auto pinned = image.operations().snapshot();
    const auto read = image.readRgbaPixels(2, to, first.size());
    QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
    QCOMPARE(read.bytes, first);
    QCOMPARE(image.replaceRgbaPixels(1, from, second), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, second.size()).bytes, second);
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, to, 2, first.size()).bytes, first);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, first.size()).bytes, first);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, second.size()).bytes, second);
    const auto root = execution.readNativeProjectionPixels(image.handle(), image.operations().snapshot(), to, second.size());
    QVERIFY2(root.code == ExecutionCode::Success, qPrintable(root.diagnostic));
    QCOMPARE(root.bytes, second);
    const auto beforePreview = image.operations().snapshot();
    const auto edit = image.beginCloneSourceEdit({2});
    QVERIFY(!edit.isNull());
    QCOMPARE(image.updateCloneSourceEdit(edit, {}), Result::Success);
    QCOMPARE(image.cancelStroke(edit), Result::Success);
    QVERIFY(!image.operations().hasActiveStroke());
    QCOMPARE(image.operations().snapshot(), beforePreview);
}

void KisGpuCloneLayersTest::sourceProjectionAndOriginalRemainDistinct_data()
{ sourceEditsKeepOffsetsPrecisionAndPinnedVersions_data(); }

void KisGpuCloneLayersTest::sourceProjectionAndOriginalRemainDistinct()
{
    QFETCH(bool, words);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {32, 32}; input.format = format;
    RasterLayer source, clone;
    source.uuid = QUuid::createUuid(); source.name = QStringLiteral("Source");
    source.visible = false;
    source.transparencyMask = SelectionMask::uniform(0);
    source.defaultPixel = {9, 7, 5, maximumComponent(format)};
    clone.kind = LayerKind::Clone; clone.name = QStringLiteral("Clone");
    clone.cloneSourceUuid = source.uuid;
    clone.position = {-3, 2};
    input.layers = {source, clone};
    PaintImage image(execution, input);
    const QRect from(4, 5, 1, 1), to(1, 7, 1, 1);
    const auto pixel = rgbaBytes({1, 17, 31, maximumComponent(format)}, format);
    QCOMPARE(image.writeRgbaPixels(1, from, pixel), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, pixel.size()).bytes, QByteArray(pixel.size(), '\0'));
    const QRect implicit(-10, 20, 1, 1);
    QCOMPARE(image.readRgbaPixels(2, implicit, pixel.size()).bytes, QByteArray(pixel.size(), '\0'));
    QCOMPARE(image.setCloneSource(2, source.uuid, true), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, pixel.size()).bytes, pixel);
    QCOMPARE(image.readRgbaPixels(2, implicit, pixel.size()).bytes, rgbaBytes(source.defaultPixel, format));
    auto hidden = rgbaBytes({17, 1, 31, 0}, format);
    QCOMPARE(image.replaceRgbaPixels(1, from, hidden), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, hidden.size()).bytes, hidden);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, pixel.size()).bytes, pixel);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(2, to, pixel.size()).bytes, QByteArray(pixel.size(), '\0'));
}

void KisGpuCloneLayersTest::deletingSourcesKeepsPixelsAndRestoresLiveReferences_data()
{ sourceEditsKeepOffsetsPrecisionAndPinnedVersions_data(); }

void KisGpuCloneLayersTest::deletingSourcesKeepsPixelsAndRestoresLiveReferences()
{
    QFETCH(bool, words);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {64, 64}; input.format = format;
    RasterLayer group, source, clone, linked;
    group.uuid = QUuid::createUuid(); group.name = QStringLiteral("Group"); group.kind = LayerKind::Group;
    source.uuid = QUuid::createUuid(); source.name = QStringLiteral("Source"); source.parentUuid = group.uuid;
    clone.uuid = QUuid::createUuid(); clone.name = QStringLiteral("Clone"); clone.kind = LayerKind::Clone;
    clone.cloneSourceUuid = source.uuid; clone.position = {3, 2};
    linked.name = QStringLiteral("Linked"); linked.kind = LayerKind::Clone;
    linked.cloneSourceUuid = clone.uuid; linked.position = {1, 1};
    input.layers = {source, group, clone, linked};
    PaintImage image(execution, input);
    const QRect from(-1, 1, 1, 1), to(2, 3, 1, 1), linkedTo(3, 4, 1, 1);
    const auto pixel = rgbaBytes({1, 17, 31, maximumComponent(format)}, format);
    QCOMPARE(image.writeRgbaPixels(1, from, pixel), Result::Success);
    QCOMPARE(image.readRgbaPixels(3, to, pixel.size()).bytes, pixel);
    const auto pinned = image.operations().snapshot();
    const auto history = image.operations().undoCount();
    QCOMPARE(image.removeLayer(2), Result::Success);
    QCOMPARE(image.operations().undoCount(), history + 1);
    QCOMPARE(image.operations().snapshot()->layers.first().kind, LayerKind::Paint);
    QCOMPARE(image.readRgbaPixels(3, to, pixel.size()).bytes, pixel);
    QCOMPARE(image.readRgbaPixels(4, linkedTo, pixel.size()).bytes, pixel);
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, to, 3, pixel.size()).bytes, pixel);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.operations().snapshot()->layers[2].kind, LayerKind::Clone);
    const auto changed = rgbaBytes({31, 1, 17, maximumComponent(format)}, format);
    QCOMPARE(image.replaceRgbaPixels(1, from, changed), Result::Success);
    QCOMPARE(image.readRgbaPixels(3, to, pixel.size()).bytes, changed);
    QCOMPARE(image.redo(), Result::NothingToRedo);
    QCOMPARE(image.removeLayer(2), Result::Success);
    QCOMPARE(image.redo(), Result::NothingToRedo);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(3, to, pixel.size()).bytes, changed);
}

void KisGpuCloneLayersTest::groupAndCloneReferencesKeepSourcePlacementAndDefaults()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {32, 32};
    RasterLayer source, group, innerClone, outerClone;
    source.uuid = QUuid::createUuid(); source.name = QStringLiteral("Source");
    source.defaultPixel = {1, 17, 31, 255}; source.visible = false;
    group.uuid = QUuid::createUuid(); group.name = QStringLiteral("Group");
    group.kind = LayerKind::Group; group.visible = false;
    innerClone.uuid = QUuid::createUuid(); innerClone.name = QStringLiteral("Inner");
    innerClone.kind = LayerKind::Clone; innerClone.cloneSourceUuid = source.uuid;
    innerClone.parentUuid = group.uuid;
    outerClone.name = QStringLiteral("Outer"); outerClone.kind = LayerKind::Clone;
    outerClone.cloneSourceUuid = group.uuid; outerClone.position = {3, 2};
    input.layers = {source, innerClone, group, outerClone};
    PaintImage image(execution, input);
    const QRect region(10, 10, 1, 1);
    const auto pixel = rgbaBytes(source.defaultPixel, input.format);
    QCOMPARE(image.readRgbaPixels(4, region, pixel.size()).bytes, pixel);
    QCOMPARE(image.removeLayer(3), Result::Success);
    QCOMPARE(image.readRgbaPixels(4, region, pixel.size()).bytes, pixel);
}

QTEST_MAIN(KisGpuCloneLayersTest)
#include "KisGpuCloneLayersTest.moc"
