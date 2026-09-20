/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QBuffer>
#include <QTest>

#include <KoStore.h>
#include <kis_store_paintdevice_writer.h>

#include <memory>

class KisStorePaintDeviceWriterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writesByteArraysAndRawRanges();
    void rejectsWritesWithoutAnOpenStoreEntry();
};

void KisStorePaintDeviceWriterTest::writesByteArraysAndRawRanges()
{
    QByteArray archive;
    QBuffer writeBuffer(&archive);
    {
        std::unique_ptr<KoStore> store(
            KoStore::createStore(&writeBuffer, KoStore::Write, QByteArray(), KoStore::Zip, false));
        QVERIFY(store);
        QVERIFY(!store->bad());
        QVERIFY(store->open(QStringLiteral("frames/pixels")));

        KisStorePaintDeviceWriter writer(store.get());
        KisPaintDeviceWriter *interface = &writer;
        QCOMPARE(writer.m_store, store.get());
        QVERIFY(interface->write(QByteArrayLiteral("alpha")));
        QVERIFY(interface->write("bravo-tail", 5));

        QVERIFY(store->close());
        QVERIFY(store->finalize());
    }

    QBuffer readBuffer(&archive);
    std::unique_ptr<KoStore> reader(
        KoStore::createStore(&readBuffer, KoStore::Read, QByteArray(), KoStore::Zip, false));
    QVERIFY(reader);
    QVERIFY(!reader->bad());
    QVERIFY(reader->open(QStringLiteral("frames/pixels")));
    QCOMPARE(reader->read(reader->size()), QByteArrayLiteral("alphabravo"));
    QVERIFY(reader->close());
}

void KisStorePaintDeviceWriterTest::rejectsWritesWithoutAnOpenStoreEntry()
{
    QByteArray archive;
    QBuffer buffer(&archive);
    std::unique_ptr<KoStore> store(KoStore::createStore(&buffer, KoStore::Write, QByteArray(), KoStore::Zip, false));
    QVERIFY(store);
    QVERIFY(!store->bad());

    std::unique_ptr<KisPaintDeviceWriter> writer = std::make_unique<KisStorePaintDeviceWriter>(store.get());
    QVERIFY(!writer->write(QByteArrayLiteral("not written")));
    QVERIFY(!writer->write("not written", 11));
}

QTEST_GUILESS_MAIN(KisStorePaintDeviceWriterTest)

#include "kis_store_paintdevice_writer_test.moc"
