/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoStore.h>
#include <KoXmlWriter.h>

#include <QBuffer>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

class TestResourceStorageArchiveContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void zipRoundTrip();
    void directoryRoundTrip();
    void malformedArchiveIsRejected();
    void failedReadPreservesArchiveSession();
    void duplicateWriteIsRejectedWithoutLosingEarlierData();
    void xmlNumericAttributesRemainStable();
};

void TestResourceStorageArchiveContract::zipRoundTrip()
{
    QByteArray archive;
    QBuffer output(&archive);
    std::unique_ptr<KoStore> writer(
        KoStore::createStore(&output, KoStore::Write, {}, KoStore::Zip, false));

    QVERIFY(writer);
    QVERIFY(!writer->bad());
    QVERIFY(writer->open(QStringLiteral("content/data.bin")));
    QCOMPARE(writer->write(QByteArrayLiteral("resource-data")), qint64(13));
    QVERIFY(writer->close());
    QVERIFY(writer->finalize());
    writer.reset();

    QBuffer input(&archive);
    std::unique_ptr<KoStore> reader(
        KoStore::createStore(&input, KoStore::Read, {}, KoStore::Zip, false));

    QVERIFY(reader);
    QVERIFY(!reader->bad());
    QVERIFY(reader->open(QStringLiteral("content/data.bin")));
    QCOMPARE(reader->read(13), QByteArrayLiteral("resource-data"));
    QVERIFY(reader->close());
}

void TestResourceStorageArchiveContract::directoryRoundTrip()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString archivePath = temporaryDirectory.filePath(QStringLiteral("bundle"));

    {
        std::unique_ptr<KoStore> writer(KoStore::createStore(
            archivePath, KoStore::Write, {}, KoStore::Directory, false));
        QVERIFY(writer);
        QVERIFY(!writer->bad());
        QVERIFY(writer->open(QStringLiteral("nested/resource.txt")));
        QCOMPARE(writer->write(QByteArrayLiteral("directory-data")), qint64(14));
        QVERIFY(writer->close());
        QVERIFY(writer->finalize());
    }

    std::unique_ptr<KoStore> reader(KoStore::createStore(
        archivePath, KoStore::Read, {}, KoStore::Directory, false));
    QVERIFY(reader);
    QVERIFY(!reader->bad());
    QVERIFY(reader->open(QStringLiteral("nested/resource.txt")));
    QCOMPARE(reader->read(14), QByteArrayLiteral("directory-data"));
    QVERIFY(reader->close());
}

void TestResourceStorageArchiveContract::malformedArchiveIsRejected()
{
    QByteArray malformedArchive = QByteArrayLiteral("not-a-zip-archive");
    QBuffer input(&malformedArchive);
    std::unique_ptr<KoStore> store(
        KoStore::createStore(&input, KoStore::Read, {}, KoStore::Auto, false));

    QVERIFY(store);
    QVERIFY(store->bad());
    QVERIFY(!store->open(QStringLiteral("content.xml")));
}

void TestResourceStorageArchiveContract::failedReadPreservesArchiveSession()
{
    QByteArray archive;
    QBuffer output(&archive);
    {
        std::unique_ptr<KoStore> writer(
            KoStore::createStore(&output, KoStore::Write, {}, KoStore::Zip, false));
        QVERIFY(writer);
        QVERIFY(!writer->bad());
        QVERIFY(writer->open(QStringLiteral("present")));
        QCOMPARE(writer->write(QByteArrayLiteral("stable")), qint64(6));
        QVERIFY(writer->close());
        QVERIFY(writer->finalize());
    }

    QBuffer input(&archive);
    std::unique_ptr<KoStore> reader(
        KoStore::createStore(&input, KoStore::Read, {}, KoStore::Zip, false));
    QVERIFY(reader);
    QVERIFY(!reader->bad());
    QVERIFY(!reader->open(QStringLiteral("missing")));
    QVERIFY(!reader->isOpen());
    QVERIFY(reader->open(QStringLiteral("present")));
    QCOMPARE(reader->read(6), QByteArrayLiteral("stable"));
    QVERIFY(reader->close());
}

void TestResourceStorageArchiveContract::duplicateWriteIsRejectedWithoutLosingEarlierData()
{
    QByteArray archive;
    QBuffer output(&archive);
    {
        std::unique_ptr<KoStore> writer(
            KoStore::createStore(&output, KoStore::Write, {}, KoStore::Zip, false));
        QVERIFY(writer);
        QVERIFY(!writer->bad());
        QVERIFY(writer->open(QStringLiteral("resource")));
        QCOMPARE(writer->write(QByteArrayLiteral("first")), qint64(5));
        QVERIFY(writer->close());
        QVERIFY(!writer->open(QStringLiteral("resource")));
        QVERIFY(!writer->isOpen());
        QVERIFY(writer->open(QStringLiteral("second")));
        QCOMPARE(writer->write(QByteArrayLiteral("next")), qint64(4));
        QVERIFY(writer->close());
        QVERIFY(writer->finalize());
    }

    QBuffer input(&archive);
    std::unique_ptr<KoStore> reader(
        KoStore::createStore(&input, KoStore::Read, {}, KoStore::Zip, false));
    QVERIFY(reader);
    QVERIFY(!reader->bad());
    QVERIFY(reader->open(QStringLiteral("resource")));
    QCOMPARE(reader->read(5), QByteArrayLiteral("first"));
    QVERIFY(reader->close());
    QVERIFY(reader->open(QStringLiteral("second")));
    QCOMPARE(reader->read(4), QByteArrayLiteral("next"));
    QVERIFY(reader->close());
}

void TestResourceStorageArchiveContract::xmlNumericAttributesRemainStable()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);
    writer.startDocument("numbers");
    writer.startElement("numbers");
    writer.addAttribute("double", 1234.56789012345);
    writer.addAttribute("float", 1.2345678f);
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<numbers double=\"1234.56789012345\" float=\"1.23457\"/>\n"));
}

QTEST_GUILESS_MAIN(TestResourceStorageArchiveContract)

#include "TestResourceStorageArchiveContract.moc"
