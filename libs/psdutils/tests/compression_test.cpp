/*
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "compression_test.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTest>

#include <compression.h>

namespace
{
constexpr auto invalidOperationVariable = "LIBREPAINT_COMPRESSION_INVALID_OPERATION";

QByteArray binaryPayload()
{
    return QByteArray::fromHex("00ff0102030004057f80fe10203000");
}

void runInvalidCompressionOperation(const QByteArray &operation)
{
    if (operation == "compress") {
        Compression::compress(QByteArray("data"), psd_compression_type::Unknown);
    } else if (operation == "uncompress") {
        Compression::uncompress(4, QByteArray("data"), psd_compression_type::Unknown);
    }
}
} // namespace

void CompressionTest::emptyInputProducesEmptyOutput()
{
    const psd_compression_type modes[] = {
        psd_compression_type::Uncompressed,
        psd_compression_type::RLE,
        psd_compression_type::ZIP,
        psd_compression_type::ZIPWithPrediction,
        psd_compression_type::Unknown,
    };

    for (const psd_compression_type mode : modes) {
        QVERIFY(Compression::compress({}, mode, 4, 8).isEmpty());
        QVERIFY(Compression::uncompress(4, {}, mode, 4, 8).isEmpty());
    }
}

void CompressionTest::uncompressedModePreservesBinaryInput()
{
    const QByteArray input = binaryPayload();

    QCOMPARE(Compression::compress(input, psd_compression_type::Uncompressed), input);
    QCOMPARE(Compression::uncompress(1, input, psd_compression_type::Uncompressed), input);
}

void CompressionTest::rleUsesPackBitsAndRoundTripsBinaryInput()
{
    const QByteArray packBitsInput("AAABCD");
    QCOMPARE(Compression::compress(packBitsInput, psd_compression_type::RLE), QByteArray::fromHex("fe410142430044"));

    const QByteArray input = QByteArray(130, '\x7f') + binaryPayload() + QByteArray(129, '\0');
    const QByteArray compressed = Compression::compress(input, psd_compression_type::RLE);
    QVERIFY(compressed.size() > 0);
    QCOMPARE(Compression::uncompress(input.size(), compressed, psd_compression_type::RLE), input);
}

void CompressionTest::rleRejectsTruncatedInputAndPadsShortOutput()
{
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Input buffer exhausted in replicate of.*"));
    QVERIFY(Compression::uncompress(3, QByteArray::fromHex("024142"), psd_compression_type::RLE).isEmpty());

    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Packbits decode - unpack left.*"));
    QCOMPARE(Compression::uncompress(4, QByteArray::fromHex("0041"), psd_compression_type::RLE), QByteArray::fromHex("41000000"));
}

void CompressionTest::zipRoundTripsBinaryInputAndRejectsMalformedData()
{
    const QByteArray input = binaryPayload() + QByteArray(257, '\x5a');
    const QByteArray compressed = Compression::compress(input, psd_compression_type::ZIP);
    QVERIFY(compressed.size() > 0);
    QCOMPARE(Compression::uncompress(input.size(), compressed, psd_compression_type::ZIP), input);

    QVERIFY(Compression::uncompress(32, QByteArray("not a zlib stream"), psd_compression_type::ZIP).isEmpty());
}

void CompressionTest::zipPredictionPreservesCurrentEightBitTransform()
{
    const QByteArray input = QByteArray::fromHex("000102ff10204080214365a7");
    const int rowSize = 4;
    const QByteArray compressed = Compression::compress(input, psd_compression_type::ZIPWithPrediction, rowSize, 8);

    QVERIFY(compressed.size() > 0);
    QCOMPARE(Compression::uncompress(input.size(), compressed, psd_compression_type::ZIP), QByteArray::fromHex("000101fe1010305021224364"));
    QCOMPARE(Compression::uncompress(input.size(), compressed, psd_compression_type::ZIPWithPrediction, rowSize, 8),
             QByteArray::fromHex("00010200102050a0214386ea"));
}

void CompressionTest::zipPredictionPreservesCurrentSixteenBitTransform()
{
    const QByteArray input = QByteArray::fromHex("00100020ff001234456789abcdef11223344ffff");
    const int rowSize = 5;
    const QByteArray compressed = Compression::compress(input, psd_compression_type::ZIPWithPrediction, rowSize, 16);

    QVERIFY(compressed.size() > 0);
    QCOMPARE(Compression::uncompress(input.size(), compressed, psd_compression_type::ZIP),
             QByteArray::fromHex("00100010fff01244332389ab4344cede64669a99"));
    QCOMPARE(Compression::uncompress(input.size(), compressed, psd_compression_type::ZIPWithPrediction, rowSize, 16),
             QByteArray::fromHex("0010002000101254457789abccef9bcd00339acc"));
}

void CompressionTest::zipPredictionRejectsUnsupportedDepth()
{
    const QByteArray input = binaryPayload();

    QTest::ignoreMessage(QtCriticalMsg, "Unsupported bit depth for prediction");
    QVERIFY(Compression::compress(input, psd_compression_type::ZIPWithPrediction, 4, 32).isEmpty());

    const QByteArray compressed = Compression::compress(input, psd_compression_type::ZIP);
    QTest::ignoreMessage(QtCriticalMsg, "Unsupported bit depth for prediction");
    QVERIFY(Compression::uncompress(input.size(), compressed, psd_compression_type::ZIPWithPrediction, 4, 32).isEmpty());
}

void CompressionTest::unknownCompressionTypeIsFatal_data()
{
    QTest::addColumn<QByteArray>("operation");
    QTest::addColumn<QByteArray>("diagnostic");

    QTest::newRow("compress") << QByteArray("compress") << QByteArray("Cannot compress layer data: invalid compression type");
    QTest::newRow("uncompress") << QByteArray("uncompress") << QByteArray("Cannot uncompress layer data: invalid compression type");
}

void CompressionTest::unknownCompressionTypeIsFatal()
{
    QFETCH(QByteArray, operation);
    QFETCH(QByteArray, diagnostic);

    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QString::fromLatin1(invalidOperationVariable), QString::fromLatin1(operation));
    process.setProcessEnvironment(environment);
    process.start(QCoreApplication::applicationFilePath());

    QVERIFY(process.waitForFinished());
    QVERIFY(process.exitStatus() == QProcess::CrashExit || process.exitCode() != 0);
    QVERIFY2(process.readAllStandardError().contains(diagnostic), diagnostic.constData());
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    const QByteArray invalidOperation = qgetenv(invalidOperationVariable);
    if (!invalidOperation.isEmpty()) {
        runInvalidCompressionOperation(invalidOperation);
        return 0;
    }

    CompressionTest test;
    return QTest::qExec(&test, argc, argv);
}
