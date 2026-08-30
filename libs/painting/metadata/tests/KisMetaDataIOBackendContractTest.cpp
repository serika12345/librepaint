/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_io_backend.h"

#include <QTest>

#include <memory>

namespace
{
class RecordingBackend final : public KisMetaData::IOBackend
{
public:
    explicit RecordingBackend(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingBackend() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QString id() const override
    {
        return idValue;
    }

    QString name() const override
    {
        return nameValue;
    }

    BackendType type() const override
    {
        return backendType;
    }

    bool supportSaving() const override
    {
        return savingSupported;
    }

    bool saveTo(const KisMetaData::Store *store, QIODevice *ioDevice, HeaderType headerType) const override
    {
        savedStore = store;
        saveDevice = ioDevice;
        savedHeaderType = headerType;
        ++saveCalls;
        return saveResult;
    }

    bool canSaveAllEntries(KisMetaData::Store *store) const override
    {
        checkedStore = store;
        ++canSaveCalls;
        return canSaveResult;
    }

    bool supportLoading() const override
    {
        return loadingSupported;
    }

    bool loadFrom(KisMetaData::Store *store, QIODevice *ioDevice) const override
    {
        loadedStore = store;
        loadDevice = ioDevice;
        ++loadCalls;
        return loadResult;
    }

    QString idValue{QStringLiteral("backend/保持")};
    QString nameValue{QStringLiteral("読込み・保存/β")};
    BackendType backendType{Text};
    bool savingSupported{true};
    bool loadingSupported{false};
    bool saveResult{true};
    bool canSaveResult{false};
    bool loadResult{true};

    mutable const KisMetaData::Store *savedStore{nullptr};
    mutable QIODevice *saveDevice{nullptr};
    mutable HeaderType savedHeaderType{JpegHeader};
    mutable int saveCalls{0};
    mutable KisMetaData::Store *checkedStore{nullptr};
    mutable int canSaveCalls{0};
    mutable KisMetaData::Store *loadedStore{nullptr};
    mutable QIODevice *loadDevice{nullptr};
    mutable int loadCalls{0};

private:
    int *m_destructionCount;
};

class KisMetaDataIOBackendContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumerationsAndIdentityRemainStable();
    void capabilitiesAreIndependentlyDispatched();
    void savePreservesArgumentsDefaultsAndReturnValue();
    void loadAndCanSavePreserveArgumentsAndReturnValues();
    void baseOwnershipHasVirtualLifetime();
};

void KisMetaDataIOBackendContractTest::enumerationsAndIdentityRemainStable()
{
    QCOMPARE(static_cast<int>(KisMetaData::IOBackend::Binary), 0);
    QCOMPARE(static_cast<int>(KisMetaData::IOBackend::Text), 1);
    QCOMPARE(static_cast<int>(KisMetaData::IOBackend::NoHeader), 0);
    QCOMPARE(static_cast<int>(KisMetaData::IOBackend::JpegHeader), 1);

    RecordingBackend backend;
    const KisMetaData::IOBackend &interface = backend;

    QCOMPARE(interface.id(), QStringLiteral("backend/保持"));
    QCOMPARE(interface.name(), QStringLiteral("読込み・保存/β"));
    QCOMPARE(interface.type(), KisMetaData::IOBackend::Text);

    backend.backendType = KisMetaData::IOBackend::Binary;
    QCOMPARE(interface.type(), KisMetaData::IOBackend::Binary);
}

void KisMetaDataIOBackendContractTest::capabilitiesAreIndependentlyDispatched()
{
    RecordingBackend backend;
    const KisMetaData::IOBackend &interface = backend;

    QVERIFY(interface.supportSaving());
    QVERIFY(!interface.supportLoading());

    backend.savingSupported = false;
    backend.loadingSupported = true;

    QVERIFY(!interface.supportSaving());
    QVERIFY(interface.supportLoading());
}

void KisMetaDataIOBackendContractTest::savePreservesArgumentsDefaultsAndReturnValue()
{
    char storeToken = 0;
    char deviceToken = 0;
    const auto *store = reinterpret_cast<const KisMetaData::Store *>(&storeToken);
    auto *device = reinterpret_cast<QIODevice *>(&deviceToken);

    RecordingBackend backend;
    const KisMetaData::IOBackend &interface = backend;

    QVERIFY(interface.saveTo(store, device));
    QCOMPARE(backend.savedStore, store);
    QCOMPARE(backend.saveDevice, device);
    QCOMPARE(backend.savedHeaderType, KisMetaData::IOBackend::NoHeader);
    QCOMPARE(backend.saveCalls, 1);

    backend.saveResult = false;
    QVERIFY(!interface.saveTo(store, device, KisMetaData::IOBackend::JpegHeader));
    QCOMPARE(backend.savedStore, store);
    QCOMPARE(backend.saveDevice, device);
    QCOMPARE(backend.savedHeaderType, KisMetaData::IOBackend::JpegHeader);
    QCOMPARE(backend.saveCalls, 2);
}

void KisMetaDataIOBackendContractTest::loadAndCanSavePreserveArgumentsAndReturnValues()
{
    char storeToken = 0;
    char deviceToken = 0;
    auto *store = reinterpret_cast<KisMetaData::Store *>(&storeToken);
    auto *device = reinterpret_cast<QIODevice *>(&deviceToken);

    RecordingBackend backend;
    const KisMetaData::IOBackend &interface = backend;

    QVERIFY(interface.loadFrom(store, device));
    QCOMPARE(backend.loadedStore, store);
    QCOMPARE(backend.loadDevice, device);
    QCOMPARE(backend.loadCalls, 1);

    QVERIFY(!interface.canSaveAllEntries(store));
    QCOMPARE(backend.checkedStore, store);
    QCOMPARE(backend.canSaveCalls, 1);

    backend.loadResult = false;
    backend.canSaveResult = true;
    QVERIFY(!interface.loadFrom(store, device));
    QVERIFY(interface.canSaveAllEntries(store));
    QCOMPARE(backend.loadCalls, 2);
    QCOMPARE(backend.canSaveCalls, 2);
}

void KisMetaDataIOBackendContractTest::baseOwnershipHasVirtualLifetime()
{
    int destructionCount = 0;
    {
        std::unique_ptr<KisMetaData::IOBackend> backend = std::make_unique<RecordingBackend>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }
    QCOMPARE(destructionCount, 1);
}
} // namespace

QTEST_GUILESS_MAIN(KisMetaDataIOBackendContractTest)

#include "KisMetaDataIOBackendContractTest.moc"
