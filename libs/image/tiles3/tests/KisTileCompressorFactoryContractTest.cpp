/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/swap/kis_tile_compressor_factory.h"

#include <QBuffer>
#include <QTest>

#include <array>
#include <cstddef>
#include <type_traits>

class KisPaintDeviceWriter
{
};

void kisSharedPtrAddReference(KisTile *)
{
    qFatal("a non-null tile entered the abstract compressor contract");
}

bool kisSharedPtrRelease(KisTile *)
{
    qFatal("a non-null tile left the abstract compressor contract");
    return false;
}

namespace
{

int legacyConstructions = 0;
int legacyDestructions = 0;
int currentConstructions = 0;
int currentDestructions = 0;

class RecordingCompressor final : public KisAbstractTileCompressor
{
public:
    enum Call {
        Write,
        Read,
        Compress,
        Decompress,
        BufferSize,
    };

    explicit RecordingCompressor(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingCompressor() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    bool writeTile(KisTileSP tile, KisPaintDeviceWriter &store) override
    {
        calls.append(Write);
        writtenTileWasNull = !tile;
        writer = &store;
        return true;
    }

    bool readTile(QIODevice *stream, KisTiledDataManager *dm) override
    {
        calls.append(Read);
        readStream = stream;
        dataManager = dm;
        return false;
    }

    void compressTileData(KisTileData *tileData, quint8 *buffer, qint32 bufferSize, qint32 &bytesWritten) override
    {
        calls.append(Compress);
        compressedTileData = tileData;
        compressedBuffer = buffer;
        compressedBufferSize = bufferSize;
        bytesWritten = 4;
    }

    bool decompressTileData(quint8 *buffer, qint32 bufferSize, KisTileData *tileData) override
    {
        calls.append(Decompress);
        decompressedBuffer = buffer;
        decompressedBufferSize = bufferSize;
        decompressedTileData = tileData;
        return true;
    }

    qint32 tileDataBufferSize(KisTileData *tileData) override
    {
        calls.append(BufferSize);
        sizedTileData = tileData;
        return 37;
    }

    QList<Call> calls;
    bool writtenTileWasNull{false};
    KisPaintDeviceWriter *writer{nullptr};
    QIODevice *readStream{nullptr};
    KisTiledDataManager *dataManager{nullptr};
    KisTileData *compressedTileData{nullptr};
    quint8 *compressedBuffer{nullptr};
    qint32 compressedBufferSize{-1};
    quint8 *decompressedBuffer{nullptr};
    qint32 decompressedBufferSize{-1};
    KisTileData *decompressedTileData{nullptr};
    KisTileData *sizedTileData{nullptr};

private:
    int *m_destructionCount;
};

void resetLifetimeObservations()
{
    legacyConstructions = 0;
    legacyDestructions = 0;
    currentConstructions = 0;
    currentDestructions = 0;
}

} // namespace

KisLegacyTileCompressor::KisLegacyTileCompressor()
{
    ++legacyConstructions;
}

KisLegacyTileCompressor::~KisLegacyTileCompressor()
{
    ++legacyDestructions;
}

bool KisLegacyTileCompressor::writeTile(KisTileSP, KisPaintDeviceWriter &)
{
    return false;
}

bool KisLegacyTileCompressor::readTile(QIODevice *, KisTiledDataManager *)
{
    return false;
}

void KisLegacyTileCompressor::compressTileData(KisTileData *, quint8 *, qint32, qint32 &bytesWritten)
{
    bytesWritten = 0;
}

bool KisLegacyTileCompressor::decompressTileData(quint8 *, qint32, KisTileData *)
{
    return false;
}

qint32 KisLegacyTileCompressor::tileDataBufferSize(KisTileData *)
{
    return 0;
}

KisTileCompressor2::KisTileCompressor2()
    : m_compression(nullptr)
{
    ++currentConstructions;
}

KisTileCompressor2::~KisTileCompressor2()
{
    ++currentDestructions;
}

bool KisTileCompressor2::writeTile(KisTileSP, KisPaintDeviceWriter &)
{
    return false;
}

bool KisTileCompressor2::readTile(QIODevice *, KisTiledDataManager *)
{
    return false;
}

void KisTileCompressor2::compressTileData(KisTileData *, quint8 *, qint32, qint32 &bytesWritten)
{
    bytesWritten = 0;
}

bool KisTileCompressor2::decompressTileData(quint8 *, qint32, KisTileData *)
{
    return false;
}

qint32 KisTileCompressor2::tileDataBufferSize(KisTileData *)
{
    return 0;
}

class KisTileCompressorFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abstractTypeAndSharedOwnershipHaveVirtualLifetime();
    void streamOperationsPreserveArgumentsAndDispatchResults();
    void bufferOperationsPreserveStorageSizesAndOutParameters();
    void versionOneCreatesDistinctOwnedLegacyCompressors();
    void versionTwoCreatesDistinctOwnedCurrentCompressors();
};

void KisTileCompressorFactoryContractTest::abstractTypeAndSharedOwnershipHaveVirtualLifetime()
{
    static_assert(std::is_abstract_v<KisAbstractTileCompressor>);
    static_assert(std::is_polymorphic_v<KisAbstractTileCompressor>);
    static_assert(std::has_virtual_destructor_v<KisAbstractTileCompressor>);
    static_assert(std::is_same_v<KisAbstractTileCompressorSP, KisSharedPtr<KisAbstractTileCompressor>>);

    int destructionCount = 0;
    KisAbstractTileCompressorSP first(new RecordingCompressor(&destructionCount));
    KisAbstractTileCompressorSP second = first;

    first.clear();
    QCOMPARE(destructionCount, 0);
    second.clear();
    QCOMPARE(destructionCount, 1);
}

void KisTileCompressorFactoryContractTest::streamOperationsPreserveArgumentsAndDispatchResults()
{
    alignas(KisTiledDataManager) std::byte managerStorage;
    auto *manager = reinterpret_cast<KisTiledDataManager *>(&managerStorage);
    KisPaintDeviceWriter writer;
    QByteArray streamStorage("tile-stream");
    QBuffer stream(&streamStorage);

    RecordingCompressor compressor;
    KisAbstractTileCompressor &interface = compressor;

    QVERIFY(interface.writeTile(KisTileSP(), writer));
    QVERIFY(!interface.readTile(&stream, manager));

    QVERIFY(compressor.writtenTileWasNull);
    QCOMPARE(compressor.writer, &writer);
    QCOMPARE(compressor.readStream, &stream);
    QCOMPARE(compressor.dataManager, manager);
    QCOMPARE(compressor.calls,
             QList<RecordingCompressor::Call>({RecordingCompressor::Write, RecordingCompressor::Read}));
}

void KisTileCompressorFactoryContractTest::bufferOperationsPreserveStorageSizesAndOutParameters()
{
    alignas(KisTileData) std::byte tileDataStorage;
    auto *tileData = reinterpret_cast<KisTileData *>(&tileDataStorage);
    std::array<quint8, 6> buffer{1, 2, 3, 4, 5, 6};
    qint32 bytesWritten = -1;

    RecordingCompressor compressor;
    KisAbstractTileCompressor &interface = compressor;

    interface.compressTileData(tileData, buffer.data(), qint32(buffer.size()), bytesWritten);
    QVERIFY(interface.decompressTileData(buffer.data(), qint32(buffer.size()), tileData));
    QCOMPARE(interface.tileDataBufferSize(tileData), 37);

    QCOMPARE(bytesWritten, 4);
    QCOMPARE(compressor.compressedTileData, tileData);
    QCOMPARE(compressor.compressedBuffer, buffer.data());
    QCOMPARE(compressor.compressedBufferSize, qint32(buffer.size()));
    QCOMPARE(compressor.decompressedBuffer, buffer.data());
    QCOMPARE(compressor.decompressedBufferSize, qint32(buffer.size()));
    QCOMPARE(compressor.decompressedTileData, tileData);
    QCOMPARE(compressor.sizedTileData, tileData);
    QCOMPARE(compressor.calls,
             QList<RecordingCompressor::Call>(
                 {RecordingCompressor::Compress, RecordingCompressor::Decompress, RecordingCompressor::BufferSize}));
}

void KisTileCompressorFactoryContractTest::versionOneCreatesDistinctOwnedLegacyCompressors()
{
    resetLifetimeObservations();

    KisAbstractTileCompressorSP first = KisTileCompressorFactory::create(1);
    KisAbstractTileCompressorSP second = KisTileCompressorFactory::create(1);

    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first.data() != second.data());
    QVERIFY(dynamic_cast<KisLegacyTileCompressor *>(first.data()));
    QVERIFY(dynamic_cast<KisLegacyTileCompressor *>(second.data()));
    QCOMPARE(legacyConstructions, 2);
    QCOMPARE(currentConstructions, 0);

    KisAbstractTileCompressorSP sharedFirst = first;
    first.clear();
    QCOMPARE(legacyDestructions, 0);

    sharedFirst.clear();
    QCOMPARE(legacyDestructions, 1);

    second.clear();
    QCOMPARE(legacyDestructions, 2);
    QCOMPARE(currentDestructions, 0);
}

void KisTileCompressorFactoryContractTest::versionTwoCreatesDistinctOwnedCurrentCompressors()
{
    resetLifetimeObservations();

    KisAbstractTileCompressorSP first = KisTileCompressorFactory::create(2);
    KisAbstractTileCompressorSP second = KisTileCompressorFactory::create(2);

    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first.data() != second.data());
    QVERIFY(dynamic_cast<KisTileCompressor2 *>(first.data()));
    QVERIFY(dynamic_cast<KisTileCompressor2 *>(second.data()));
    QCOMPARE(currentConstructions, 2);
    QCOMPARE(legacyConstructions, 0);

    KisAbstractTileCompressorSP sharedFirst = first;
    first.clear();
    QCOMPARE(currentDestructions, 0);

    sharedFirst.clear();
    QCOMPARE(currentDestructions, 1);

    second.clear();
    QCOMPARE(currentDestructions, 2);
    QCOMPARE(legacyDestructions, 0);
}

QTEST_GUILESS_MAIN(KisTileCompressorFactoryContractTest)

#include "KisTileCompressorFactoryContractTest.moc"
