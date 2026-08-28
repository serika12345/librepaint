/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoEmbeddedResource.h>
#include <KoMD5Generator.h>
#include <KoResourceSignature.h>

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class KoEmbeddedResourceValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructsAndComparesResourceSignatures();
    void formatsResourceSignature();
    void hashesByteArrays();
    void hashesDevices();
    void hashesFiles();
    void representsEmbeddedResourceAndChecksDigest();
};

void KoEmbeddedResourceValueContractTest::constructsAndComparesResourceSignatures()
{
    KoResourceSignature empty;
    QVERIFY(empty.type.isEmpty());
    QVERIFY(empty.md5sum.isEmpty());
    QVERIFY(empty.filename.isEmpty());
    QVERIFY(empty.name.isEmpty());

    const KoResourceSignature signature(
        QStringLiteral("brushes"),
        QStringLiteral("digest"),
        QStringLiteral("brush.gbr"),
        QStringLiteral("Brush"));
    QCOMPARE(signature.type, QStringLiteral("brushes"));
    QCOMPARE(signature.md5sum, QStringLiteral("digest"));
    QCOMPARE(signature.filename, QStringLiteral("brush.gbr"));
    QCOMPARE(signature.name, QStringLiteral("Brush"));

    const KoResourceSignature sameIdentityWithDifferentType(
        QStringLiteral("patterns"),
        signature.md5sum,
        signature.filename,
        signature.name);
    QVERIFY(signature == sameIdentityWithDifferentType);

    KoResourceSignature differentName = signature;
    differentName.name = QStringLiteral("Other Brush");
    QVERIFY(signature != differentName);
}

void KoEmbeddedResourceValueContractTest::formatsResourceSignature()
{
    const KoResourceSignature signature(
        QStringLiteral("brushes"),
        QStringLiteral("digest"),
        QStringLiteral("brush.gbr"),
        QStringLiteral("Brush"));
    QString output;
    {
        QDebug debug(&output);
        debug << signature;
    }

    QCOMPARE(output, QStringLiteral("KoResourceSignature(\"brushes\", \"digest\", \"brush.gbr\", \"Brush\") "));
}

void KoEmbeddedResourceValueContractTest::hashesByteArrays()
{
    QCOMPARE(KoMD5Generator::generateHash(QByteArray()), QString());
    QCOMPARE(
        KoMD5Generator::generateHash(QByteArrayLiteral("abc")),
        QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
}

void KoEmbeddedResourceValueContractTest::hashesDevices()
{
    QBuffer buffer;
    buffer.setData(QByteArrayLiteral("abc"));
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QCOMPARE(
        KoMD5Generator::generateHash(&buffer),
        QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
    QCOMPARE(buffer.pos(), qint64(3));
}

void KoEmbeddedResourceValueContractTest::hashesFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filename = directory.filePath(QStringLiteral("payload.bin"));
    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(QByteArrayLiteral("abc")), qint64(3));
    file.close();

    QCOMPARE(
        KoMD5Generator::generateHash(filename),
        QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
    QCOMPARE(KoMD5Generator::generateHash(directory.filePath(QStringLiteral("missing.bin"))), QString());
}

void KoEmbeddedResourceValueContractTest::representsEmbeddedResourceAndChecksDigest()
{
    const KoEmbeddedResource empty;
    QVERIFY(!empty.isValid());
    QVERIFY(empty.data().isEmpty());
    QVERIFY(empty.signature().type.isEmpty());
    QVERIFY(empty.sanityCheckMd5());

    const QByteArray data = QByteArrayLiteral("embedded-data");
    const KoResourceSignature signature(
        QStringLiteral("patterns"),
        KoMD5Generator::generateHash(data),
        QStringLiteral("pattern.pat"),
        QStringLiteral("Pattern"));
    const KoEmbeddedResource resource(signature, data);
    QVERIFY(resource.isValid());
    QCOMPARE(resource.signature(), signature);
    QCOMPARE(resource.data(), data);
    QVERIFY(resource.sanityCheckMd5());

    KoResourceSignature wrongSignature = signature;
    wrongSignature.md5sum = QStringLiteral("wrong-digest");
    const KoEmbeddedResource resourceWithWrongDigest(wrongSignature, data);
    QVERIFY(!resourceWithWrongDigest.sanityCheckMd5());
}

QTEST_GUILESS_MAIN(KoEmbeddedResourceValueContractTest)

#include "KoEmbeddedResourceValueContractTest.moc"
