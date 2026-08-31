/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoStoreDevice.h"

#include <QHash>
#include <QTest>

namespace
{
struct StoreState {
    KoStore::Mode mode{KoStore::Read};
    qint64 size{0};
    qint64 position{0};
    bool atEnd{false};
    bool seekResult{true};
    int seekCalls{0};
    qint64 lastSeekPosition{0};
};

QHash<const KoStore *, StoreState> storeStates;

StoreState &stateFor(const KoStore *store)
{
    return storeStates[store];
}

class RecordingStore final : public KoStore
{
public:
    explicit RecordingStore(Mode mode, bool *destroyed = nullptr)
        : KoStore(mode)
        , m_destroyed(destroyed)
    {
    }

    ~RecordingStore() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    StoreState &state()
    {
        return stateFor(this);
    }

protected:
    bool openWrite(const QString &) override
    {
        return false;
    }

    bool openRead(const QString &) override
    {
        return false;
    }

    bool closeRead() override
    {
        return false;
    }

    bool closeWrite() override
    {
        return false;
    }

    bool enterRelativeDirectory(const QString &) override
    {
        return false;
    }

    bool enterAbsoluteDirectory(const QString &) override
    {
        return false;
    }

    bool fileExists(const QString &) const override
    {
        return false;
    }

private:
    bool *m_destroyed;
};
} // namespace

KoStore::KoStore(Mode mode, bool)
    : d_ptr(nullptr)
{
    stateFor(this).mode = mode;
}

KoStore::~KoStore()
{
    storeStates.remove(this);
}

qint64 KoStore::read(char *, qint64)
{
    return -1;
}

qint64 KoStore::write(const char *, qint64)
{
    return -1;
}

QStringList KoStore::directoryList() const
{
    return {};
}

bool KoStore::enterDirectory(const QString &)
{
    return false;
}

void KoStore::setCompressionEnabled(bool)
{
}

qint64 KoStore::size() const
{
    return stateFor(this).size;
}

KoStore::Mode KoStore::mode() const
{
    return stateFor(this).mode;
}

bool KoStore::seek(qint64 position)
{
    StoreState &state = stateFor(this);
    state.seekCalls++;
    state.lastSeekPosition = position;
    return state.seekResult;
}

qint64 KoStore::pos() const
{
    return stateFor(this).position;
}

bool KoStore::atEnd() const
{
    return stateFor(this).atEnd;
}

class KoStoreDeviceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionMapsModeAndBorrowsStore();
    void openValidatesStoreModeAfterUpdatingDeviceMode();
    void sizeSequentialAndClosePreserveCompatibilityBehavior();
    void positionAndEndStateFollowTheBorrowedStore();
    void seekForwardsPositionAndResultExactlyOnce();
};

void KoStoreDeviceContractTest::constructionMapsModeAndBorrowsStore()
{
    bool destroyed = false;
    auto *readStore = new RecordingStore(KoStore::Read, &destroyed);

    {
        KoStoreDevice device(readStore);
        QCOMPARE(device.openMode(), QIODevice::ReadOnly);
        QVERIFY(device.isOpen());
    }

    QVERIFY(!destroyed);
    delete readStore;
    QVERIFY(destroyed);

    RecordingStore writeStore(KoStore::Write);
    KoStoreDevice writeDevice(&writeStore);
    QCOMPARE(writeDevice.openMode(), QIODevice::WriteOnly);
}

void KoStoreDeviceContractTest::openValidatesStoreModeAfterUpdatingDeviceMode()
{
    RecordingStore readStore(KoStore::Read);
    KoStoreDevice readDevice(&readStore);

    QVERIFY(readDevice.open(QIODevice::ReadOnly));
    QCOMPARE(readDevice.openMode(), QIODevice::ReadOnly);
    QVERIFY(!readDevice.open(QIODevice::WriteOnly));
    QCOMPARE(readDevice.openMode(), QIODevice::WriteOnly);

    RecordingStore writeStore(KoStore::Write);
    KoStoreDevice writeDevice(&writeStore);

    QVERIFY(writeDevice.open(QIODevice::WriteOnly));
    QVERIFY(!writeDevice.open(QIODevice::ReadOnly));
    QCOMPARE(writeDevice.openMode(), QIODevice::ReadOnly);
    QVERIFY(!writeDevice.open(QIODevice::NotOpen));
    QCOMPARE(writeDevice.openMode(), QIODevice::NotOpen);
}

void KoStoreDeviceContractTest::sizeSequentialAndClosePreserveCompatibilityBehavior()
{
    RecordingStore readStore(KoStore::Read);
    readStore.state().size = 137;
    KoStoreDevice readDevice(&readStore);

    QVERIFY(readDevice.isSequential());
    QCOMPARE(readDevice.size(), qint64(137));
    readDevice.close();
    QVERIFY(readDevice.isOpen());
    QCOMPARE(readDevice.openMode(), QIODevice::ReadOnly);

    RecordingStore writeStore(KoStore::Write);
    KoStoreDevice writeDevice(&writeStore);
    QCOMPARE(writeDevice.size(), qint64(0xffffffff));
}

void KoStoreDeviceContractTest::positionAndEndStateFollowTheBorrowedStore()
{
    RecordingStore store(KoStore::Read);
    KoStoreDevice device(&store);

    store.state().position = 29;
    store.state().atEnd = false;
    QCOMPARE(device.pos(), qint64(29));
    QVERIFY(!device.atEnd());

    store.state().position = 311;
    store.state().atEnd = true;
    QCOMPARE(device.pos(), qint64(311));
    QVERIFY(device.atEnd());
}

void KoStoreDeviceContractTest::seekForwardsPositionAndResultExactlyOnce()
{
    RecordingStore store(KoStore::Read);
    KoStoreDevice device(&store);

    store.state().seekResult = true;
    QVERIFY(device.seek(173));
    QCOMPARE(store.state().seekCalls, 1);
    QCOMPARE(store.state().lastSeekPosition, qint64(173));

    store.state().seekResult = false;
    QVERIFY(!device.seek(-19));
    QCOMPARE(store.state().seekCalls, 2);
    QCOMPARE(store.state().lastSeekPosition, qint64(-19));
}

QTEST_GUILESS_MAIN(KoStoreDeviceContractTest)

#include "KoStoreDeviceContractTest.moc"
