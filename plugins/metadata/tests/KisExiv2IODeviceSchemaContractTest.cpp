/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisExiv2IODevice.h"
#include "kis_exiv2_common.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_EXIV2_IO_DEVICE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisExiv2IODevice::method)), signature>)
} // namespace

class KisExiv2IODeviceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeLifetimeAndCoreIoSchemaRemainStable();
    void readWriteAndPositionSchemaRemainStable();
    void mappingStateAndPathSchemaRemainStable();
    void metadataConversionFunctionSchemaRemainsStable();
};

void KisExiv2IODeviceSchemaContractTest::typeLifetimeAndCoreIoSchemaRemainStable()
{
    using Device = KisExiv2IODevice;

    static_assert(std::is_base_of_v<Exiv2::BasicIo, Device>);
    static_assert(std::is_constructible_v<Device, QString>);
    static_assert(std::has_virtual_destructor_v<Device>);
#if EXIV2_TEST_VERSION(0, 28, 0)
    static_assert(std::is_same_v<Device::ptr_type, Exiv2::BasicIo::UniquePtr>);
#else
    static_assert(std::is_same_v<Device::ptr_type, Exiv2::BasicIo::AutoPtr>);
#endif
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(open, int (Device::*)());
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(close, int (Device::*)());
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(getb, int (Device::*)());
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(putb, int (Device::*)(Exiv2::byte));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(transfer, void (Device::*)(Exiv2::BasicIo &));

    QVERIFY(true);
}

void KisExiv2IODeviceSchemaContractTest::readWriteAndPositionSchemaRemainStable()
{
    using Device = KisExiv2IODevice;

#if EXIV2_TEST_VERSION(0, 28, 0)
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(write, size_t (Device::*)(Exiv2::BasicIo &));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(write, size_t (Device::*)(const Exiv2::byte *, size_t));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(read, Exiv2::DataBuf (Device::*)(size_t));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(read, size_t (Device::*)(Exiv2::byte *, size_t));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(seek, int (Device::*)(int64_t, Exiv2::BasicIo::Position));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(tell, size_t (Device::*)() const);
#else
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(write, long (Device::*)(Exiv2::BasicIo &));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(write, long (Device::*)(const Exiv2::byte *, long));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(read, Exiv2::DataBuf (Device::*)(long));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(read, long (Device::*)(Exiv2::byte *, long));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(seek, int (Device::*)(long, Exiv2::BasicIo::Position));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(tell, long (Device::*)() const);
#endif
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(size, size_t (Device::*)() const);

    QVERIFY(true);
}

void KisExiv2IODeviceSchemaContractTest::mappingStateAndPathSchemaRemainStable()
{
    using Device = KisExiv2IODevice;

    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(mmap, Exiv2::byte * (Device::*)(bool));
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(munmap, int (Device::*)());
#if EXIV2_TEST_VERSION(0, 28, 0)
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(populateFakeData, void (Device::*)());
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(path, const std::string &(Device::*)() const noexcept);
#else
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(path, std::string (Device::*)() const);
#endif
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(isopen, bool (Device::*)() const);
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(error, int (Device::*)() const);
    ASSERT_EXIV2_IO_DEVICE_SIGNATURE(eof, bool (Device::*)() const);

    QVERIFY(true);
}

void KisExiv2IODeviceSchemaContractTest::metadataConversionFunctionSchemaRemainsStable()
{
#if EXIV2_TEST_VERSION(0, 28, 0)
    using ExivValueToKmdValue =
        KisMetaData::Value (*)(const Exiv2::Value::UniquePtr &, bool, KisMetaData::Value::ValueType);
#else
    using ExivValueToKmdValue =
        KisMetaData::Value (*)(const Exiv2::Value::AutoPtr &, bool, KisMetaData::Value::ValueType);
#endif
    using VariantToExivValue = Exiv2::Value *(*)(const QVariant &, Exiv2::TypeId);
    using KmdValueToExivValue = Exiv2::Value *(*)(const KisMetaData::Value &, Exiv2::TypeId);
    using KmdValueToExivXmpValue = Exiv2::Value *(*)(const KisMetaData::Value &);
    using ArrayToExivValue = Exiv2::Value *(*)(const KisMetaData::Value &);

    static_assert(std::is_same_v<decltype(&exivValueToKMDValue), ExivValueToKmdValue>);
    static_assert(std::is_same_v<decltype(&variantToExivValue), VariantToExivValue>);
    static_assert(std::is_same_v<decltype(&kmdValueToExivValue), KmdValueToExivValue>);
    static_assert(std::is_same_v<decltype(&kmdValueToExivXmpValue), KmdValueToExivXmpValue>);
    static_assert(std::is_same_v<decltype(&arrayToExivValue<uint16_t>), ArrayToExivValue>);

    QVERIFY(true);
}

#undef ASSERT_EXIV2_IO_DEVICE_SIGNATURE

QTEST_APPLESS_MAIN(KisExiv2IODeviceSchemaContractTest)

#include "KisExiv2IODeviceSchemaContractTest.moc"
