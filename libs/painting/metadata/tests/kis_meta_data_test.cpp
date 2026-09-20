/*
 *  SPDX-FileCopyrightText: 2008-2009 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_entry.h"
#include "kis_meta_data_schema.h"
#include "kis_meta_data_schema_registry.h"
#include "kis_meta_data_store.h"
#include "kis_meta_data_validator.h"
#include "kis_meta_data_value.h"

#include <kistest.h>

class KisMetaDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void metadataStoreAddsCopiesAndRemovesEntries();
    void validationReportsUnknownTypesAndClosedChoiceValues();
};

void KisMetaDataTest::metadataStoreAddsCopiesAndRemovesEntries()
{
    // Consumer: metadata importers and the metadata editor.
    // Operation: add an entry, create an entry on demand, copy the store, and remove an entry.
    // Observable result: each qualified entry retains its value until removal and copied stores retain independent entries.
    // Failure impact: image metadata can disappear, be overwritten by a duplicate, or be unavailable to the editor after import.
    const KisMetaData::Schema *const tiffSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::TIFFSchemaUri);
    QVERIFY(tiffSchema);

    KisMetaData::Store store;
    const KisMetaData::Entry artist(tiffSchema, QStringLiteral("Artist"), KisMetaData::Value(QStringLiteral("Ada")));
    QVERIFY(store.addEntry(artist));
    QVERIFY(!store.addEntry(artist));
    QVERIFY(store.containsEntry(tiffSchema, QStringLiteral("Artist")));
    QCOMPARE(store.getValue(KisMetaData::Schema::TIFFSchemaUri, QStringLiteral("Artist")).asVariant(),
             QVariant(QStringLiteral("Ada")));

    store.getEntry(tiffSchema, QStringLiteral("Copyright")).value() = KisMetaData::Value(QStringLiteral("2026"));
    QVERIFY(store.containsEntry(tiffSchema, QStringLiteral("Copyright")));

    KisMetaData::Store copied;
    copied.copyFrom(&store);
    QCOMPARE(copied.getValue(KisMetaData::Schema::TIFFSchemaUri, QStringLiteral("Copyright")).asVariant(),
             QVariant(QStringLiteral("2026")));

    store.removeEntry(tiffSchema, QStringLiteral("Artist"));
    QVERIFY(!store.containsEntry(tiffSchema, QStringLiteral("Artist")));
    QVERIFY(copied.containsEntry(tiffSchema, QStringLiteral("Artist")));
}

void KisMetaDataTest::validationReportsUnknownTypesAndClosedChoiceValues()
{
    // Consumer: metadata importers and the metadata editor.
    // Operation: validate imported EXIF values with an unknown field, an incompatible type, and an undefined closed choice.
    // Observable result: each failure is reported with its distinct validation reason.
    // Failure impact: callers cannot distinguish unsupported metadata from corrupt data and may export invalid image metadata.
    const KisMetaData::Schema *const exifSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::EXIFSchemaUri);
    QVERIFY(exifSchema);

    KisMetaData::Store store;
    QVERIFY(store.addEntry(KisMetaData::Entry(exifSchema, QStringLiteral("PixelXDimension"), KisMetaData::Value(1200))));
    QVERIFY(store.addEntry(KisMetaData::Entry(exifSchema, QStringLiteral("UnknownField"), KisMetaData::Value(1))));
    QVERIFY(store.addEntry(KisMetaData::Entry(exifSchema, QStringLiteral("FNumber"), KisMetaData::Value(8))));
    QVERIFY(store.addEntry(KisMetaData::Entry(exifSchema, QStringLiteral("SensingMethod"), KisMetaData::Value(1242))));

    const KisMetaData::Validator validator(&store);
    QCOMPARE(validator.countValidEntries(), 1);
    QCOMPARE(validator.countInvalidEntries(), 3);
    QCOMPARE(validator.invalidEntries().value(exifSchema->generateQualifiedName(QStringLiteral("UnknownField"))).type(),
             KisMetaData::Validator::Reason::UNKNOWN_ENTRY);
    QCOMPARE(validator.invalidEntries().value(exifSchema->generateQualifiedName(QStringLiteral("FNumber"))).type(),
             KisMetaData::Validator::Reason::INVALID_TYPE);
    QCOMPARE(validator.invalidEntries().value(exifSchema->generateQualifiedName(QStringLiteral("SensingMethod"))).type(),
             KisMetaData::Validator::Reason::INVALID_VALUE);
}

KISTEST_MAIN(KisMetaDataTest)

#include "kis_meta_data_test.moc"
