/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "xcftools.h"

#include <QTest>

#include <cstddef>
#include <cstdint>
#include <type_traits>

class XcfToolsValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valueInitializationRemainsZeroed();
    void signedAndDimensionValuesRemainIndependent();
    void borrowedPointerValuesRemainShallowOnCopy();
    void copiesSharePointersAndKeepScalarStateIndependent();
    void xcfPrimitiveAndGlobalSchemaRemainsStable();
    void xcfFatalDiagnosticSignaturesRemainStable();
    void xcfFileLifecycleSignaturesRemainStable();
    void xcfAllocationAndSupportSignaturesRemainStable();
    void xcfParsingSignaturesRemainStable();
};

void XcfToolsValueContractTest::valueInitializationRemainsZeroed()
{
    const rect bounds {};
    QCOMPARE(bounds.t, 0);
    QCOMPARE(bounds.b, 0);
    QCOMPARE(bounds.l, 0);
    QCOMPARE(bounds.r, 0);

    const tileDimensions dimensions {};
    QCOMPARE(dimensions.c.t, 0);
    QCOMPARE(dimensions.c.b, 0);
    QCOMPARE(dimensions.c.l, 0);
    QCOMPARE(dimensions.c.r, 0);
    QCOMPARE(dimensions.width, 0U);
    QCOMPARE(dimensions.height, 0U);
    QCOMPARE(dimensions.tilesx, 0U);
    QCOMPARE(dimensions.tilesy, 0U);
    QCOMPARE(dimensions.ntiles, 0U);

    const xcfTiles tiles {};
    QVERIFY(tiles.params == nullptr);
    QVERIFY(tiles.tileptrs == nullptr);
    QCOMPARE(tiles.hierarchy, uint32_t(0));

    const xcfLayer layer {};
    QCOMPARE(layer.dim.width, 0U);
    QVERIFY(layer.name == nullptr);
    QCOMPARE(static_cast<int>(layer.mode), 0);
    QCOMPARE(static_cast<int>(layer.type), 0);
    QCOMPARE(layer.opacity, 0U);
    QCOMPARE(layer.isVisible, 0);
    QCOMPARE(layer.hasMask, 0);
    QCOMPARE(layer.propptr, uint32_t(0));
    QCOMPARE(layer.pixels.hierarchy, uint32_t(0));
    QCOMPARE(layer.mask.hierarchy, uint32_t(0));
    QCOMPARE(layer.isGroup, 0);
    QCOMPARE(layer.pathLength, 0U);
    QVERIFY(layer.path == nullptr);

    const xcfImage image {};
    QCOMPARE(image.version, 0);
    QCOMPARE(image.width, 0U);
    QCOMPARE(image.height, 0U);
    QCOMPARE(static_cast<int>(image.type), 0);
    QCOMPARE(static_cast<int>(image.compression), 0);
    QCOMPARE(image.numLayers, 0);
    QVERIFY(image.layers == nullptr);
    QCOMPARE(image.colormapptr, uint32_t(0));
}

void XcfToolsValueContractTest::signedAndDimensionValuesRemainIndependent()
{
    rect bounds {};
    bounds.t = -31;
    bounds.b = 47;
    bounds.l = -19;
    bounds.r = 53;

    tileDimensions dimensions {};
    dimensions.c = bounds;
    dimensions.width = 72;
    dimensions.height = 65;
    dimensions.tilesx = 2;
    dimensions.tilesy = 3;
    dimensions.ntiles = 6;

    QCOMPARE(dimensions.c.t, -31);
    QCOMPARE(dimensions.c.b, 47);
    QCOMPARE(dimensions.c.l, -19);
    QCOMPARE(dimensions.c.r, 53);
    QCOMPARE(dimensions.width, 72U);
    QCOMPARE(dimensions.height, 65U);
    QCOMPARE(dimensions.tilesx, 2U);
    QCOMPARE(dimensions.tilesy, 3U);
    QCOMPARE(dimensions.ntiles, 6U);

    xcfLayer layer {};
    layer.dim = dimensions;
    QCOMPARE(layer.dim.c.l, -19);
    QCOMPARE(layer.dim.c.t, -31);
    QCOMPARE(layer.dim.width, 72U);
    QCOMPARE(layer.dim.ntiles, 6U);

    xcfImage image {};
    image.width = 4097;
    image.height = 3073;
    QCOMPARE(image.width, 4097U);
    QCOMPARE(image.height, 3073U);
}

void XcfToolsValueContractTest::borrowedPointerValuesRemainShallowOnCopy()
{
    alignas(std::max_align_t) unsigned char paramsStorage {};
    const auto *params = reinterpret_cast<const _convertParams *>(&paramsStorage);
    uint32_t tilePointers[] {11, 23};
    unsigned path[] {5, 8, 13};
    const char name[] = "borrowed-layer-name";

    xcfLayer layers[1] {};
    layers[0].name = name;
    layers[0].path = path;
    layers[0].pathLength = 3;
    layers[0].pixels.params = params;
    layers[0].pixels.tileptrs = tilePointers;
    layers[0].mask.params = params;
    layers[0].mask.tileptrs = tilePointers;

    xcfImage image {};
    image.layers = layers;

    const xcfLayer layerCopy = layers[0];
    const xcfImage imageCopy = image;

    QVERIFY(layerCopy.name == layers[0].name);
    QVERIFY(layerCopy.path == layers[0].path);
    QVERIFY(layerCopy.pixels.params == layers[0].pixels.params);
    QVERIFY(layerCopy.pixels.tileptrs == layers[0].pixels.tileptrs);
    QVERIFY(layerCopy.mask.params == layers[0].mask.params);
    QVERIFY(layerCopy.mask.tileptrs == layers[0].mask.tileptrs);
    QVERIFY(imageCopy.layers == image.layers);
}

void XcfToolsValueContractTest::copiesSharePointersAndKeepScalarStateIndependent()
{
    uint32_t tilePointers[] {17, 29};
    unsigned path[] {2, 4};
    const char name[] = "copy-contract";

    xcfLayer originalLayer {};
    originalLayer.dim.c.t = -7;
    originalLayer.dim.width = 129;
    originalLayer.mode = GIMP_MULTIPLY_MODE;
    originalLayer.type = GIMP_RGBA_IMAGE;
    originalLayer.opacity = 201;
    originalLayer.isVisible = 1;
    originalLayer.hasMask = 1;
    originalLayer.propptr = 101;
    originalLayer.pixels.hierarchy = 202;
    originalLayer.pixels.tileptrs = tilePointers;
    originalLayer.mask.hierarchy = 303;
    originalLayer.isGroup = 1;
    originalLayer.pathLength = 2;
    originalLayer.path = path;
    originalLayer.name = name;

    xcfLayer layerCopy = originalLayer;
    layerCopy.dim.c.t = 70;
    layerCopy.dim.width = 513;
    layerCopy.mode = GIMP_SCREEN_MODE;
    layerCopy.type = GIMP_GRAY_IMAGE;
    layerCopy.opacity = 77;
    layerCopy.isVisible = 0;
    layerCopy.hasMask = 0;
    layerCopy.propptr = 404;
    layerCopy.pixels.hierarchy = 505;
    layerCopy.mask.hierarchy = 606;
    layerCopy.isGroup = 0;
    layerCopy.pathLength = 1;

    QCOMPARE(originalLayer.dim.c.t, -7);
    QCOMPARE(originalLayer.dim.width, 129U);
    QCOMPARE(originalLayer.mode, GIMP_MULTIPLY_MODE);
    QCOMPARE(originalLayer.type, GIMP_RGBA_IMAGE);
    QCOMPARE(originalLayer.opacity, 201U);
    QCOMPARE(originalLayer.isVisible, 1);
    QCOMPARE(originalLayer.hasMask, 1);
    QCOMPARE(originalLayer.propptr, uint32_t(101));
    QCOMPARE(originalLayer.pixels.hierarchy, uint32_t(202));
    QCOMPARE(originalLayer.mask.hierarchy, uint32_t(303));
    QCOMPARE(originalLayer.isGroup, 1);
    QCOMPARE(originalLayer.pathLength, 2U);
    QVERIFY(layerCopy.name == originalLayer.name);
    QVERIFY(layerCopy.path == originalLayer.path);
    QVERIFY(layerCopy.pixels.tileptrs == originalLayer.pixels.tileptrs);

    xcfImage originalImage {};
    originalImage.version = -5;
    originalImage.width = 2049;
    originalImage.height = 1025;
    originalImage.type = GIMP_INDEXED;
    originalImage.compression = COMPRESS_RLE;
    originalImage.numLayers = 1;
    originalImage.layers = &originalLayer;
    originalImage.colormapptr = 707;

    xcfImage imageCopy = originalImage;
    imageCopy.version = 12;
    imageCopy.width = 300;
    imageCopy.height = 200;
    imageCopy.type = GIMP_RGB;
    imageCopy.compression = COMPRESS_NONE;
    imageCopy.numLayers = 4;
    imageCopy.colormapptr = 808;

    QCOMPARE(originalImage.version, -5);
    QCOMPARE(originalImage.width, 2049U);
    QCOMPARE(originalImage.height, 1025U);
    QCOMPARE(originalImage.type, GIMP_INDEXED);
    QCOMPARE(originalImage.compression, COMPRESS_RLE);
    QCOMPARE(originalImage.numLayers, 1);
    QCOMPARE(originalImage.colormapptr, uint32_t(707));
    QVERIFY(imageCopy.layers == originalImage.layers);
}

void XcfToolsValueContractTest::xcfPrimitiveAndGlobalSchemaRemainsStable()
{
    static_assert(std::is_same_v<int32_t, std::int32_t>);
    static_assert(std::is_same_v<int8_t, std::int8_t>);
    static_assert(std::is_same_v<uint32_t, std::uint32_t>);
    static_assert(std::is_same_v<uint8_t, std::uint8_t>);
    static_assert(std::is_same_v<uintptr_t, std::uintptr_t>);

    static_assert(std::is_same_v<decltype(XCF), xcfImage>);
    static_assert(std::is_same_v<decltype(progname), const char *>);
    static_assert(std::is_same_v<decltype(use_utf8), int>);
    static_assert(std::is_same_v<decltype(verboseFlag), int>);
    static_assert(std::is_same_v<decltype(xcf_file), uint8_t *>);
    static_assert(std::is_same_v<decltype(xcf_length), size_t>);
}

void XcfToolsValueContractTest::xcfFatalDiagnosticSignaturesRemainStable()
{
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(const char *, ...)>(&FatalBadXCF)), void (*)(const char *, ...)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(int, const char *, ...)>(&FatalGeneric)),
                                 void (*)(int, const char *, ...)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(const char *, ...)>(&FatalUnexpected)),
                                 void (*)(const char *, ...)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(const char *, ...)>(&FatalUnsupportedXCF)),
                                 void (*)(const char *, ...)>);
}

void XcfToolsValueContractTest::xcfFileLifecycleSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<void (*)(const char *, const char *)>(&read_or_mmap_xcf)),
                                 void (*)(const char *, const char *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)()>(&free_or_close_xcf)), void (*)()>);
    static_assert(std::is_same_v<decltype(static_cast<FILE *(*)(const char *)>(&openout)), FILE *(*)(const char *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<int (*)(FILE *, const char *)>(&closeout)), int (*)(FILE *, const char *)>);
}

void XcfToolsValueContractTest::xcfAllocationAndSupportSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<void *(*)(size_t)>(&xcfmalloc)), void *(*)(size_t)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(void *)>(&xcffree)), void (*)(void *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)()>(&gpl_blurb)), void (*)()>);
    static_assert(std::is_same_v<decltype(nls_init()), void>);
}

void XcfToolsValueContractTest::xcfParsingSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<int (*)(tileDimensions *)>(&computeDimensions)),
                                 int (*)(tileDimensions *)>);
    static_assert(std::is_same_v<decltype(static_cast<int (*)()>(&getBasicXcfInfo)), int (*)()>);
    static_assert(std::is_same_v<decltype(ntohl(uint32_t{})), uint32_t>);
    static_assert(std::is_same_v<decltype(static_cast<int (*)(uint32_t, int, const char *, ...)>(&xcfCheckspace)),
                                 int (*)(uint32_t, int, const char *, ...)>);
    static_assert(std::is_same_v<decltype(static_cast<int (*)(uint32_t *, uint32_t *, PropType *)>(&xcfNextprop)),
                                 int (*)(uint32_t *, uint32_t *, PropType *)>);
    static_assert(std::is_same_v<decltype(static_cast<int (*)(uint32_t, int, uint32_t *)>(&xcfOffset)),
                                 int (*)(uint32_t, int, uint32_t *)>);
    static_assert(std::is_same_v<decltype(static_cast<const char *(*)(uint32_t, uint32_t *)>(&xcfString)),
                                 const char *(*)(uint32_t, uint32_t *)>);
}

QTEST_APPLESS_MAIN(XcfToolsValueContractTest)

#include "XcfToolsValueContractTest.moc"
