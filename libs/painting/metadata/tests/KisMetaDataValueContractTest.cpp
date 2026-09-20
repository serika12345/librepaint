/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_value.h"

#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QTest>
#include <QTime>
#include <QVariant>

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
    void metadataEditorUpdatesValuesAndReportsRejectedEdits();
    void copiedMetadataValuesKeepDocumentEditsIndependent();
    void metadataMergingPreservesTimestampAndNormalizesTimeCarry();
    void localizedMetadataKeepsEachLanguageValueDistinct();
};

// Consumer: The Metadata Editor.
// Operation: Edit scalar, structure, and ordered-array metadata fields.
// Observable result: Compatible edits update the selected field and report success; rejected edits preserve the existing value and report failure.
// Failure impact: A user can lose metadata or see an edit reported as saved when the document retains a different value.
void KisMetaDataValueContractTest::metadataEditorUpdatesValuesAndReportsRejectedEdits()
{
    KisMetaData::Value scalar(QVariant(7));
    QVERIFY(!scalar.setVariant(QVariant(QStringLiteral("seven"))));
    QCOMPARE(scalar.asVariant(), QVariant(7));
    QVERIFY(scalar.setVariant(QVariant(9)));
    QCOMPARE(scalar.asVariant(), QVariant(9));

    KisMetaData::Value structure(QMap<QString, KisMetaData::Value>{
        {QStringLiteral("caption"), variantValue(QStringLiteral("before"))},
    });
    QVERIFY(structure.setStructureVariant(QStringLiteral("caption"), QVariant(QStringLiteral("after"))));
    QCOMPARE(structure.asStructure().value(QStringLiteral("caption")).asVariant(),
             QVariant(QStringLiteral("after")));

    KisMetaData::Value orderedArray(QList<KisMetaData::Value>{variantValue(QStringLiteral("first"))});
    QVERIFY(orderedArray.setArrayVariant(2, QVariant(QStringLiteral("third"))));
    QVERIFY(!orderedArray.setArrayVariant(-1, QVariant(QStringLiteral("ignored"))));
    const QList<KisMetaData::Value> values = orderedArray.asArray();
    QCOMPARE(values.size(), 3);
    QCOMPARE(values.at(0).asVariant(), QVariant(QStringLiteral("first")));
    QCOMPARE(values.at(1).type(), KisMetaData::Value::Invalid);
    QCOMPARE(values.at(2).asVariant(), QVariant(QStringLiteral("third")));
}

// Consumer: Document duplication and metadata-store copying.
// Operation: Edit a copied metadata value.
// Observable result: The copied value changes while the source document value remains unchanged.
// Failure impact: Editing metadata in one document can silently alter the metadata shown or saved by another document.
void KisMetaDataValueContractTest::copiedMetadataValuesKeepDocumentEditsIndependent()
{
    KisMetaData::Value source(QVariant(QStringLiteral("original")));
    KisMetaData::Value copied(source);

    QVERIFY(copied.setVariant(QVariant(QStringLiteral("copy"))));

    QCOMPARE(source.asVariant(), QVariant(QStringLiteral("original")));
    QCOMPARE(copied.asVariant(), QVariant(QStringLiteral("copy")));
}

// Consumer: Metadata merge strategies used when combining document content.
// Operation: Merge date-time and time metadata values.
// Observable result: The latest complete timestamp is retained and millisecond overflow carries exactly once into the next second.
// Failure impact: Merged documents can save an incorrect date or a time that is one millisecond away from the source values.
void KisMetaDataValueContractTest::metadataMergingPreservesTimestampAndNormalizesTimeCarry()
{
    const QDateTime earlier(QDate(2025, 2, 3), QTime(4, 5), QTimeZone::UTC);
    const QDateTime later(QDate(2026, 7, 8), QTime(9, 10), QTimeZone::UTC);
    KisMetaData::Value timestamp{QVariant(earlier)};
    timestamp += KisMetaData::Value(QVariant(later));
    QCOMPARE(timestamp.asVariant(), QVariant(later));

    KisMetaData::Value time(QVariant(QTime(0, 0, 0, 700)));
    time += KisMetaData::Value(QVariant(QTime(0, 0, 0, 500)));
    QCOMPARE(time.asVariant().toTime(), QTime(0, 0, 1, 200));
}

// Consumer: XMP import/export and localized metadata views.
// Operation: Read language-qualified values after loading metadata.
// Observable result: Each locale resolves to its own text and distinct locale values are not considered identical.
// Failure impact: Export or merge can replace one language's title or description with another language's text.
void KisMetaDataValueContractTest::localizedMetadataKeepsEachLanguageValueDistinct()
{
    const KisMetaData::Value english = languageValue(QStringLiteral("en"), QStringLiteral("Hello"));
    const KisMetaData::Value japanese = languageValue(QStringLiteral("ja"), QString::fromUtf8("こんにちは"));
    const KisMetaData::Value localized(QList<KisMetaData::Value>{english, japanese},
                                       KisMetaData::Value::LangArray);
    const KisMetaData::Value localizedWithFrench(QList<KisMetaData::Value>{
                                                       languageValue(QStringLiteral("fr"), QStringLiteral("Hello")),
                                                       japanese,
                                                   },
                                                   KisMetaData::Value::LangArray);

    const QMap<QString, KisMetaData::Value> values = localized.asLangArray();
    QCOMPARE(values.value(QStringLiteral("en")).asVariant(), QVariant(QStringLiteral("Hello")));
    QCOMPARE(values.value(QStringLiteral("ja")).asVariant(), QVariant(QString::fromUtf8("こんにちは")));
    QVERIFY(!(localized == localizedWithFrench));
}

QTEST_GUILESS_MAIN(KisMetaDataValueContractTest)

#include "KisMetaDataValueContractTest.moc"
