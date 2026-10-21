/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <kistest.h>
#include <kis_meta_data_backend_registry.h>
#include <kis_meta_data_entry.h>
#include <kis_meta_data_io_backend.h>
#include <kis_meta_data_schema.h>
#include <kis_meta_data_schema_registry.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_value.h>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>

class KisMetadataExportTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        TestUtil::initializeResourcePathsForTests();
    }
    void equivalentMetadataKeepsEncodedBytes_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("jpegHeader");
        QTest::newRow("iptc-plain") << QStringLiteral("iptc") << false;
        QTest::newRow("iptc-jpeg") << QStringLiteral("iptc") << true;
        QTest::newRow("xmp-plain") << QStringLiteral("xmp") << false;
        QTest::newRow("xmp-jpeg") << QStringLiteral("xmp") << true;
    }
    void equivalentMetadataKeepsEncodedBytes()
    {
        // Image exporters must produce the same file after metadata edits that
        // restore the same fields. Ordered values retain their original sequence.
        using namespace KisMetaData;
        QFETCH(QString, kind);
        QFETCH(bool, jpegHeader);
        auto *backend = KisMetadataBackendRegistry::instance()->get(kind);
        QVERIFY(backend);
        const auto dc = SchemaRegistry::instance()->schemaFromUri(Schema::DublinCoreSchemaUri);
        const auto photoshop = SchemaRegistry::instance()->schemaFromUri(Schema::PhotoshopSchemaUri);
        QVERIFY(dc);
        QVERIFY(photoshop);
        QList<Entry> values{{dc, "title", Value(QStringLiteral("Fixed title"))},
                            {dc, "description", Value(QStringLiteral("Fixed description"))},
                            {dc, "rights", Value(QStringLiteral("CC0"))}};
        const QStringList names{QStringLiteral("City"), QStringLiteral("Country"), QStringLiteral("AuthorsPosition"),
            QStringLiteral("CaptionWriter"), QStringLiteral("Headline"), QStringLiteral("Instructions"),
            QStringLiteral("JobID"), QStringLiteral("Credit"), QStringLiteral("State"), QStringLiteral("Source"),
            QStringLiteral("Category"), QStringLiteral("SupplementalCategory")};
        for (const auto &name : names)
            values.append(Entry(photoshop, name, Value(QString(QStringLiteral("fixed-") + name))));
        if (kind == QStringLiteral("xmp")) {
            values.append(Entry(dc, QStringLiteral("creator"), Value({Value(QStringLiteral("First author")),
                Value(QStringLiteral("Second author"))}, Value::OrderedArray)));
        }
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
        QVERIFY(QDir().mkpath(directory));
        const auto name = QString::fromLatin1(QTest::currentDataTag());
        QByteArray expected, actual;
        const auto write = [&](const QString &suffix, const QByteArray &bytes) {
            QFile file(QDir(directory).filePath(name + suffix));
            return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
        };
        const auto preserveFailure = qScopeGuard([&] {
            if (!QTest::currentTestFailed()) return;
            temporary.setAutoRemove(false);
            write(QStringLiteral("-expected.bin"), expected);
            write(QStringLiteral("-actual.bin"), actual);
            qInfo() << "Metadata comparison artifacts retained at" << directory;
        });
        for (int turn = 0; turn < values.size() * 2; ++turn) {
            Store store;
            for (int index = 0; index < values.size(); ++index) {
                const auto offset = (turn + (turn % 2 ? values.size() - index - 1 : index)) % values.size();
                QVERIFY(store.addEntry(values[offset]));
            }
            for (int index = 0; index < turn * 32; ++index)
                QVERIFY(store.addEntry(Entry(photoshop, QStringLiteral("Temporary%1").arg(index), Value(index))));
            for (int index = 0; index < turn * 32; ++index)
                store.removeEntry(photoshop, QStringLiteral("Temporary%1").arg(index));
            QBuffer output;
            QVERIFY(backend->saveTo(&store, &output, jpegHeader ? IOBackend::JpegHeader : IOBackend::NoHeader));
            actual = output.data();
            QVERIFY(!actual.isEmpty());
            if (turn == 0) expected = actual;
            else QCOMPARE(actual, expected);
        }
        if (kind == QStringLiteral("xmp") && !jpegHeader) {
            QBuffer input(&expected);
            Store loaded;
            QVERIFY(backend->loadFrom(&loaded, &input));
            QCOMPARE(loaded.getEntry(dc, "creator").value(), values.last().value());
        }
        if (qEnvironmentVariableIsSet("LIBREPAINT_GPU_RESULT_DIR"))
            QVERIFY(write(QStringLiteral(".bin"), expected));
    }
};

QTEST_GUILESS_MAIN(KisMetadataExportTest)
#include "KisMetadataExportTest.moc"
