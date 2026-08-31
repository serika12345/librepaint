/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <asl/kis_asl_writer_utils.h>

#include <QBuffer>
#include <QTest>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion at %s:%d: %s", file, line, assertion);
}

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion at %s:%d: %s", file, line, assertion);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion at %s:%d: %s", file, line, assertion);
}

class KisAslWriterUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writeExceptionPreservesDiagnostic();
    void alignmentRoundsUpAtPowerOfTwoBoundaries();
    void rectanglesUseRequestedByteOrder();
    void stringsPreserveDeclaredBinaryLayouts();
    void offsetPusherWritesSizesPaddingAndRestoresPosition();
};

void KisAslWriterUtilsContractTest::writeExceptionPreservesDiagnostic()
{
    const KisAslWriterUtils::ASLWriteException exception(QStringLiteral("write failed: tag"));
    QCOMPARE(QString::fromLatin1(exception.what()), QStringLiteral("write failed: tag"));
}

void KisAslWriterUtilsContractTest::alignmentRoundsUpAtPowerOfTwoBoundaries()
{
    QCOMPARE(KisAslWriterUtils::alignOffsetCeil(0, 4), qint64(0));
    QCOMPARE(KisAslWriterUtils::alignOffsetCeil(4, 4), qint64(4));
    QCOMPARE(KisAslWriterUtils::alignOffsetCeil(5, 4), qint64(8));
    QCOMPARE(KisAslWriterUtils::alignOffsetCeil(15, 8), qint64(16));
}

void KisAslWriterUtilsContractTest::rectanglesUseRequestedByteOrder()
{
    const QRect rect(0x10, 0x20, 0x30, 0x40);

    QByteArray bigEndian;
    QBuffer bigEndianBuffer(&bigEndian);
    QVERIFY(bigEndianBuffer.open(QIODevice::WriteOnly));
    KisAslWriterUtils::writeRect<psd_byte_order::psdBigEndian>(rect, bigEndianBuffer);
    QCOMPARE(bigEndian, QByteArray::fromHex("00000020000000100000006000000040"));

    QByteArray littleEndian;
    QBuffer littleEndianBuffer(&littleEndian);
    QVERIFY(littleEndianBuffer.open(QIODevice::WriteOnly));
    KisAslWriterUtils::writeRect<psd_byte_order::psdLittleEndian>(rect, littleEndianBuffer);
    QCOMPARE(littleEndian, QByteArray::fromHex("20000000100000006000000040000000"));
}

void KisAslWriterUtilsContractTest::stringsPreserveDeclaredBinaryLayouts()
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));

    KisAslWriterUtils::writeUnicodeString<psd_byte_order::psdBigEndian>(QString::fromUtf8("A\xCE\xA9"), buffer);
    KisAslWriterUtils::writeVarString<psd_byte_order::psdBigEndian>(QStringLiteral("abc"), buffer);
    KisAslWriterUtils::writeVarString<psd_byte_order::psdBigEndian>(QStringLiteral("8BIM"), buffer);
    KisAslWriterUtils::writePascalString<psd_byte_order::psdBigEndian>(QStringLiteral("xy"), buffer);
    KisAslWriterUtils::writeFixedString<psd_byte_order::psdBigEndian>(QStringLiteral("8BIM"), buffer);
    KisAslWriterUtils::writeFixedString<psd_byte_order::psdLittleEndian>(QStringLiteral("8BIM"), buffer);

    QCOMPARE(bytes, QByteArray::fromHex("00000003004103a9000000000003616263000000003842494d0278793842494d4d494238"));
}

void KisAslWriterUtilsContractTest::offsetPusherWritesSizesPaddingAndRestoresPosition()
{
    QByteArray internalBytes;
    QBuffer internalBuffer(&internalBytes);
    QVERIFY(internalBuffer.open(QIODevice::ReadWrite));
    {
        KisAslWriterUtils::OffsetStreamPusher<quint32, psd_byte_order::psdBigEndian> pusher(internalBuffer, 4);
        QCOMPARE(internalBuffer.write("abc", 3), qint64(3));
    }
    QCOMPARE(internalBytes, QByteArray::fromHex("0000000461626300"));
    QCOMPARE(internalBuffer.pos(), qint64(8));

    QByteArray externalBytes(4, '\0');
    QBuffer externalBuffer(&externalBytes);
    QVERIFY(externalBuffer.open(QIODevice::ReadWrite));
    QVERIFY(externalBuffer.seek(4));
    {
        KisAslWriterUtils::OffsetStreamPusher<quint32, psd_byte_order::psdBigEndian> pusher(externalBuffer, 4, 0);
        QCOMPARE(externalBuffer.write("xyz", 3), qint64(3));
    }
    QCOMPARE(externalBytes, QByteArray::fromHex("0000000478797a00"));
    QCOMPARE(externalBuffer.pos(), qint64(8));
}

QTEST_GUILESS_MAIN(KisAslWriterUtilsContractTest)

#include "KisAslWriterUtilsContractTest.moc"
