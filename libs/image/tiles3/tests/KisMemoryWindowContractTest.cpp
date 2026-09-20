/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/swap/kis_memory_window.h"

#include <QDir>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QTest>

#include <array>
#include <cstring>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

const QLoggingCategory &_41000()
{
    static const QLoggingCategory category("librepaint.test.memory-window");
    return category;
}

class KisMemoryWindowContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void temporaryFileFollowsWindowLifetime();
    void chunkDataOverloadsPreserveBytesAcrossRemapping();
    void chunkHandleOverloadsAddressTheSameStoredBytes();
};

void KisMemoryWindowContractTest::temporaryFileFollowsWindowLifetime()
{
    QTemporaryDir swapDirectory;
    QVERIFY(swapDirectory.isValid());

    QDir directory(swapDirectory.path());
    const QStringList swapFileFilter{QStringLiteral("KRITA_SWAP_FILE_*")};
    QCOMPARE(directory.entryList(swapFileFilter, QDir::Files).size(), 0);

    {
        KisMemoryWindow window(swapDirectory.path());
        QCOMPARE(directory.entryList(swapFileFilter, QDir::Files).size(), 1);
    }

    QCOMPARE(directory.entryList(swapFileFilter, QDir::Files).size(), 0);
}

void KisMemoryWindowContractTest::chunkDataOverloadsPreserveBytesAcrossRemapping()
{
    QTemporaryDir swapDirectory;
    QVERIFY(swapDirectory.isValid());

    KisMemoryWindow window(swapDirectory.path(), 64);
    const KisChunkData firstChunk(0, 8);
    const KisChunkData secondChunk(65, 8);
    const std::array<quint8, 8> firstBytes{0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87};
    const std::array<quint8, 8> secondBytes{0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81};

    quint8 *firstWritePointer = window.getWriteChunkPtr(firstChunk);
    QVERIFY(firstWritePointer);
    std::memcpy(firstWritePointer, firstBytes.data(), firstBytes.size());

    quint8 *secondWritePointer = window.getWriteChunkPtr(secondChunk);
    QVERIFY(secondWritePointer);
    std::memcpy(secondWritePointer, secondBytes.data(), secondBytes.size());

    const quint8 *secondReadPointer = window.getReadChunkPtr(secondChunk);
    QVERIFY(secondReadPointer);
    QCOMPARE(std::memcmp(secondReadPointer, secondBytes.data(), secondBytes.size()), 0);

    const quint8 *firstReadPointer = window.getReadChunkPtr(firstChunk);
    QVERIFY(firstReadPointer);
    QCOMPARE(std::memcmp(firstReadPointer, firstBytes.data(), firstBytes.size()), 0);
}

void KisMemoryWindowContractTest::chunkHandleOverloadsAddressTheSameStoredBytes()
{
    QTemporaryDir swapDirectory;
    QVERIFY(swapDirectory.isValid());

    KisMemoryWindow window(swapDirectory.path(), 64);
    KisChunkDataList chunks;
    chunks.append(KisChunkData(16, 8));
    KisChunk chunk(chunks.begin());
    const std::array<quint8, 8> bytes{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

    quint8 *writePointer = window.getWriteChunkPtr(chunk);
    QVERIFY(writePointer);
    std::memcpy(writePointer, bytes.data(), bytes.size());

    const quint8 *readPointer = window.getReadChunkPtr(chunk);
    QVERIFY(readPointer);
    QCOMPARE(std::memcmp(readPointer, bytes.data(), bytes.size()), 0);
}

QTEST_GUILESS_MAIN(KisMemoryWindowContractTest)

#include "KisMemoryWindowContractTest.moc"
