/*
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "psd_header_test.h"

#include <QBuffer>
#include <QDebug>
#include <QTest>

#include <psd_header.h>

namespace
{
PSDHeader makeValidHeader(quint16 version = 1)
{
    PSDHeader header;
    header.signature = QStringLiteral("8BPS");
    header.version = version;
    header.nChannels = 3;
    header.width = 200;
    header.height = 100;
    header.channelDepth = 16;
    header.colormode = RGB;
    return header;
}

QByteArray canonicalHeaderBytes()
{
    return QByteArray::fromHex("384250530001000000000000000300000064000000c800100003");
}
} // namespace

void PSDHeaderTest::defaultsExposeInvalidEmptyHeader()
{
    PSDHeader header;

    QCOMPARE(header.signature, QString());
    QCOMPARE(header.version, quint16(0));
    QCOMPARE(header.nChannels, quint16(0));
    QCOMPARE(header.height, quint32(0));
    QCOMPARE(header.width, quint32(0));
    QCOMPARE(header.channelDepth, quint16(0));
    QCOMPARE(header.colormode, COLORMODE_UNKNOWN);
    QCOMPARE(header.byteOrder, psd_byte_order::psdBigEndian);
    QCOMPARE(header.tiffStyleLayerBlock, false);
    QCOMPARE(header.error, QString());

    QVERIFY(!header.valid());
    QCOMPARE(header.error, QStringLiteral("Not a PhotoShop document. Signature is: "));
}

void PSDHeaderTest::validationEnforcesFormatSpecificRanges()
{
    PSDHeader header = makeValidHeader();
    QVERIFY(header.valid());

    header.width = 30001;
    QVERIFY(!header.valid());
    QCOMPARE(header.error, QStringLiteral("Width out of range: 30001"));

    PSDHeader largeHeader = makeValidHeader(2);
    largeHeader.width = 300000;
    largeHeader.height = 300000;
    QVERIFY(largeHeader.valid());

    PSDHeader channels = makeValidHeader();
    channels.nChannels = 57;
    QVERIFY(!channels.valid());
    QCOMPARE(channels.error, QStringLiteral("Channel count out of range: 57"));

    PSDHeader depth = makeValidHeader();
    depth.channelDepth = 32;
    QVERIFY(!depth.valid());
    QCOMPARE(depth.error, QStringLiteral("Channel depth incorrect: 32"));

    PSDHeader colorMode = makeValidHeader();
    colorMode.colormode = COLORMODE_UNKNOWN;
    QVERIFY(!colorMode.valid());
    QCOMPARE(colorMode.error, QStringLiteral("Colormode is out of range: 9000"));
}

void PSDHeaderTest::readDecodesBigEndianHeaderAndRejectsTruncation()
{
    QByteArray bytes = canonicalHeaderBytes();
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    PSDHeader header;
    QVERIFY(header.read(buffer));
    QCOMPARE(header.signature, QStringLiteral("8BPS"));
    QCOMPARE(header.version, quint16(1));
    QCOMPARE(header.nChannels, quint16(3));
    QCOMPARE(header.height, quint32(100));
    QCOMPARE(header.width, quint32(200));
    QCOMPARE(header.channelDepth, quint16(16));
    QCOMPARE(header.colormode, RGB);
    QCOMPARE(header.byteOrder, psd_byte_order::psdBigEndian);
    QCOMPARE(header.tiffStyleLayerBlock, false);
    QCOMPARE(header.error, QString());

    QByteArray truncatedBytes = bytes.first(bytes.size() - 1);
    QBuffer truncatedBuffer(&truncatedBytes);
    QVERIFY(truncatedBuffer.open(QIODevice::ReadOnly));
    PSDHeader truncatedHeader;
    QVERIFY(!truncatedHeader.read(truncatedBuffer));
    QCOMPARE(truncatedHeader.error, QStringLiteral("Could not read header: not enough bytes"));
}

void PSDHeaderTest::writeProducesCanonicalHeaderBytes()
{
    PSDHeader header = makeValidHeader();
    QByteArray bytes;
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));

    QVERIFY(header.write(buffer));
    QCOMPARE(bytes, canonicalHeaderBytes());

    PSDHeader invalidHeader;
    QByteArray unchanged("sentinel");
    QBuffer invalidBuffer(&unchanged);
    QVERIFY(invalidBuffer.open(QIODevice::WriteOnly));
    QVERIFY(!invalidHeader.write(invalidBuffer));
    QCOMPARE(unchanged, QByteArray("sentinel"));
}

void PSDHeaderTest::debugOutputDescribesValidatedHeader()
{
    PSDHeader header = makeValidHeader();
    QString output;
    {
        QDebug debug(&output);
        debug << header;
    }

    QVERIFY(output.contains(QStringLiteral("valid: true")));
    QVERIFY(output.contains(QStringLiteral("signature: \"8BPS\"")));
    QVERIFY(output.contains(QStringLiteral("version:1")));
    QVERIFY(output.contains(QStringLiteral("number of channels: 3")));
    QVERIFY(output.contains(QStringLiteral("height: 100")));
    QVERIFY(output.contains(QStringLiteral("width: 200")));
    QVERIFY(output.contains(QStringLiteral("channel depth: 16")));
    QVERIFY(output.contains(QStringLiteral("color mode: RGB")));
}

QTEST_GUILESS_MAIN(PSDHeaderTest)
