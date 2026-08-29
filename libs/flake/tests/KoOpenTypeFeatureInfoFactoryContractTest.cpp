/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoOpenTypeFeatureInfoFactory.h"

#include <QSet>
#include <QTest>

#include <algorithm>

class KoOpenTypeFeatureInfoFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openTypeTableValuesAreStable();
    void defaultAndValueConstructionRetainMetadata();
    void factoryReturnsKnownAndIndependentMetadata();
    void unknownTagReturnsGenericMetadata();
    void tagsAreSortedUniqueAndContainGeneratedRanges();
    void duplicateChwsKeepsLaterRegistrationDefect();
};

void KoOpenTypeFeatureInfoFactoryContractTest::openTypeTableValuesAreStable()
{
    using Table = KoOpenTypeFeatureInfo::OpenTypeTable;

    const QVector<Table> tables{
        Table::GSUB1,
        Table::GSUB2,
        Table::GSUB3,
        Table::GSUB4,
        Table::GSUB5,
        Table::GSUB6,
        Table::GSUB7,
        Table::GSUB8,
        Table::GPOS1,
        Table::GPOS2,
        Table::GPOS3,
        Table::GPOS4,
        Table::GPOS5,
        Table::GPOS6,
        Table::GPOS7,
        Table::GPOS8,
        Table::GPOS9,
    };

    for (int i = 0; i < tables.size(); ++i) {
        QCOMPARE(static_cast<int>(tables.at(i)), i);
    }
}

void KoOpenTypeFeatureInfoFactoryContractTest::defaultAndValueConstructionRetainMetadata()
{
    KoOpenTypeFeatureInfo defaults;
    QCOMPARE(defaults.tag, QByteArray());
    QCOMPARE(defaults.name, QString());
    QCOMPARE(defaults.description, QString());
    QCOMPARE(defaults.sample, QString());
    QCOMPARE(defaults.namedParameters, QStringList());
    QCOMPARE(defaults.tables, QVector<KoOpenTypeFeatureInfo::OpenTypeTable>());
    QVERIFY(!defaults.glyphPalette);

    const QVector<KoOpenTypeFeatureInfo::OpenTypeTable> tables{
        KoOpenTypeFeatureInfo::GSUB3,
        KoOpenTypeFeatureInfo::GPOS7,
    };
    KoOpenTypeFeatureInfo info(QByteArrayLiteral("cv42"),
                               QStringLiteral("Variant name"),
                               QStringLiteral("Variant description"),
                               tables,
                               true,
                               19);
    info.sample = QStringLiteral("A variant sample");
    info.namedParameters = {QStringLiteral("First"), QStringLiteral("Second")};

    QCOMPARE(info.tag, QByteArrayLiteral("cv42"));
    QCOMPARE(info.name, QStringLiteral("Variant name"));
    QCOMPARE(info.description, QStringLiteral("Variant description"));
    QCOMPARE(info.sample, QStringLiteral("A variant sample"));
    QCOMPARE(info.namedParameters, QStringList({QStringLiteral("First"), QStringLiteral("Second")}));
    QCOMPARE(info.tables, tables);
    QVERIFY(info.glyphPalette);
    QCOMPARE(info.maxValue, 19);
}

void KoOpenTypeFeatureInfoFactoryContractTest::factoryReturnsKnownAndIndependentMetadata()
{
    KoOpenTypeFeatureInfoFactory factory;

    KoOpenTypeFeatureInfo alternates = factory.infoByTag(QByteArrayLiteral("aalt"));
    QCOMPARE(alternates.tag, QByteArrayLiteral("aalt"));
    QVERIFY(!alternates.name.isEmpty());
    QVERIFY(!alternates.description.isEmpty());
    QCOMPARE(
        alternates.tables,
        QVector<KoOpenTypeFeatureInfo::OpenTypeTable>({KoOpenTypeFeatureInfo::GSUB1, KoOpenTypeFeatureInfo::GSUB3}));
    QVERIFY(alternates.glyphPalette);
    QCOMPARE(alternates.maxValue, 1);

    const KoOpenTypeFeatureInfo kerning = factory.infoByTag(QByteArrayLiteral("kern"));
    QCOMPARE(kerning.tables, QVector<KoOpenTypeFeatureInfo::OpenTypeTable>({KoOpenTypeFeatureInfo::GPOS2}));
    QVERIFY(!kerning.glyphPalette);

    const QString originalName = alternates.name;
    alternates.name.clear();
    alternates.tables.clear();

    const KoOpenTypeFeatureInfo freshAlternates = factory.infoByTag(QByteArrayLiteral("aalt"));
    QCOMPARE(freshAlternates.name, originalName);
    QCOMPARE(freshAlternates.tables.size(), 2);
}

void KoOpenTypeFeatureInfoFactoryContractTest::unknownTagReturnsGenericMetadata()
{
    KoOpenTypeFeatureInfoFactory factory;

    const KoOpenTypeFeatureInfo unknown = factory.infoByTag(QByteArrayLiteral("zzzz"));

    QCOMPARE(unknown.tag, QByteArrayLiteral("zzzz"));
    QCOMPARE(unknown.name, QString());
    QCOMPARE(unknown.description, QString());
    QCOMPARE(unknown.sample, QString());
    QCOMPARE(unknown.namedParameters, QStringList());
    QCOMPARE(unknown.tables, QVector<KoOpenTypeFeatureInfo::OpenTypeTable>());
    QVERIFY(!unknown.glyphPalette);
    QCOMPARE(unknown.maxValue, 1);
}

void KoOpenTypeFeatureInfoFactoryContractTest::tagsAreSortedUniqueAndContainGeneratedRanges()
{
    KoOpenTypeFeatureInfoFactory factory;
    const QList<QString> tags = factory.tags();

    QVERIFY(std::is_sorted(tags.cbegin(), tags.cend()));

    QSet<QString> uniqueTags;
    for (const QString &tag : tags) {
        uniqueTags.insert(tag);
    }
    QCOMPARE(uniqueTags.size(), tags.size());

    QVERIFY(tags.contains(QStringLiteral("aalt")));
    QVERIFY(tags.contains(QStringLiteral("kern")));
    QVERIFY(!tags.contains(QStringLiteral("zzzz")));

    for (int i = 1; i <= 20; ++i) {
        QVERIFY(tags.contains(QStringLiteral("ss%1").arg(i, 2, 10, QLatin1Char('0'))));
    }
    for (int i = 1; i <= 99; ++i) {
        QVERIFY(tags.contains(QStringLiteral("cv%1").arg(i, 2, 10, QLatin1Char('0'))));
    }
}

void KoOpenTypeFeatureInfoFactoryContractTest::duplicateChwsKeepsLaterRegistrationDefect()
{
    KoOpenTypeFeatureInfoFactory factory;

    const KoOpenTypeFeatureInfo contextualHalfWidth = factory.infoByTag(QByteArrayLiteral("chws"));

    QCOMPARE(contextualHalfWidth.tables, QVector<KoOpenTypeFeatureInfo::OpenTypeTable>({KoOpenTypeFeatureInfo::GPOS1}));
    QVERIFY(contextualHalfWidth.glyphPalette);
    QCOMPARE(factory.tags().count(QStringLiteral("chws")), 1);
}

QTEST_GUILESS_MAIN(KoOpenTypeFeatureInfoFactoryContractTest)

#include "KoOpenTypeFeatureInfoFactoryContractTest.moc"
