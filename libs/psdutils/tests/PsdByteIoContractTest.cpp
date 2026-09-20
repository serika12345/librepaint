/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <psd_utils.h>

#include <QBuffer>
#include <QTest>

#include <cmath>

namespace
{

template<typename T>
QByteArray writeRaw(bool (*writer)(QIODevice &, const T &), T value)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !writer(buffer, value)) {
        return {};
    }
    return bytes;
}

template<typename T>
QByteArray writeSized(bool (*writer)(QIODevice &, T &), T value)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !writer(buffer, value)) {
        return {};
    }
    return bytes;
}

template<typename T>
bool readRaw(bool (*reader)(QIODevice &, T &), const QByteArray &bytes, T *value)
{
    QBuffer buffer;
    buffer.setData(bytes);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return false;
    }
    return reader(buffer, *value);
}

} // namespace

class PsdByteIoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rawIntegerWritesUseRequestedByteOrder();
    void arithmeticWritesDispatchBySizeAndReportFailure();
    void stringsPaddingAndPascalStringsPreserveLayout();
    void rawIntegerReadsUseRequestedByteOrder();
    void arithmeticReadsDispatchBySizeAndPreserveOutputOnFailure();
    void byteAndStringReadersConsumeTheirDeclaredLayout();
    void blendModeReaderValidatesSignatureAndKeyLength();
    void fixedPointValuesRoundTripInBothByteOrders();
};

void PsdByteIoContractTest::rawIntegerWritesUseRequestedByteOrder()
{
    QCOMPARE(writeRaw<quint8>(psdwriteBE, 0x12U), QByteArray::fromHex("12"));
    QCOMPARE(writeRaw<quint8>(psdwriteLE, 0x12U), QByteArray::fromHex("12"));
    QCOMPARE(writeRaw<quint16>(psdwriteBE, 0x1234U), QByteArray::fromHex("1234"));
    QCOMPARE(writeRaw<quint16>(psdwriteLE, 0x1234U), QByteArray::fromHex("3412"));
    QCOMPARE(writeRaw<quint32>(psdwriteBE, 0x12345678U), QByteArray::fromHex("12345678"));
    QCOMPARE(writeRaw<quint32>(psdwriteLE, 0x12345678U), QByteArray::fromHex("78563412"));
    QCOMPARE(writeRaw<quint64>(psdwriteBE, UINT64_C(0x0123456789abcdef)), QByteArray::fromHex("0123456789abcdef"));
    QCOMPARE(writeRaw<quint64>(psdwriteLE, UINT64_C(0x0123456789abcdef)), QByteArray::fromHex("efcdab8967452301"));
}

void PsdByteIoContractTest::arithmeticWritesDispatchBySizeAndReportFailure()
{
    QCOMPARE(writeSized<qint8>(psdwriteBE<qint8>, qint8(-2)), QByteArray::fromHex("fe"));
    QCOMPARE(writeSized<qint8>(psdwriteLE<qint8>, qint8(-2)), QByteArray::fromHex("fe"));
    QCOMPARE(writeSized<qint16>(psdwriteBE<qint16>, qint16(0x1234)), QByteArray::fromHex("1234"));
    QCOMPARE(writeSized<qint16>(psdwriteLE<qint16>, qint16(0x1234)), QByteArray::fromHex("3412"));
    QCOMPARE(writeSized<qint32>(psdwriteBE<qint32>, qint32(0x12345678)), QByteArray::fromHex("12345678"));
    QCOMPARE(writeSized<qint32>(psdwriteLE<qint32>, qint32(0x12345678)), QByteArray::fromHex("78563412"));
    QCOMPARE(writeSized<qint64>(psdwriteBE<qint64>, qint64(INT64_C(0x0123456789abcdef))),
             QByteArray::fromHex("0123456789abcdef"));
    QCOMPARE(writeSized<qint64>(psdwriteLE<qint64>, qint64(INT64_C(0x0123456789abcdef))),
             QByteArray::fromHex("efcdab8967452301"));

    QByteArray bytes;
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));

    QVERIFY(psdwrite<psd_byte_order::psdBigEndian>(buffer, qint8(-2)));
    QVERIFY(psdwrite<psd_byte_order::psdLittleEndian>(buffer, qint16(0x1234)));
    QVERIFY(psdwrite<psd_byte_order::psdBigEndian>(buffer, qint32(0x12345678)));
    QVERIFY(psdwrite<psd_byte_order::psdLittleEndian>(buffer, qint64(INT64_C(0x0123456789abcdef))));
    QCOMPARE(bytes, QByteArray::fromHex("fe341212345678efcdab8967452301"));

    QBuffer closed;
    QVERIFY(!psdwriteBE(closed, quint32(1)));
    QVERIFY(!psdwriteLE(closed, quint64(1)));
}

void PsdByteIoContractTest::stringsPaddingAndPascalStringsPreserveLayout()
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));

    QVERIFY(psdwrite(buffer, QStringLiteral("8BPS")));
    QVERIFY(psdpad(buffer, 2));
    QVERIFY(psdwrite_pascalstring(buffer, QStringLiteral("abc")));
    QVERIFY(psdwrite_pascalstring(buffer, QStringLiteral("xy"), 4));
    QCOMPARE(bytes, QByteArray::fromHex("384250530000036162630002787900"));

    QByteArray nullBytes;
    QBuffer nullBuffer(&nullBytes);
    QVERIFY(nullBuffer.open(QIODevice::WriteOnly));
    QVERIFY(psdwrite_pascalstring(nullBuffer, QString()));
    QCOMPARE(nullBytes, QByteArray(2, '\0'));

    QBuffer closed;
    QVERIFY(!psdwrite(closed, QStringLiteral("x")));
    QVERIFY(!psdpad(closed, 1));
}

void PsdByteIoContractTest::rawIntegerReadsUseRequestedByteOrder()
{
    quint8 value8 = 0;
    quint16 value16 = 0;
    quint32 value32 = 0;
    quint64 value64 = 0;

    QVERIFY(readRaw<quint8>(psdreadBE, QByteArray::fromHex("12"), &value8));
    QCOMPARE(value8, quint8(0x12));
    QVERIFY(readRaw<quint8>(psdreadLE, QByteArray::fromHex("34"), &value8));
    QCOMPARE(value8, quint8(0x34));
    QVERIFY(readRaw<quint16>(psdreadBE, QByteArray::fromHex("1234"), &value16));
    QCOMPARE(value16, quint16(0x1234));
    QVERIFY(readRaw<quint16>(psdreadLE, QByteArray::fromHex("3412"), &value16));
    QCOMPARE(value16, quint16(0x1234));
    QVERIFY(readRaw<quint32>(psdreadBE, QByteArray::fromHex("12345678"), &value32));
    QCOMPARE(value32, quint32(0x12345678));
    QVERIFY(readRaw<quint32>(psdreadLE, QByteArray::fromHex("78563412"), &value32));
    QCOMPARE(value32, quint32(0x12345678));
    QVERIFY(readRaw<quint64>(psdreadBE, QByteArray::fromHex("0123456789abcdef"), &value64));
    QCOMPARE(value64, quint64(UINT64_C(0x0123456789abcdef)));
    QVERIFY(readRaw<quint64>(psdreadLE, QByteArray::fromHex("efcdab8967452301"), &value64));
    QCOMPARE(value64, quint64(UINT64_C(0x0123456789abcdef)));
}

void PsdByteIoContractTest::arithmeticReadsDispatchBySizeAndPreserveOutputOnFailure()
{
    qint8 sized8 = 0;
    qint16 sized16 = 0;
    qint32 sized32 = 0;
    qint64 sized64 = 0;
    QVERIFY(readRaw<qint8>(psdreadBE<qint8>, QByteArray::fromHex("fe"), &sized8));
    QCOMPARE(sized8, qint8(-2));
    QVERIFY(readRaw<qint8>(psdreadLE<qint8>, QByteArray::fromHex("fe"), &sized8));
    QCOMPARE(sized8, qint8(-2));
    QVERIFY(readRaw<qint16>(psdreadBE<qint16>, QByteArray::fromHex("1234"), &sized16));
    QCOMPARE(sized16, qint16(0x1234));
    QVERIFY(readRaw<qint16>(psdreadLE<qint16>, QByteArray::fromHex("3412"), &sized16));
    QCOMPARE(sized16, qint16(0x1234));
    QVERIFY(readRaw<qint32>(psdreadBE<qint32>, QByteArray::fromHex("12345678"), &sized32));
    QCOMPARE(sized32, qint32(0x12345678));
    QVERIFY(readRaw<qint32>(psdreadLE<qint32>, QByteArray::fromHex("78563412"), &sized32));
    QCOMPARE(sized32, qint32(0x12345678));
    QVERIFY(readRaw<qint64>(psdreadBE<qint64>, QByteArray::fromHex("0123456789abcdef"), &sized64));
    QCOMPARE(sized64, qint64(INT64_C(0x0123456789abcdef)));
    QVERIFY(readRaw<qint64>(psdreadLE<qint64>, QByteArray::fromHex("efcdab8967452301"), &sized64));
    QCOMPARE(sized64, qint64(INT64_C(0x0123456789abcdef)));

    QBuffer buffer;
    buffer.setData(QByteArray::fromHex("fe341212345678efcdab8967452301"));
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    qint8 value8 = 0;
    qint16 value16 = 0;
    qint32 value32 = 0;
    qint64 value64 = 0;
    QVERIFY(psdread<psd_byte_order::psdBigEndian>(buffer, value8));
    QVERIFY(psdread<psd_byte_order::psdLittleEndian>(buffer, value16));
    QVERIFY(psdread<psd_byte_order::psdBigEndian>(buffer, value32));
    QVERIFY(psdread<psd_byte_order::psdLittleEndian>(buffer, value64));
    QCOMPARE(value8, qint8(-2));
    QCOMPARE(value16, qint16(0x1234));
    QCOMPARE(value32, qint32(0x12345678));
    QCOMPARE(value64, qint64(INT64_C(0x0123456789abcdef)));

    QBuffer shortBuffer;
    shortBuffer.setData(QByteArray::fromHex("1234"));
    QVERIFY(shortBuffer.open(QIODevice::ReadOnly));
    quint32 unchanged = 99;
    QVERIFY(!psdreadBE(shortBuffer, unchanged));
    QCOMPARE(unchanged, quint32(99));
}

void PsdByteIoContractTest::byteAndStringReadersConsumeTheirDeclaredLayout()
{
    QBuffer bytesBuffer;
    bytesBuffer.setData(QByteArray("abcd"));
    QVERIFY(bytesBuffer.open(QIODevice::ReadOnly));
    QCOMPARE(psdreadBytes(bytesBuffer, 4), QByteArray("abcd"));

    QBuffer reversedBuffer;
    reversedBuffer.setData(QByteArray("abcd"));
    QVERIFY(reversedBuffer.open(QIODevice::ReadOnly));
    QCOMPARE(psdreadBytes<psd_byte_order::psdLittleEndian>(reversedBuffer, 4), QByteArray("dcba"));

    QBuffer pascalBuffer;
    pascalBuffer.setData(QByteArray::fromHex("03616263027879"));
    QVERIFY(pascalBuffer.open(QIODevice::ReadOnly));
    QString pascal = QStringLiteral("prefix-");
    QVERIFY(psdread_pascalstring(pascalBuffer, pascal, 4));
    QCOMPARE(pascal, QStringLiteral("prefix-abc"));
    QVERIFY(psdread_pascalstring(pascalBuffer, pascal, 1));
    QCOMPARE(pascal, QStringLiteral("prefix-abcxy"));

    QByteArray unicodeBytes;
    QBuffer unicodeWriter(&unicodeBytes);
    QVERIFY(unicodeWriter.open(QIODevice::WriteOnly));
    QVERIFY(psdwrite(unicodeWriter, quint32(3)));
    QVERIFY(psdwrite(unicodeWriter, quint16('A')));
    QVERIFY(psdwrite(unicodeWriter, quint16('B')));
    QVERIFY(psdwrite(unicodeWriter, quint16(' ')));
    unicodeWriter.close();

    QBuffer unicodeReader(&unicodeBytes);
    QVERIFY(unicodeReader.open(QIODevice::ReadOnly));
    QString unicode = QStringLiteral("old");
    QVERIFY(psdread_unicodestring(unicodeReader, unicode));
    QCOMPARE(unicode, QStringLiteral("AB"));
}

void PsdByteIoContractTest::blendModeReaderValidatesSignatureAndKeyLength()
{
    QBuffer valid;
    valid.setData(QByteArray("8BIMnorm"));
    QVERIFY(valid.open(QIODevice::ReadOnly));
    QString key;
    QVERIFY(psd_read_blendmode(valid, key));
    QCOMPARE(key, QStringLiteral("norm"));

    QBuffer invalidSignature;
    invalidSignature.setData(QByteArray("XXXXnorm"));
    QVERIFY(invalidSignature.open(QIODevice::ReadOnly));
    key = QStringLiteral("old");
    QVERIFY(!psd_read_blendmode(invalidSignature, key));
    QCOMPARE(key, QStringLiteral("old"));

    QBuffer shortKey;
    shortKey.setData(QByteArray("8BIMno"));
    QVERIFY(shortKey.open(QIODevice::ReadOnly));
    QVERIFY(!psd_read_blendmode(shortKey, key));
    QCOMPARE(key, QStringLiteral("no"));
}

void PsdByteIoContractTest::fixedPointValuesRoundTripInBothByteOrders()
{
    for (const psd_byte_order order : {psd_byte_order::psdBigEndian, psd_byte_order::psdLittleEndian}) {
        QByteArray bytes;
        QBuffer writer(&bytes);
        QVERIFY(writer.open(QIODevice::WriteOnly));
        if (order == psd_byte_order::psdBigEndian) {
            psdwriteFixedPoint(writer, -2.7);
        } else {
            psdwriteFixedPoint<psd_byte_order::psdLittleEndian>(writer, -2.7);
        }
        QCOMPARE(bytes.size(), 4);

        QBuffer reader(&bytes);
        QVERIFY(reader.open(QIODevice::ReadOnly));
        const double value = order == psd_byte_order::psdBigEndian
            ? psdreadFixedPoint(reader)
            : psdreadFixedPoint<psd_byte_order::psdLittleEndian>(reader);
        QVERIFY(std::fabs(value + 2.7) < 0.001);
    }

    QBuffer empty;
    QVERIFY(empty.open(QIODevice::ReadOnly));
    QCOMPARE(psdreadFixedPoint(empty), 0.0);
}

QTEST_GUILESS_MAIN(PsdByteIoContractTest)

#include "PsdByteIoContractTest.moc"
