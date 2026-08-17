/*
 * SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 * SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_exif_test.h"

#include <simpletest.h>


#include <QBuffer>
#include <QCoreApplication>

#include <kis_debug.h>
#include <kis_meta_data_backend_registry.h>
#include <kis_meta_data_entry.h>
#include <kis_meta_data_io_backend.h>
#include <kis_meta_data_schema.h>
#include <kis_meta_data_schema_registry.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_validator.h>
#include <kis_meta_data_value.h>

#ifndef FILES_DATA_DIR
#error "FILES_DATA_DIR not set. A directory with the data used for testing the metadata parser in krita"
#endif

using namespace KisMetaData;

void KisExifTest::testExifLoader()
{
    IOBackend *exifIO = KisMetadataBackendRegistry::instance()->get("exif");
    QVERIFY(exifIO);
    QFile exifFile(QString(FILES_DATA_DIR) + "/metadata/hpim3238.exv");
    KIS_ASSERT(exifFile.open(QIODevice::ReadOnly));
    exifFile.seek(17);
    QByteArray exifBytes = exifFile.readAll();
    QBuffer exifBuffer(&exifBytes);

    Store *store = new Store;
    bool loadSuccess = exifIO->loadFrom(store, &exifBuffer);
    QVERIFY(loadSuccess);
    Validator validator(store);

    for (QMap<QString, Validator::Reason>::const_iterator it = validator.invalidEntries().begin();
         it != validator.invalidEntries().end();
         ++it) {
        dbgKrita << it.key() << " = " << it.value().type() << " entry = " << store->getEntry(it.key());
    }

    QCOMPARE(validator.countInvalidEntries(), 0);
    QCOMPARE(validator.countValidEntries(), 51);

    const KisMetaData::Schema *tiffSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::TIFFSchemaUri);

    QCOMPARE(store->getEntry(tiffSchema, "Make").value(), Value("Hewlett-Packard"));
    QCOMPARE(store->getEntry(tiffSchema, "Model").value(), Value("HP PhotoSmart R707 (V01.00) "));
    QCOMPARE(store->getEntry(tiffSchema, "Orientation").value(), Value(1));
    QCOMPARE(store->getEntry(tiffSchema, "XResolution").value(), Value(Rational(72 / 1)));
    QCOMPARE(store->getEntry(tiffSchema, "YResolution").value(), Value(Rational(72 / 1)));
    QCOMPARE(store->getEntry(tiffSchema, "ResolutionUnit").value(), Value(2));
    QCOMPARE(store->getEntry(tiffSchema, "YCbCrPositioning").value(), Value(1));

    const KisMetaData::Schema *exifSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::EXIFSchemaUri);

    QCOMPARE(store->getEntry(exifSchema, "ExposureTime").value(), Value(Rational(35355, 100000)));
    QCOMPARE(store->getEntry(exifSchema, "FNumber").value(), Value(Rational(280, 100)));
    QCOMPARE(store->getEntry(exifSchema, "ExposureProgram").value(), Value(2));
    //     QCOMPARE(store->getEntry(exifSchema, "ISOSpeedRatings").value(), Value(100)); // TODO it's a list
    QCOMPARE(store->getEntry(exifSchema, "ExifVersion").value(), Value("0220"));
    QCOMPARE(store->getEntry(exifSchema, "DateTimeOriginal").value(),
             Value(QDateTime(QDate(2007, 5, 8), QTime(0, 19, 18))));
    QCOMPARE(store->getEntry(exifSchema, "DateTimeDigitized").value(),
             Value(QDateTime(QDate(2007, 5, 8), QTime(0, 19, 18))));
    // TODO ComponentsConfiguration
    QCOMPARE(store->getEntry(exifSchema, "ShutterSpeedValue").value(), Value(Rational(384, 256)));
    QCOMPARE(store->getEntry(exifSchema, "ApertureValue").value(), Value(Rational(780, 256)));
    QCOMPARE(store->getEntry(exifSchema, "BrightnessValue").value(), Value(Rational(-37, 256)));
    QCOMPARE(store->getEntry(exifSchema, "ExposureBiasValue").value(), Value(Rational(256, 256)));
    QCOMPARE(store->getEntry(exifSchema, "MaxApertureValue").value(), Value(Rational(280, 100)));
    QCOMPARE(store->getEntry(exifSchema, "SubjectDistance").value(), Value(Rational(65535, 1000)));

    const KisMetaData::Schema *dcSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::DublinCoreSchemaUri);
    Q_UNUSED(dcSchema);

    const KisMetaData::Schema *xmpSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::XMPSchemaUri);
    QCOMPARE(store->getEntry(xmpSchema, "CreatorTool").value(), Value("digiKam-0.9.1"));
    QCOMPARE(store->getEntry(xmpSchema, "ModifyDate").value(), Value(QDateTime(QDate(2007, 5, 8), QTime(0, 19, 18))));

    const KisMetaData::Schema *mknSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::MakerNoteSchemaUri);
    QCOMPARE(store->getEntry(mknSchema, "RawData").value(), Value("SFBNZXQ="));
}

void KisExifTest::testOECF()
{
    IOBackend *exifIO = KisMetadataBackendRegistry::instance()->get("exif");
    QVERIFY(exifIO);

    QFile exifFile(QString(FILES_DATA_DIR) + "/metadata/hpim3238.exv");
    KIS_ASSERT(exifFile.open(QIODevice::ReadOnly));
    exifFile.seek(17);
    QByteArray exifBytes = exifFile.readAll();

    QBuffer exifBuffer(&exifBytes);
    Store store;
    QVERIFY(exifIO->loadFrom(&store, &exifBuffer));

    const Schema *exifSchema =
        SchemaRegistry::instance()->schemaFromUri(Schema::EXIFSchemaUri);
    const QMap<QString, Value> oecf = store.getEntry(exifSchema, "OECF").value().asStructure();
    QCOMPARE(oecf["Columns"].asInteger(), 2);
    QCOMPARE(oecf["Rows"].asInteger(), 129);
    QCOMPARE(oecf["Names"].asArray().size(), 2);
    QCOMPARE(oecf["Names"].asArray().at(0).asVariant().toString(), QString());
    QCOMPARE(oecf["Names"].asArray().at(1).asVariant().toString(), QString());
    QCOMPARE(oecf["Values"].asArray().size(), 258);
    QCOMPARE(oecf["Values"].asArray().at(0).asRational().numerator, 0);
    QCOMPARE(oecf["Values"].asArray().at(0).asRational().denominator, 1);
    QCOMPARE(oecf["Values"].asArray().at(2).asRational().numerator, 8);
    QCOMPARE(oecf["Values"].asArray().at(2).asRational().denominator, 1);
}

void KisExifTest::testMalformedOECF()
{
    IOBackend *exifIO = KisMetadataBackendRegistry::instance()->get("exif");
    QVERIFY(exifIO);

    QFile exifFile(QString(FILES_DATA_DIR) + "/metadata/hpim3238.exv");
    KIS_ASSERT(exifFile.open(QIODevice::ReadOnly));
    exifFile.seek(17);
    QByteArray exifBytes = exifFile.readAll();

    const QByteArray oecfPrefix = QByteArray::fromHex("020081000000000001000000000000000100000008000000");
    const qsizetype oecfOffset = exifBytes.indexOf(oecfPrefix);
    QVERIFY(oecfOffset >= 0);
    exifBytes.replace(oecfOffset, 4, QByteArray::fromHex("ffffffff"));

    QBuffer exifBuffer(&exifBytes);
    Store store;
    QVERIFY(exifIO->loadFrom(&store, &exifBuffer));

    const Schema *exifSchema =
        SchemaRegistry::instance()->schemaFromUri(Schema::EXIFSchemaUri);
    const Schema *tiffSchema =
        SchemaRegistry::instance()->schemaFromUri(Schema::TIFFSchemaUri);
    QVERIFY(!store.containsEntry(exifSchema, "OECF"));
    QCOMPARE(store.getEntry(tiffSchema, "Model").value(), Value("HP PhotoSmart R707 (V01.00) "));
}

void KisExifTest::testCFAPattern()
{
    IOBackend *exifIO = KisMetadataBackendRegistry::instance()->get("exif");
    QVERIFY(exifIO);

    const Schema *exifSchema =
        SchemaRegistry::instance()->schemaFromUri(Schema::EXIFSchemaUri);
    QMap<QString, Value> pattern;
    pattern["Columns"] = Value(2);
    pattern["Rows"] = Value(2);
    pattern["Values"] = Value({Value(0), Value(1), Value(2), Value(255)}, Value::OrderedArray);

    Store sourceStore;
    QVERIFY(sourceStore.addEntry({exifSchema, "CFAPattern", Value(pattern)}));

    QBuffer output;
    QVERIFY(exifIO->saveTo(&sourceStore, &output));
    QByteArray exifBytes = output.data();

    QBuffer validInput(&exifBytes);
    Store loadedStore;
    QVERIFY(exifIO->loadFrom(&loadedStore, &validInput));
    const QMap<QString, Value> loadedPattern =
        loadedStore.getEntry(exifSchema, "CFAPattern").value().asStructure();
    QCOMPARE(loadedPattern["Columns"].asInteger(), 2);
    QCOMPARE(loadedPattern["Rows"].asInteger(), 2);
    QCOMPARE(loadedPattern["Values"].asArray().size(), 4);
    QCOMPARE(loadedPattern["Values"].asArray().at(3).asInteger(), 255);

    const QByteArray cfaPayload = QByteArray::fromHex("02000200000102ff");
    const qsizetype cfaOffset = exifBytes.indexOf(cfaPayload);
    QVERIFY(cfaOffset >= 0);
    exifBytes.replace(cfaOffset, 4, QByteArray::fromHex("ffffffff"));

    QBuffer invalidInput(&exifBytes);
    Store invalidStore;
    QVERIFY(exifIO->loadFrom(&invalidStore, &invalidInput));
    QVERIFY(!invalidStore.containsEntry(exifSchema, "CFAPattern"));
}

SIMPLE_TEST_MAIN(KisExifTest)
