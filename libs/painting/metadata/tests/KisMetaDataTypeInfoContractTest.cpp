/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_parser.h"
#include "kis_meta_data_schema.h"
#include "kis_meta_data_type_info.h"
#include "kis_meta_data_type_info_p.h"
#include "kis_meta_data_value.h"

#include <QDateTime>
#include <QTest>
#include <QVariant>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{

KisMetaData::Value variantValue(const QVariant &value)
{
    return KisMetaData::Value(value);
}

KisMetaData::Value languageValue(const QVariant &text, const QVariant &language)
{
    KisMetaData::Value value(text);
    value.addPropertyQualifier(QStringLiteral("xml:lang"), variantValue(language));
    return value;
}

} // namespace

class KisMetaDataTypeInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void propertyTypeOrdinalsRemainStable();
    void privateStateStartsEmpty();
    void primitiveTypesDescribeValidateAndParseValues();
    void languageArrayAcceptsQualifiedTextEntries();
    void arrayFactoriesCacheAndValidateElementKinds();
    void choiceValuesCopyAssignAndExposeFields();
    void choiceFactoriesDifferentiateOpenAndClosedValues();
    void structureFactoryPreservesBorrowedSchemaAndName();
};

void KisMetaDataTypeInfoContractTest::propertyTypeOrdinalsRemainStable()
{
    using TypeInfo = KisMetaData::TypeInfo;

    QCOMPARE(static_cast<int>(TypeInfo::BooleanType), 0);
    QCOMPARE(static_cast<int>(TypeInfo::IntegerType), 1);
    QCOMPARE(static_cast<int>(TypeInfo::DateType), 2);
    QCOMPARE(static_cast<int>(TypeInfo::TextType), 3);
    QCOMPARE(static_cast<int>(TypeInfo::OrderedArrayType), 4);
    QCOMPARE(static_cast<int>(TypeInfo::UnorderedArrayType), 5);
    QCOMPARE(static_cast<int>(TypeInfo::AlternativeArrayType), 6);
    QCOMPARE(static_cast<int>(TypeInfo::LangArrayType), 7);
    QCOMPARE(static_cast<int>(TypeInfo::StructureType), 8);
    QCOMPARE(static_cast<int>(TypeInfo::RationalType), 9);
    QCOMPARE(static_cast<int>(TypeInfo::GPSCoordinateType), 10);
    QCOMPARE(static_cast<int>(TypeInfo::OpenedChoice), 11);
    QCOMPARE(static_cast<int>(TypeInfo::ClosedChoice), 12);
}

void KisMetaDataTypeInfoContractTest::privateStateStartsEmpty()
{
    const KisMetaData::TypeInfo::Private state;

    QCOMPARE(state.propertyType, KisMetaData::TypeInfo::BooleanType);
    QCOMPARE(state.embeddedTypeInfo, nullptr);
    QVERIFY(state.choices.isEmpty());
    QCOMPARE(state.structureSchema, nullptr);
    QVERIFY(state.structureName.isEmpty());
    QCOMPARE(state.parser, nullptr);
}

void KisMetaDataTypeInfoContractTest::primitiveTypesDescribeValidateAndParseValues()
{
    using TypeInfo = KisMetaData::TypeInfo;

    const TypeInfo *const booleanType = TypeInfo::Private::Boolean;
    const TypeInfo *const integerType = TypeInfo::Private::Integer;
    const TypeInfo *const dateType = TypeInfo::Private::Date;
    const TypeInfo *const textType = TypeInfo::Private::Text;
    const TypeInfo *const rationalType = TypeInfo::Private::Rational;
    const TypeInfo *const gpsType = TypeInfo::Private::GPSCoordinate;

    QCOMPARE(booleanType->propertyType(), TypeInfo::BooleanType);
    QCOMPARE(integerType->propertyType(), TypeInfo::IntegerType);
    QCOMPARE(dateType->propertyType(), TypeInfo::DateType);
    QCOMPARE(textType->propertyType(), TypeInfo::TextType);
    QCOMPARE(rationalType->propertyType(), TypeInfo::RationalType);
    QCOMPARE(gpsType->propertyType(), TypeInfo::GPSCoordinateType);

    const QList<const TypeInfo *> primitiveTypes{
        booleanType,
        integerType,
        dateType,
        textType,
        rationalType,
        gpsType,
    };
    for (const TypeInfo *type : primitiveTypes) {
        QCOMPARE(type->embeddedPropertyType(), nullptr);
        QVERIFY(type->choices().isEmpty());
        QVERIFY(type->hasCorrectValue(KisMetaData::Value()));
    }

    const KisMetaData::Value booleanValue(QVariant(true));
    const KisMetaData::Value integerValue(QVariant(17));
    const KisMetaData::Value dateValue(
        QVariant(QDateTime::fromString(QStringLiteral("2026-08-30T03:00:00"), Qt::ISODate)));
    const KisMetaData::Value textValue(QVariant(QStringLiteral("metadata")));
    const KisMetaData::Value rationalValue(KisMetaData::Rational(7, 3));

    QVERIFY(booleanType->hasCorrectType(booleanValue));
    QVERIFY(!booleanType->hasCorrectType(integerValue));
    QVERIFY(integerType->hasCorrectType(integerValue));
    QVERIFY(!integerType->hasCorrectType(textValue));
    QVERIFY(dateType->hasCorrectType(dateValue));
    QVERIFY(!dateType->hasCorrectType(textValue));
    QVERIFY(textType->hasCorrectType(textValue));
    QVERIFY(gpsType->hasCorrectType(textValue));
    QVERIFY(!gpsType->hasCorrectType(integerValue));
    QVERIFY(rationalType->hasCorrectType(rationalValue));
    QVERIFY(!rationalType->hasCorrectType(integerValue));

    QCOMPARE(booleanType->parser(), nullptr);
    QCOMPARE(gpsType->parser(), nullptr);
    QVERIFY(integerType->parser());
    QVERIFY(dateType->parser());
    QVERIFY(textType->parser());
    QVERIFY(rationalType->parser());
    QCOMPARE(integerType->parser()->parse(QStringLiteral("42")).asVariant(), QVariant(42));
    QCOMPARE(textType->parser()->parse(QStringLiteral("caption")).asVariant(), QVariant(QStringLiteral("caption")));
    QCOMPARE(rationalType->parser()->parse(QStringLiteral("5/8")).asRational(), KisMetaData::Rational(5, 8));
}

void KisMetaDataTypeInfoContractTest::languageArrayAcceptsQualifiedTextEntries()
{
    using TypeInfo = KisMetaData::TypeInfo;

    const TypeInfo *const languageType = TypeInfo::Private::LangArray;
    QCOMPARE(languageType->propertyType(), TypeInfo::LangArrayType);
    QCOMPARE(languageType->embeddedPropertyType(), TypeInfo::Private::Text);
    QVERIFY(languageType->choices().isEmpty());
    QCOMPARE(languageType->parser(), nullptr);

    const KisMetaData::Value english = languageValue(QStringLiteral("Hello"), QStringLiteral("en"));
    const KisMetaData::Value japanese = languageValue(QString::fromUtf8("こんにちは"), QStringLiteral("ja"));
    const KisMetaData::Value validLanguages(QList<KisMetaData::Value>{english, japanese},
                                            KisMetaData::Value::LangArray);
    QVERIFY(languageType->hasCorrectType(validLanguages));
    QVERIFY(
        languageType->hasCorrectType(KisMetaData::Value(QList<KisMetaData::Value>(), KisMetaData::Value::LangArray)));

    QVERIFY(!languageType->hasCorrectType(KisMetaData::Value(QList<KisMetaData::Value>{english})));
}

void KisMetaDataTypeInfoContractTest::arrayFactoriesCacheAndValidateElementKinds()
{
    using TypeInfo = KisMetaData::TypeInfo;

    const TypeInfo *const ordered = TypeInfo::Private::orderedArray(TypeInfo::Private::Integer);
    const TypeInfo *const orderedAgain = TypeInfo::Private::orderedArray(TypeInfo::Private::Integer);
    const TypeInfo *const orderedText = TypeInfo::Private::orderedArray(TypeInfo::Private::Text);
    const TypeInfo *const unordered = TypeInfo::Private::unorderedArray(TypeInfo::Private::Integer);
    const TypeInfo *const alternative = TypeInfo::Private::alternativeArray(TypeInfo::Private::Integer);

    QCOMPARE(ordered, orderedAgain);
    QVERIFY(ordered != orderedText);
    QCOMPARE(ordered->propertyType(), TypeInfo::OrderedArrayType);
    QCOMPARE(unordered->propertyType(), TypeInfo::UnorderedArrayType);
    QCOMPARE(alternative->propertyType(), TypeInfo::AlternativeArrayType);
    QCOMPARE(ordered->embeddedPropertyType(), TypeInfo::Private::Integer);
    QCOMPARE(orderedText->embeddedPropertyType(), TypeInfo::Private::Text);
    QCOMPARE(unordered->embeddedPropertyType(), TypeInfo::Private::Integer);
    QCOMPARE(alternative->embeddedPropertyType(), TypeInfo::Private::Integer);
    QVERIFY(ordered->choices().isEmpty());
    QCOMPARE(ordered->parser(), nullptr);

    const QList<KisMetaData::Value> integers{variantValue(3), variantValue(5)};
    const QList<KisMetaData::Value> mixed{variantValue(3), variantValue(QStringLiteral("five"))};
    QVERIFY(ordered->hasCorrectType(KisMetaData::Value(integers, KisMetaData::Value::OrderedArray)));
    QVERIFY(!ordered->hasCorrectType(KisMetaData::Value(mixed, KisMetaData::Value::OrderedArray)));
    QVERIFY(!ordered->hasCorrectType(KisMetaData::Value(integers, KisMetaData::Value::UnorderedArray)));
    QVERIFY(unordered->hasCorrectType(KisMetaData::Value(integers, KisMetaData::Value::UnorderedArray)));
    QVERIFY(!unordered->hasCorrectType(KisMetaData::Value(mixed, KisMetaData::Value::UnorderedArray)));
    QVERIFY(alternative->hasCorrectType(KisMetaData::Value(integers, KisMetaData::Value::AlternativeArray)));
    QVERIFY(!alternative->hasCorrectType(KisMetaData::Value(mixed, KisMetaData::Value::AlternativeArray)));
}

void KisMetaDataTypeInfoContractTest::choiceValuesCopyAssignAndExposeFields()
{
    const KisMetaData::TypeInfo::Choice original(variantValue(12), QStringLiteral("twelve"));
    const KisMetaData::TypeInfo::Choice copied(original);
    KisMetaData::TypeInfo::Choice assigned(variantValue(7), QStringLiteral("seven"));
    assigned = original;

    QCOMPARE(original.value(), variantValue(12));
    QCOMPARE(original.hint(), QStringLiteral("twelve"));
    QCOMPARE(copied.value(), original.value());
    QCOMPARE(copied.hint(), original.hint());
    QCOMPARE(assigned.value(), original.value());
    QCOMPARE(assigned.hint(), original.hint());
}

void KisMetaDataTypeInfoContractTest::choiceFactoriesDifferentiateOpenAndClosedValues()
{
    using TypeInfo = KisMetaData::TypeInfo;

    const QList<TypeInfo::Choice> choices{
        TypeInfo::Choice(variantValue(12), QStringLiteral("twelve")),
        TypeInfo::Choice(variantValue(42), QStringLiteral("forty-two")),
    };
    const TypeInfo *const openChoice =
        TypeInfo::Private::createChoice(TypeInfo::OpenedChoice, TypeInfo::Private::Integer, choices);
    const TypeInfo *const closedChoice =
        TypeInfo::Private::createChoice(TypeInfo::ClosedChoice, TypeInfo::Private::Integer, choices);

    QCOMPARE(openChoice->propertyType(), TypeInfo::OpenedChoice);
    QCOMPARE(closedChoice->propertyType(), TypeInfo::ClosedChoice);
    QCOMPARE(openChoice->embeddedPropertyType(), TypeInfo::Private::Integer);
    QCOMPARE(closedChoice->embeddedPropertyType(), TypeInfo::Private::Integer);
    QCOMPARE(openChoice->parser(), TypeInfo::Private::Integer->parser());
    QCOMPARE(closedChoice->parser(), TypeInfo::Private::Integer->parser());
    QCOMPARE(openChoice->choices().size(), 2);
    QCOMPARE(openChoice->choices().at(0).value(), variantValue(12));
    QCOMPARE(openChoice->choices().at(1).hint(), QStringLiteral("forty-two"));

    QVERIFY(openChoice->hasCorrectType(variantValue(-1)));
    QVERIFY(!openChoice->hasCorrectType(variantValue(QStringLiteral("12"))));
    QVERIFY(openChoice->hasCorrectValue(variantValue(-1)));
    QVERIFY(closedChoice->hasCorrectType(variantValue(-1)));
    QVERIFY(closedChoice->hasCorrectValue(variantValue(12)));
    QVERIFY(closedChoice->hasCorrectValue(variantValue(42)));
    QVERIFY(!closedChoice->hasCorrectValue(variantValue(-1)));
}

void KisMetaDataTypeInfoContractTest::structureFactoryPreservesBorrowedSchemaAndName()
{
    using TypeInfo = KisMetaData::TypeInfo;

    KisMetaData::Schema *const borrowedSchema = reinterpret_cast<KisMetaData::Schema *>(quintptr(0x1234));
    const TypeInfo *const structure = TypeInfo::Private::createStructure(borrowedSchema, QStringLiteral("Contact"));

    QCOMPARE(structure->propertyType(), TypeInfo::StructureType);
    QCOMPARE(structure->structureSchema(), borrowedSchema);
    QCOMPARE(structure->structureName(), QStringLiteral("Contact"));
    QCOMPARE(structure->embeddedPropertyType(), nullptr);
    QVERIFY(structure->choices().isEmpty());
    QCOMPARE(structure->parser(), nullptr);
    QVERIFY(!structure->hasCorrectType(variantValue(1)));
    QVERIFY(structure->hasCorrectValue(variantValue(1)));
}

QTEST_MAIN(KisMetaDataTypeInfoContractTest)

#include "KisMetaDataTypeInfoContractTest.moc"
