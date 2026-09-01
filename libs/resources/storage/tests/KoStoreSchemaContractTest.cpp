/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoStore.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KoStoreSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storeTypeAndLifetimeSchemaRemainStable();
    void storeCreationAndOpenStateSignaturesRemainStable();
    void storeDataStreamAndPositionSignaturesRemainStable();
    void storeDirectoryNavigationSignaturesRemainStable();
    void storeExtractionAndConfigurationSignaturesRemainStable();
};

void KoStoreSchemaContractTest::storeTypeAndLifetimeSchemaRemainStable()
{
    using Store = KoStore;

    static_assert(std::is_class_v<Store>);
    static_assert(std::is_abstract_v<Store>);
    static_assert(std::has_virtual_destructor_v<Store>);
    static_assert(std::is_enum_v<Store::Mode>);
    static_assert(std::is_enum_v<Store::Backend>);

    QCOMPARE(static_cast<int>(Store::Read), 0);
    QCOMPARE(static_cast<int>(Store::Write), 1);
    QCOMPARE(static_cast<int>(Store::Auto), 0);
    QCOMPARE(static_cast<int>(Store::Zip), 1);
    QCOMPARE(static_cast<int>(Store::Directory), 2);
}

void KoStoreSchemaContractTest::storeCreationAndOpenStateSignaturesRemainStable()
{
    using Store = KoStore;
    using FileFactory = Store *(*)(const QString &, Store::Mode, const QByteArray &, Store::Backend, bool);
    using DeviceFactory = Store *(*)(QIODevice *, Store::Mode, const QByteArray &, Store::Backend, bool);
    using OpenSignature = bool (Store::*)(const QString &);
    using BooleanOperation = bool (Store::*)();
    using BooleanQuery = bool (Store::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<FileFactory>(&Store::createStore)), FileFactory>);
    static_assert(std::is_same_v<decltype(static_cast<DeviceFactory>(&Store::createStore)), DeviceFactory>);
    static_assert(std::is_same_v<decltype(Store::createStore(std::declval<const QString &>(), Store::Read)), Store *>);
    static_assert(std::is_same_v<decltype(Store::createStore(std::declval<QIODevice *>(), Store::Read)), Store *>);
    static_assert(std::is_same_v<decltype(static_cast<OpenSignature>(&Store::open)), OpenSignature>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Store::isOpen)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanOperation>(&Store::close)), BooleanOperation>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanOperation>(&Store::finalize)), BooleanOperation>);

    QVERIFY(true);
}

void KoStoreSchemaContractTest::storeDataStreamAndPositionSignaturesRemainStable()
{
    using Store = KoStore;
    using DeviceQuery = QIODevice *(Store::*)() const;
    using ReadArraySignature = QByteArray (Store::*)(qint64);
    using ReadBufferSignature = qint64 (Store::*)(char *, qint64);
    using WriteArraySignature = qint64 (Store::*)(const QByteArray &);
    using WriteBufferSignature = qint64 (Store::*)(const char *, qint64);
    using IntegerQuery = qint64 (Store::*)() const;
    using BooleanQuery = bool (Store::*)() const;
    using ModeQuery = Store::Mode (Store::*)() const;
    using SeekSignature = bool (Store::*)(qint64);

    static_assert(std::is_same_v<decltype(static_cast<DeviceQuery>(&Store::device)), DeviceQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ReadArraySignature>(&Store::read)), ReadArraySignature>);
    static_assert(std::is_same_v<decltype(static_cast<ReadBufferSignature>(&Store::read)), ReadBufferSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WriteArraySignature>(&Store::write)), WriteArraySignature>);
    static_assert(std::is_same_v<decltype(static_cast<WriteBufferSignature>(&Store::write)), WriteBufferSignature>);
    static_assert(std::is_same_v<decltype(static_cast<IntegerQuery>(&Store::size)), IntegerQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Store::bad)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ModeQuery>(&Store::mode)), ModeQuery>);
    static_assert(std::is_same_v<decltype(static_cast<SeekSignature>(&Store::seek)), SeekSignature>);
    static_assert(std::is_same_v<decltype(static_cast<IntegerQuery>(&Store::pos)), IntegerQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Store::atEnd)), BooleanQuery>);

    QVERIFY(true);
}

void KoStoreSchemaContractTest::storeDirectoryNavigationSignaturesRemainStable()
{
    using Store = KoStore;
    using DirectoryListQuery = QStringList (Store::*)() const;
    using DirectoryOperation = bool (Store::*)(const QString &);
    using BooleanOperation = bool (Store::*)();
    using PathQuery = QString (Store::*)() const;
    using VoidOperation = void (Store::*)();
    using FileQuery = bool (Store::*)(const QString &) const;

    static_assert(std::is_same_v<decltype(static_cast<DirectoryListQuery>(&Store::directoryList)), DirectoryListQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<DirectoryOperation>(&Store::enterDirectory)), DirectoryOperation>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanOperation>(&Store::leaveDirectory)), BooleanOperation>);
    static_assert(std::is_same_v<decltype(static_cast<PathQuery>(&Store::currentPath)), PathQuery>);
    static_assert(std::is_same_v<decltype(static_cast<VoidOperation>(&Store::pushDirectory)), VoidOperation>);
    static_assert(std::is_same_v<decltype(static_cast<VoidOperation>(&Store::popDirectory)), VoidOperation>);
    static_assert(std::is_same_v<decltype(static_cast<FileQuery>(&Store::hasFile)), FileQuery>);
    static_assert(std::is_same_v<decltype(static_cast<DirectoryOperation>(&Store::hasDirectory)), DirectoryOperation>);

    QVERIFY(true);
}

void KoStoreSchemaContractTest::storeExtractionAndConfigurationSignaturesRemainStable()
{
    using Store = KoStore;
    using ExtractSignature = bool (Store::*)(const QString &, QByteArray &);
    using CompressionSignature = void (Store::*)(bool);
    using SubstitutionSignature = void (Store::*)(const QString &, const QString &);

    static_assert(std::is_same_v<decltype(static_cast<ExtractSignature>(&Store::extractFile)), ExtractSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CompressionSignature>(&Store::setCompressionEnabled)),
                                 CompressionSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SubstitutionSignature>(&Store::setSubstitution)), SubstitutionSignature>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KoStoreSchemaContractTest)

#include "KoStoreSchemaContractTest.moc"
