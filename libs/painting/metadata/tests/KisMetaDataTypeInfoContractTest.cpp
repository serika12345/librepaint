/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_entry.h"
#include "kis_meta_data_parser.h"
#include "kis_meta_data_schema.h"
#include "kis_meta_data_schema_registry.h"
#include "kis_meta_data_store.h"
#include "kis_meta_data_type_info.h"
#include "kis_meta_data_validator.h"
#include "kis_meta_data_value.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QVariant>

#include <kistest.h>

namespace
{

KisMetaData::Value languageValue(const QString &text, const QString &language)
{
    KisMetaData::Value value(text);
    value.addPropertyQualifier(QStringLiteral("xml:lang"), KisMetaData::Value(language));
    return value;
}

} // namespace

class KisMetaDataTypeInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void schemaValidationAcceptsTypedExifEntries();
    void schemaValidationReportsWrongTypesAndClosedChoiceValues();
    void schemaValidationRejectsLanguageEntriesWithoutLanguageQualifiers();
    void schemaParsesXmpScalarTextForMetadataValidation();

private:
    const KisMetaData::Schema *m_exifSchema {nullptr};
};

void KisMetaDataTypeInfoContractTest::initTestCase()
{
    m_exifSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::EXIFSchemaUri);
    QVERIFY(m_exifSchema);
}

void KisMetaDataTypeInfoContractTest::schemaValidationAcceptsTypedExifEntries()
{
    // Consumer: metadata importers and the metadata editor.
    // Operation: store EXIF scalar, array, structure, and localized comment values.
    // Observable result: the metadata validator accepts every value under its schema field.
    // Failure impact: valid image metadata is reported as corrupt and cannot be reliably edited or exported.
    KisMetaData::Store store;

    QVERIFY(store.addEntry(KisMetaData::Entry(m_exifSchema, QStringLiteral("PixelXDimension"), KisMetaData::Value(1200))));
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("ISOSpeedRatings"),
        KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(100), KisMetaData::Value(200)}))));
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("OECF"),
        KisMetaData::Value(QMap<QString, KisMetaData::Value>{
            {QStringLiteral("Columns"), KisMetaData::Value(1)},
            {QStringLiteral("Rows"), KisMetaData::Value(1)},
            {QStringLiteral("Names"), KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(QStringLiteral("luminance"))})},
            {QStringLiteral("Values"), KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(KisMetaData::Rational(1, 2))})},
        }))));
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("UserComment"),
        KisMetaData::Value(QList<KisMetaData::Value>{languageValue(QStringLiteral("Hello"), QStringLiteral("en"))},
                           KisMetaData::Value::LangArray))));

    const KisMetaData::Validator validator(&store);
    QCOMPARE(validator.countValidEntries(), 4);
    QCOMPARE(validator.countInvalidEntries(), 0);
}

void KisMetaDataTypeInfoContractTest::schemaValidationReportsWrongTypesAndClosedChoiceValues()
{
    // Consumer: metadata importers and the metadata editor.
    // Operation: validate an EXIF array with an incompatible element and a closed-choice value outside its schema list.
    // Observable result: the validator reports the array as an invalid type and the choice as an invalid value.
    // Failure impact: malformed metadata reaches export code or the editor rejects a value for the wrong reason.
    KisMetaData::Store store;

    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("ISOSpeedRatings"),
        KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(QStringLiteral("fast"))}))));
    QVERIFY(store.addEntry(KisMetaData::Entry(m_exifSchema, QStringLiteral("ColorSpace"), KisMetaData::Value(2))));

    const KisMetaData::Validator validator(&store);
    QCOMPARE(validator.countValidEntries(), 0);
    QCOMPARE(validator.countInvalidEntries(), 2);
    QCOMPARE(validator.invalidEntries().value(m_exifSchema->generateQualifiedName(QStringLiteral("ISOSpeedRatings"))).type(),
             KisMetaData::Validator::Reason::INVALID_TYPE);
    QCOMPARE(validator.invalidEntries().value(m_exifSchema->generateQualifiedName(QStringLiteral("ColorSpace"))).type(),
             KisMetaData::Validator::Reason::INVALID_VALUE);
}

void KisMetaDataTypeInfoContractTest::schemaValidationRejectsLanguageEntriesWithoutLanguageQualifiers()
{
    // Consumer: metadata importers and the metadata editor.
    // Operation: validate an EXIF localized comment whose text lacks its xml:lang qualifier.
    // Observable result: the validator rejects the malformed localized value as an invalid type.
    // Failure impact: exporters can write locale-less language alternatives that other metadata consumers cannot interpret.
    KisMetaData::Store store;
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("UserComment"),
        KisMetaData::Value(QList<KisMetaData::Value>{KisMetaData::Value(QStringLiteral("Hello"))},
                           KisMetaData::Value::LangArray))));

    const KisMetaData::Validator validator(&store);
    QCOMPARE(validator.countValidEntries(), 0);
    QCOMPARE(validator.countInvalidEntries(), 1);
    QCOMPARE(validator.invalidEntries().value(m_exifSchema->generateQualifiedName(QStringLiteral("UserComment"))).type(),
             KisMetaData::Validator::Reason::INVALID_TYPE);
}

void KisMetaDataTypeInfoContractTest::schemaParsesXmpScalarTextForMetadataValidation()
{
    // Consumer: the XMP metadata importer.
    // Operation: parse canonical EXIF integer, rational, and date-time text through their schema fields.
    // Observable result: the parsed values retain their domain values and the metadata validator accepts them.
    // Failure impact: imported XMP metadata is rejected or saved with a different dimension, exposure, or capture time.
    const KisMetaData::TypeInfo *const pixelDimensionType = m_exifSchema->propertyType(QStringLiteral("PixelXDimension"));
    const KisMetaData::TypeInfo *const fNumberType = m_exifSchema->propertyType(QStringLiteral("FNumber"));
    const KisMetaData::TypeInfo *const captureTimeType = m_exifSchema->propertyType(QStringLiteral("DateTimeOriginal"));
    QVERIFY(pixelDimensionType);
    QVERIFY(fNumberType);
    QVERIFY(captureTimeType);
    QVERIFY(pixelDimensionType->parser());
    QVERIFY(fNumberType->parser());
    QVERIFY(captureTimeType->parser());

    KisMetaData::Store store;
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("PixelXDimension"),
        pixelDimensionType->parser()->parse(QStringLiteral("1200")))));
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("FNumber"),
        fNumberType->parser()->parse(QStringLiteral("5/8")))));
    QVERIFY(store.addEntry(KisMetaData::Entry(
        m_exifSchema,
        QStringLiteral("DateTimeOriginal"),
        captureTimeType->parser()->parse(QStringLiteral("2026-08-30T03:00:00")))));

    QCOMPARE(store.getValue(KisMetaData::Schema::EXIFSchemaUri, QStringLiteral("PixelXDimension")).asVariant(), QVariant(1200));
    QCOMPARE(store.getValue(KisMetaData::Schema::EXIFSchemaUri, QStringLiteral("FNumber")).asRational(),
             KisMetaData::Rational(5, 8));
    QCOMPARE(store.getValue(KisMetaData::Schema::EXIFSchemaUri, QStringLiteral("DateTimeOriginal")).asVariant().toDateTime(),
             QDateTime(QDate(2026, 8, 30), QTime(3, 0)));

    const KisMetaData::Validator validator(&store);
    QCOMPARE(validator.countValidEntries(), 3);
    QCOMPARE(validator.countInvalidEntries(), 0);
}

KISTEST_MAIN(KisMetaDataTypeInfoContractTest)

#include "KisMetaDataTypeInfoContractTest.moc"
