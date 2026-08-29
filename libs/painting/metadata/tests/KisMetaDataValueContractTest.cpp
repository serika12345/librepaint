/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_value.h"

#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QMap>
#include <QMetaType>
#include <QString>
#include <QTest>
#include <QTime>
#include <QVariant>

#include <memory>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

class LifetimeProbe
{
public:
    explicit LifetimeProbe(const std::shared_ptr<int> &destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~LifetimeProbe()
    {
        ++*m_destructionCount;
    }

private:
    std::shared_ptr<int> m_destructionCount;
};

Q_DECLARE_METATYPE(LifetimeProbe)

namespace
{

KisMetaData::Value variantValue(const QVariant &variant)
{
    return KisMetaData::Value(variant);
}

KisMetaData::Value languageValue(const QString &language, const QString &text)
{
    KisMetaData::Value value{QVariant(text)};
    value.addPropertyQualifier(QStringLiteral("xml:lang"), variantValue(language));
    return value;
}

} // namespace

class KisMetaDataValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rationalStoresSignedComponentsAndCompares();
    void constructsEveryValueCategory();
    void leavesOwnedVariantPayloadAliveAfterDestruction();
    void convertsScalarAndCompositeValues();
    void copiesAssignsAndComparesIndependentValues();
    void mapsQualifiedLanguageEntries();
    void updatesVariantsAndStructureFields();
    void reportsSuccessForRejectedVariantType();
    void reportsFailureAfterUpdatingArray();
    void addsVariantsArraysAndRationals();
    void collapsesDateTimeAdditionToDate();
    void usesOffByOneMillisecondCarryWhenAddingTimes();
    void treatsDistinctLanguageArraysAsEqual();
    void formatsSafeValueCategoriesAndDebugOutput();
};

void KisMetaDataValueContractTest::rationalStoresSignedComponentsAndCompares()
{
    const KisMetaData::Rational defaultValue;
    QCOMPARE(defaultValue.numerator, 0);
    QCOMPARE(defaultValue.denominator, 1);

    const KisMetaData::Rational negativeValue(-7, 3);
    QCOMPARE(negativeValue.numerator, -7);
    QCOMPARE(negativeValue.denominator, 3);
    QVERIFY(negativeValue == KisMetaData::Rational(-7, 3));
    QVERIFY(!(negativeValue == KisMetaData::Rational(-7, 4)));
}

void KisMetaDataValueContractTest::constructsEveryValueCategory()
{
    QCOMPARE(static_cast<int>(KisMetaData::Value::Invalid), 0);
    QCOMPARE(static_cast<int>(KisMetaData::Value::Variant), 1);
    QCOMPARE(static_cast<int>(KisMetaData::Value::OrderedArray), 2);
    QCOMPARE(static_cast<int>(KisMetaData::Value::UnorderedArray), 3);
    QCOMPARE(static_cast<int>(KisMetaData::Value::AlternativeArray), 4);
    QCOMPARE(static_cast<int>(KisMetaData::Value::LangArray), 5);
    QCOMPARE(static_cast<int>(KisMetaData::Value::Structure), 6);
    QCOMPARE(static_cast<int>(KisMetaData::Value::Rational), 7);

    const KisMetaData::Value invalid;
    const KisMetaData::Value variant(QVariant(17));
    const QList<KisMetaData::Value> elements{variantValue(QStringLiteral("entry"))};
    const KisMetaData::Value ordered(elements);
    const KisMetaData::Value unordered(elements, KisMetaData::Value::UnorderedArray);
    const KisMetaData::Value alternative(elements, KisMetaData::Value::AlternativeArray);
    const KisMetaData::Value language(elements, KisMetaData::Value::LangArray);
    const KisMetaData::Value structure(QMap<QString, KisMetaData::Value>{
        {QStringLiteral("field"), variantValue(23)},
    });
    const KisMetaData::Value rational(KisMetaData::Rational(5, 8));

    QCOMPARE(invalid.type(), KisMetaData::Value::Invalid);
    QCOMPARE(variant.type(), KisMetaData::Value::Variant);
    QCOMPARE(ordered.type(), KisMetaData::Value::OrderedArray);
    QCOMPARE(unordered.type(), KisMetaData::Value::UnorderedArray);
    QCOMPARE(alternative.type(), KisMetaData::Value::AlternativeArray);
    QCOMPARE(language.type(), KisMetaData::Value::LangArray);
    QCOMPARE(structure.type(), KisMetaData::Value::Structure);
    QCOMPARE(rational.type(), KisMetaData::Value::Rational);
}

void KisMetaDataValueContractTest::leavesOwnedVariantPayloadAliveAfterDestruction()
{
    const std::shared_ptr<int> destructionCount = std::make_shared<int>(0);
    KisMetaData::Value *value = [destructionCount]() {
        const QVariant payload = QVariant::fromValue(LifetimeProbe(destructionCount));
        return new KisMetaData::Value(payload);
    }();
    const int destructionsBeforeValue = *destructionCount;

    delete value;

    QCOMPARE(*destructionCount, destructionsBeforeValue);
}

void KisMetaDataValueContractTest::convertsScalarAndCompositeValues()
{
    const KisMetaData::Value integer(QVariant(7));
    QCOMPARE(integer.asDouble(), 7.0);
    QCOMPARE(integer.asInteger(), 7);
    QCOMPARE(integer.asVariant(), QVariant(7));
    QCOMPARE(integer.asRational(), KisMetaData::Rational());

    const KisMetaData::Value rational(KisMetaData::Rational(7, 2));
    QCOMPARE(rational.asDouble(), 3.5);
    QCOMPARE(rational.asInteger(), 3);
    QCOMPARE(rational.asVariant(), QVariant(QStringLiteral("7 / 2")));
    QCOMPARE(rational.asRational(), KisMetaData::Rational(7, 2));

    const KisMetaData::Value invalid;
    QCOMPARE(invalid.asDouble(), 0.0);
    QCOMPARE(invalid.asInteger(), 0);
    QVERIFY(!invalid.asVariant().isValid());
    QVERIFY(!invalid.isArray());
    QVERIFY(invalid.asArray().isEmpty());
    QVERIFY(invalid.asStructure().isEmpty());

    const QList<KisMetaData::Value> elements{variantValue(2), variantValue(3)};
    const KisMetaData::Value ordered(elements);
    QVERIFY(ordered.isArray());
    QCOMPARE(ordered.asArray(), elements);

    const KisMetaData::Value language(elements, KisMetaData::Value::LangArray);
    QVERIFY(!language.isArray());
    QVERIFY(language.asArray().isEmpty());

    const QMap<QString, KisMetaData::Value> fields{
        {QStringLiteral("answer"), variantValue(42)},
    };
    const KisMetaData::Value structure(fields);
    QCOMPARE(structure.asStructure(), fields);
}

void KisMetaDataValueContractTest::copiesAssignsAndComparesIndependentValues()
{
    KisMetaData::Value source(QVariant(QStringLiteral("alpha")));
    source.addPropertyQualifier(QStringLiteral("role"), variantValue(QStringLiteral("source")));

    KisMetaData::Value copied(source);
    KisMetaData::Value assigned;
    assigned = source;
    QVERIFY(copied == source);
    QVERIFY(assigned == source);
    QCOMPARE(copied.propertyQualifiers().value(QStringLiteral("role")).asVariant().toString(),
             QStringLiteral("source"));

    QVERIFY(copied.setVariant(QVariant(QStringLiteral("beta"))));
    QCOMPARE(source.asVariant().toString(), QStringLiteral("alpha"));
    QCOMPARE(copied.asVariant().toString(), QStringLiteral("beta"));
    QVERIFY(!(copied == source));
}

void KisMetaDataValueContractTest::mapsQualifiedLanguageEntries()
{
    const KisMetaData::Value english = languageValue(QStringLiteral("en"), QStringLiteral("Hello"));
    const KisMetaData::Value japanese = languageValue(QStringLiteral("ja"), QString::fromUtf8("こんにちは"));
    const KisMetaData::Value languageArray(QList<KisMetaData::Value>{english, japanese}, KisMetaData::Value::LangArray);

    QCOMPARE(english.propertyQualifiers().size(), 1);
    QCOMPARE(english.propertyQualifiers().value(QStringLiteral("xml:lang")).asVariant().toString(),
             QStringLiteral("en"));

    const QMap<QString, KisMetaData::Value> mapped = languageArray.asLangArray();
    QCOMPARE(mapped.size(), 2);
    QCOMPARE(mapped.value(QStringLiteral("en")).asVariant().toString(), QStringLiteral("Hello"));
    QCOMPARE(mapped.value(QStringLiteral("ja")).asVariant().toString(), QString::fromUtf8("こんにちは"));
}

void KisMetaDataValueContractTest::updatesVariantsAndStructureFields()
{
    KisMetaData::Value invalid;
    QVERIFY(invalid.setVariant(QVariant(11)));
    QCOMPARE(invalid.type(), KisMetaData::Value::Variant);
    QCOMPARE(invalid.asVariant(), QVariant(11));

    KisMetaData::Value integer(QVariant(4));
    QVERIFY(integer.setVariant(QVariant(9)));
    QCOMPARE(integer.asVariant(), QVariant(9));

    KisMetaData::Value structure{QMap<QString, KisMetaData::Value>()};
    QVERIFY(structure.setStructureVariant(QStringLiteral("answer"), QVariant(42)));
    QCOMPARE(structure.asStructure().value(QStringLiteral("answer")).asVariant(), QVariant(42));
    QVERIFY(!integer.setStructureVariant(QStringLiteral("answer"), QVariant(1)));
}

void KisMetaDataValueContractTest::reportsSuccessForRejectedVariantType()
{
    KisMetaData::Value value(QVariant(7));

    QVERIFY(value.setVariant(QVariant(QStringLiteral("replacement"))));
    QCOMPARE(value.asVariant(), QVariant(7));
}

void KisMetaDataValueContractTest::reportsFailureAfterUpdatingArray()
{
    KisMetaData::Value value(QList<KisMetaData::Value>{variantValue(1)});

    QVERIFY(!value.setArrayVariant(2, QVariant(9)));
    const QList<KisMetaData::Value> result = value.asArray();
    QCOMPARE(result.size(), 3);
    QCOMPARE(result.at(0).asVariant(), QVariant(1));
    QCOMPARE(result.at(1).type(), KisMetaData::Value::Invalid);
    QCOMPARE(result.at(2).asVariant(), QVariant(9));

    KisMetaData::Value scalar(QVariant(1));
    QVERIFY(!scalar.setArrayVariant(0, QVariant(2)));
    QCOMPARE(scalar.asVariant(), QVariant(1));
}

void KisMetaDataValueContractTest::addsVariantsArraysAndRationals()
{
    KisMetaData::Value integer(QVariant(7));
    KisMetaData::Value &integerResult = (integer += variantValue(5));
    QCOMPARE(&integerResult, &integer);
    QCOMPARE(integer.asVariant(), QVariant(12));

    KisMetaData::Value array(QList<KisMetaData::Value>{variantValue(1)});
    array += variantValue(2);
    array += KisMetaData::Value(QList<KisMetaData::Value>{variantValue(3)});
    QCOMPARE(array.asArray().size(), 3);
    QCOMPARE(array.asArray().at(0).asVariant(), QVariant(1));
    QCOMPARE(array.asArray().at(1).asVariant(), QVariant(2));
    QCOMPARE(array.asArray().at(2).asVariant(), QVariant(3));

    KisMetaData::Value rational(KisMetaData::Rational(1, 2));
    rational += KisMetaData::Value(KisMetaData::Rational(1, 3));
    QCOMPARE(rational.asRational(), KisMetaData::Rational(5, 6));
}

void KisMetaDataValueContractTest::collapsesDateTimeAdditionToDate()
{
    const QDateTime earlier(QDate(2025, 2, 3), QTime(4, 5), QTimeZone::UTC);
    const QDateTime later(QDate(2026, 7, 8), QTime(9, 10), QTimeZone::UTC);
    KisMetaData::Value value{QVariant(earlier)};

    value += KisMetaData::Value(QVariant(later));

    QCOMPARE(value.asVariant().typeId(), static_cast<int>(QMetaType::QDate));
    QCOMPARE(value.asVariant().toDate(), later.date());
}

void KisMetaDataValueContractTest::usesOffByOneMillisecondCarryWhenAddingTimes()
{
    KisMetaData::Value value(QVariant(QTime(0, 0, 0, 700)));

    value += KisMetaData::Value(QVariant(QTime(0, 0, 0, 500)));

    QCOMPARE(value.asVariant().toTime(), QTime(0, 0, 1, 201));
}

void KisMetaDataValueContractTest::treatsDistinctLanguageArraysAsEqual()
{
    const KisMetaData::Value english(
        QList<KisMetaData::Value>{languageValue(QStringLiteral("en"), QStringLiteral("Hello"))},
        KisMetaData::Value::LangArray);
    const KisMetaData::Value japanese(
        QList<KisMetaData::Value>{languageValue(QStringLiteral("ja"), QString::fromUtf8("こんにちは"))},
        KisMetaData::Value::LangArray);

    QVERIFY(english == japanese);
}

void KisMetaDataValueContractTest::formatsSafeValueCategoriesAndDebugOutput()
{
    const KisMetaData::Value invalid;
    const KisMetaData::Value variant(QVariant(QStringLiteral("metadata")));
    const KisMetaData::Value array(
        QList<KisMetaData::Value>{variantValue(QStringLiteral("a")), variantValue(QStringLiteral("b"))});
    const KisMetaData::Value rational(KisMetaData::Rational(3, 4));
    const KisMetaData::Value structure(QMap<QString, KisMetaData::Value>{
        {QStringLiteral("answer"), variantValue(42)},
    });

    QVERIFY(!invalid.toString().isEmpty());
    QCOMPARE(variant.toString(), QStringLiteral("metadata"));
    QCOMPARE(array.toString(), QStringLiteral("[2]{ a, b }"));
    QCOMPARE(rational.toString(), QStringLiteral("3 / 4"));

    QString debugText;
    {
        QDebug debug(&debugText);
        debug << structure;
    }
    QVERIFY(debugText.contains(QStringLiteral("Structure:")));
    QVERIFY(debugText.contains(QStringLiteral("answer")));
    QVERIFY(debugText.contains(QStringLiteral("42")));
}

QTEST_GUILESS_MAIN(KisMetaDataValueContractTest)

#include "KisMetaDataValueContractTest.moc"
