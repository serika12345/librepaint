/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cached_paint_device.h"
#include "kis_paint_device.h"

#include <QTest>

#include <type_traits>

#define ASSERT_DEVICE_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintDevice::method)), signature>)
#define ASSERT_CACHED_DEVICE_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCachedPaintDevice::method)), signature>)
#define ASSERT_CACHED_SELECTION_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCachedSelection::method)), signature>)

class KisPaintDeviceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void persistenceParentAndPositionSignaturesRemainStable();
    void boundsRegionAndClearingSignaturesRemainStable();
    void defaultPixelFillAndCloneSignaturesRemainStable();
    void byteAndPlanarTransferSignaturesRemainStable();
    void colorConversionAndThumbnailSignaturesRemainStable();
    void pixelColorSpaceAndCompositionSignaturesRemainStable();
    void interstrokeKeyframeAndDirtySignaturesRemainStable();
    void iteratorSelectionAndHistorySignaturesRemainStable();
    void notificationBoundsAndMemorySignaturesRemainStable();
    void lodSignaturesRemainStable();
    void wraparoundProjectionAndTestingSignaturesRemainStable();
    void cachedPaintDeviceTypeSchemaRemainStable();
    void cachedPaintDeviceOperationSignaturesRemainStable();
    void cachedPaintDeviceGuardSchemaRemainStable();
    void cachedSelectionOperationSignaturesRemainStable();
    void cachedSelectionGuardSchemaRemainStable();
};

// clang-format off
void KisPaintDeviceSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Device = KisPaintDevice;
    static_assert(std::is_same_v<KisDataManagerSP, KisSharedPtr<KisDataManager>>);
    static_assert(std::is_class_v<Device>);
    static_assert(std::is_enum_v<KritaUtils::DeviceCopyMode>);
    static_assert(KritaUtils::CopySnapshot == 0);
    static_assert(KritaUtils::CopyAllFrames == 1);
    static_assert(std::is_constructible_v<Device, const KoColorSpace *, const QString &>);
    static_assert(std::is_constructible_v<Device, KisNodeWSP, const KoColorSpace *, KisDefaultBoundsBaseSP, const QString &>);
    static_assert(std::is_constructible_v<Device, const Device &, KritaUtils::DeviceCopyMode, KisNode *>);
    static_assert(std::has_virtual_destructor_v<Device>);
    static_assert(std::is_class_v<Device::MemoryReleaseObject>);
    static_assert(std::has_virtual_destructor_v<Device::MemoryReleaseObject>);
    static_assert(std::is_class_v<Device::LodDataStruct>);
    static_assert(std::has_virtual_destructor_v<Device::LodDataStruct>);
}

void KisPaintDeviceSchemaContractTest::persistenceParentAndPositionSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(write, bool (Device::*)(KisPaintDeviceWriter &));
    ASSERT_DEVICE_SIGNATURE(read, bool (Device::*)(QIODevice *));
    ASSERT_DEVICE_SIGNATURE(setParentNode, void (Device::*)(KisNodeWSP));
    ASSERT_DEVICE_SIGNATURE(setDefaultBounds, void (Device::*)(KisDefaultBoundsBaseSP));
    ASSERT_DEVICE_SIGNATURE(defaultBounds, KisDefaultBoundsBaseSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(moveTo, void (Device::*)(qint32, qint32));
    ASSERT_DEVICE_SIGNATURE(moveTo, void (Device::*)(const QPoint &));
    ASSERT_DEVICE_SIGNATURE(offset, QPoint (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(x, qint32 (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(y, qint32 (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(setX, void (Device::*)(qint32));
    ASSERT_DEVICE_SIGNATURE(setY, void (Device::*)(qint32));
}

void KisPaintDeviceSchemaContractTest::boundsRegionAndClearingSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(extent, QRect (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(extent, void (Device::*)(qint32 &, qint32 &, qint32 &, qint32 &) const);
    ASSERT_DEVICE_SIGNATURE(exactBounds, QRect (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(exactBoundsAmortized, QRect (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(nonDefaultPixelArea, QRect (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(region, KisRegion (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(regionExact, KisRegion (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(crop, void (Device::*)(qint32, qint32, qint32, qint32));
    ASSERT_DEVICE_SIGNATURE(crop, void (Device::*)(const QRect &));
    ASSERT_DEVICE_SIGNATURE(clear, void (Device::*)());
    ASSERT_DEVICE_SIGNATURE(clear, void (Device::*)(const QRect &));
    ASSERT_DEVICE_SIGNATURE(purgeDefaultPixels, void (Device::*)());
}

void KisPaintDeviceSchemaContractTest::defaultPixelFillAndCloneSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(setDefaultPixel, void (Device::*)(const KoColor &));
    ASSERT_DEVICE_SIGNATURE(defaultPixel, KoColor (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(fill, void (Device::*)(const QRect &, const KoColor &));
    ASSERT_DEVICE_SIGNATURE(fill, void (Device::*)(qint32, qint32, qint32, qint32, const quint8 *));
    ASSERT_DEVICE_SIGNATURE(prepareClone, void (Device::*)(KisPaintDeviceSP));
    ASSERT_DEVICE_SIGNATURE(makeCloneFrom, void (Device::*)(KisPaintDeviceSP, const QRect &));
    ASSERT_DEVICE_SIGNATURE(makeCloneFromRough, void (Device::*)(KisPaintDeviceSP, const QRect &));
    ASSERT_DEVICE_SIGNATURE(makeFullCopyFrom, void (Device::*)(const Device &, KritaUtils::DeviceCopyMode, KisNode *));
}

void KisPaintDeviceSchemaContractTest::byteAndPlanarTransferSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(readBytes, void (Device::*)(quint8 *, qint32, qint32, qint32, qint32) const);
    ASSERT_DEVICE_SIGNATURE(readBytes, void (Device::*)(quint8 *, const QRect &) const);
    ASSERT_DEVICE_SIGNATURE(writeBytes, void (Device::*)(const quint8 *, qint32, qint32, qint32, qint32));
    ASSERT_DEVICE_SIGNATURE(writeBytes, void (Device::*)(const quint8 *, const QRect &));
    ASSERT_DEVICE_SIGNATURE(readPlanarBytes, QVector<quint8 *> (Device::*)(qint32, qint32, qint32, qint32) const);
    ASSERT_DEVICE_SIGNATURE(writePlanarBytes, void (Device::*)(QVector<quint8 *>, qint32, qint32, qint32, qint32));
}

void KisPaintDeviceSchemaContractTest::colorConversionAndThumbnailSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    using Intent = KoColorConversionTransformation::Intent;
    using Flags = KoColorConversionTransformation::ConversionFlags;
    ASSERT_DEVICE_SIGNATURE(convertTo, void (Device::*)(const KoColorSpace *, Intent, Flags, KUndo2Command *, KoUpdater *));
    ASSERT_DEVICE_SIGNATURE(setProfile, bool (Device::*)(const KoColorProfile *, KUndo2Command *));
    ASSERT_DEVICE_SIGNATURE(convertFromQImage, void (Device::*)(const QImage &, const KoColorProfile *, qint32, qint32));
    ASSERT_DEVICE_SIGNATURE(convertToQImage, QImage (Device::*)(const KoColorProfile *, qint32, qint32, qint32, qint32, Intent, Flags) const);
    ASSERT_DEVICE_SIGNATURE(convertToQImage, QImage (Device::*)(const KoColorProfile *, const QRect &, Intent, Flags) const);
    ASSERT_DEVICE_SIGNATURE(convertToQImage, QImage (Device::*)(const KoColorProfile *, Intent, Flags) const);
    ASSERT_DEVICE_SIGNATURE(createThumbnailDevice, KisPaintDeviceSP (Device::*)(qint32, qint32, QRect, QRect) const);
    ASSERT_DEVICE_SIGNATURE(createThumbnailDeviceOversampled, KisPaintDeviceSP (Device::*)(qint32, qint32, qreal, QRect, QRect) const);
    ASSERT_DEVICE_SIGNATURE(createThumbnailUncached, QImage (Device::*)(qint32, qint32, QRect, qreal, Intent, Flags));
    ASSERT_DEVICE_SIGNATURE(createThumbnail, QImage (Device::*)(qint32, qint32, KisThumbnailBoundsMode, qreal, Intent, Flags));
    ASSERT_DEVICE_SIGNATURE(createThumbnail, QImage (Device::*)(qint32, qint32, Qt::AspectRatioMode, KisThumbnailBoundsMode, qreal, Intent, Flags));
}

void KisPaintDeviceSchemaContractTest::pixelColorSpaceAndCompositionSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(pixel, bool (Device::*)(qint32, qint32, QColor *) const);
    ASSERT_DEVICE_SIGNATURE(pixel, bool (Device::*)(qint32, qint32, KoColor *) const);
    ASSERT_DEVICE_SIGNATURE(pixel, KoColor (Device::*)(const QPoint &) const);
    ASSERT_DEVICE_SIGNATURE(setPixel, bool (Device::*)(qint32, qint32, const QColor &));
    ASSERT_DEVICE_SIGNATURE(setPixel, bool (Device::*)(qint32, qint32, const KoColor &));
    ASSERT_DEVICE_SIGNATURE(colorSpace, const KoColorSpace *(Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(compositionSourceColorSpace, const KoColorSpace *(Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(createCompositionSourceDevice, KisPaintDeviceSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(createCompositionSourceDevice, KisPaintDeviceSP (Device::*)(KisPaintDeviceSP) const);
    ASSERT_DEVICE_SIGNATURE(createCompositionSourceDevice, KisPaintDeviceSP (Device::*)(KisPaintDeviceSP, const QRect) const);
    ASSERT_DEVICE_SIGNATURE(createCompositionSourceDeviceFixed, KisFixedPaintDeviceSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(dataManager, KisDataManagerSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(setDataManager, void (Device::*)(KisDataManagerSP, const KoColorSpace *));
    ASSERT_DEVICE_SIGNATURE(pixelSize, quint32 (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(channelCount, quint32 (Device::*)() const);
}

void KisPaintDeviceSchemaContractTest::interstrokeKeyframeAndDirtySignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(interstrokeData, KisInterstrokeDataSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(createChangeInterstrokeDataCommand, KUndo2Command *(Device::*)(KisInterstrokeDataSP));
    ASSERT_DEVICE_SIGNATURE(createKeyframeChannel, KisRasterKeyframeChannel *(Device::*)(const KoID &));
    ASSERT_DEVICE_SIGNATURE(keyframeChannel, KisRasterKeyframeChannel *(Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(framesInterface, KisPaintDeviceFramesInterface *(Device::*)());
    ASSERT_DEVICE_SIGNATURE(burnKeyframe, bool (Device::*)(int));
    ASSERT_DEVICE_SIGNATURE(burnKeyframe, bool (Device::*)());
    ASSERT_DEVICE_SIGNATURE(setDirty, void (Device::*)(const QRect &));
    ASSERT_DEVICE_SIGNATURE(setDirty, void (Device::*)(const KisRegion &));
    ASSERT_DEVICE_SIGNATURE(setDirty, void (Device::*)());
    ASSERT_DEVICE_SIGNATURE(setDirty, void (Device::*)(const QVector<QRect> &));
    ASSERT_DEVICE_SIGNATURE(requestTimeSwitch, void (Device::*)(int));
    ASSERT_DEVICE_SIGNATURE(sequenceNumber, int (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(estimateMemoryStats, void (Device::*)(qint64 &, qint64 &, qint64 &) const);
}

void KisPaintDeviceSchemaContractTest::iteratorSelectionAndHistorySignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(createHLineIteratorNG, KisHLineIteratorSP (Device::*)(qint32, qint32, qint32));
    ASSERT_DEVICE_SIGNATURE(createHLineConstIteratorNG, KisHLineConstIteratorSP (Device::*)(qint32, qint32, qint32) const);
    ASSERT_DEVICE_SIGNATURE(createVLineIteratorNG, KisVLineIteratorSP (Device::*)(qint32, qint32, qint32));
    ASSERT_DEVICE_SIGNATURE(createVLineConstIteratorNG, KisVLineConstIteratorSP (Device::*)(qint32, qint32, qint32) const);
    ASSERT_DEVICE_SIGNATURE(createRandomAccessorNG, KisRandomAccessorSP (Device::*)());
    ASSERT_DEVICE_SIGNATURE(createRandomConstAccessorNG, KisRandomConstAccessorSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(createRepeatHLineConstIterator, KisRepeatHLineConstIteratorSP (Device::*)(qint32, qint32, qint32, const QRect &) const);
    ASSERT_DEVICE_SIGNATURE(createRepeatVLineConstIterator, KisRepeatVLineConstIteratorSP (Device::*)(qint32, qint32, qint32, const QRect &) const);
    ASSERT_DEVICE_SIGNATURE(createRandomSubAccessor, KisRandomSubAccessorSP (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(clearSelection, void (Device::*)(KisSelectionSP));
    ASSERT_DEVICE_SIGNATURE(reincarnateWithDetachedHistory, KUndo2Command *(Device::*)(bool));
}

void KisPaintDeviceSchemaContractTest::notificationBoundsAndMemorySignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(profileChanged, void (Device::*)(const KoColorProfile *));
    ASSERT_DEVICE_SIGNATURE(colorSpaceChanged, void (Device::*)(const KoColorSpace *));
    ASSERT_DEVICE_SIGNATURE(calculateExactBounds, QRect (Device::*)(bool) const);
    ASSERT_DEVICE_SIGNATURE(createMemoryReleaseObject, Device::MemoryReleaseObject *(*)());
}

void KisPaintDeviceSchemaContractTest::lodSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(regionForLodSyncing, KisRegion (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(createLodDataStruct, Device::LodDataStruct *(Device::*)(int));
    ASSERT_DEVICE_SIGNATURE(updateLodDataStruct, void (Device::*)(Device::LodDataStruct *, const QRect &));
    ASSERT_DEVICE_SIGNATURE(uploadLodDataStruct, void (Device::*)(Device::LodDataStruct *));
    ASSERT_DEVICE_SIGNATURE(generateLodCloneDevice, void (Device::*)(KisPaintDeviceSP, const QRect &, int));
}

void KisPaintDeviceSchemaContractTest::wraparoundProjectionAndTestingSignaturesRemainStable()
{
    using Device = KisPaintDevice;
    ASSERT_DEVICE_SIGNATURE(setSupportsWraparoundMode, void (Device::*)(bool));
    ASSERT_DEVICE_SIGNATURE(supportsWraproundMode, bool (Device::*)() const);
    ASSERT_DEVICE_SIGNATURE(setProjectionDevice, void (Device::*)(bool));
    ASSERT_DEVICE_SIGNATURE(testingFetchLodDevice, void (Device::*)(KisPaintDeviceSP));
}

void KisPaintDeviceSchemaContractTest::cachedPaintDeviceTypeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisCachedPaintDevice>);
    static_assert(std::is_class_v<KisCachedPaintDevice::Guard>);
    static_assert(std::is_class_v<KisCachedSelection>);
    static_assert(std::is_class_v<KisCachedSelection::Guard>);
}

void KisPaintDeviceSchemaContractTest::cachedPaintDeviceOperationSignaturesRemainStable()
{
    using Cache = KisCachedPaintDevice;
    ASSERT_CACHED_DEVICE_SIGNATURE(getDevice, KisPaintDeviceSP (Cache::*)(KisPaintDeviceSP));
    ASSERT_CACHED_DEVICE_SIGNATURE(getDevice, KisPaintDeviceSP (Cache::*)(KisPaintDeviceSP, const KoColorSpace *));
    ASSERT_CACHED_DEVICE_SIGNATURE(isEmpty, bool (Cache::*)() const);
    ASSERT_CACHED_DEVICE_SIGNATURE(putDevice, void (Cache::*)(KisPaintDeviceSP));
}

void KisPaintDeviceSchemaContractTest::cachedPaintDeviceGuardSchemaRemainStable()
{
    using Cache = KisCachedPaintDevice;
    using Guard = Cache::Guard;
    static_assert(std::is_constructible_v<Guard, KisPaintDeviceSP, Cache &>);
    static_assert(std::is_constructible_v<Guard, KisPaintDeviceSP, const KoColorSpace *, Cache &>);
    static_assert(std::is_destructible_v<Guard>);
    static_assert(std::is_same_v<decltype(static_cast<KisPaintDeviceSP (Guard::*)() const>(&Guard::device)),
                                 KisPaintDeviceSP (Guard::*)() const>);
}

void KisPaintDeviceSchemaContractTest::cachedSelectionOperationSignaturesRemainStable()
{
    using Cache = KisCachedSelection;
    ASSERT_CACHED_SELECTION_SIGNATURE(getSelection, KisSelectionSP (Cache::*)());
    ASSERT_CACHED_SELECTION_SIGNATURE(isEmpty, bool (Cache::*)() const);
    ASSERT_CACHED_SELECTION_SIGNATURE(putSelection, void (Cache::*)(KisSelectionSP));
}

void KisPaintDeviceSchemaContractTest::cachedSelectionGuardSchemaRemainStable()
{
    using Cache = KisCachedSelection;
    using Guard = Cache::Guard;
    static_assert(std::is_constructible_v<Guard, Cache &>);
    static_assert(std::is_destructible_v<Guard>);
    static_assert(std::is_same_v<decltype(static_cast<KisSelectionSP (Guard::*)() const>(&Guard::selection)),
                                 KisSelectionSP (Guard::*)() const>);
}
// clang-format on

#undef ASSERT_CACHED_SELECTION_SIGNATURE
#undef ASSERT_CACHED_DEVICE_SIGNATURE
#undef ASSERT_DEVICE_SIGNATURE

QTEST_APPLESS_MAIN(KisPaintDeviceSchemaContractTest)

#include "KisPaintDeviceSchemaContractTest.moc"
