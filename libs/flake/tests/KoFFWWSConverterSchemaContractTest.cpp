/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoFFWWSConverter.h"

#include <QTest>
#include <QTimeZone>

#include <type_traits>
#include <utility>

namespace
{

using Converter = KoFFWWSConverter;
using Family = KoFontFamilyWWSRepresentation;
using FileEntry = KoFFWWSConverter::FontFileEntry;

} // namespace

class KoFFWWSConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fontFileAndFamilyIdentityValuesRemainStable();
    void fontFamilyLocalizationValuesRemainStable();
    void fontFamilyAxesStylesAndColorFlagsRemainStable();
    void fontConverterTypeAndIngestionSignaturesRemainStable();
    void fontConverterCollectionAndLookupSignaturesRemainStable();
};

void KoFFWWSConverterSchemaContractTest::fontFileAndFamilyIdentityValuesRemainStable()
{
    static_assert(std::is_aggregate_v<FileEntry>);
    static_assert(std::is_same_v<decltype(FileEntry::fileName), QString>);
    static_assert(std::is_same_v<decltype(FileEntry::fontIndex), int>);
    static_assert(std::is_aggregate_v<Family>);
    static_assert(std::is_same_v<decltype(Family::fontFamilyName), QString>);
    static_assert(std::is_same_v<decltype(Family::typographicFamilyName), QString>);
    static_assert(std::is_same_v<decltype(Family::type), KoSvgText::FontFormatType>);

    FileEntry defaultEntry{};
    QVERIFY(defaultEntry.fileName.isEmpty());
    QCOMPARE(defaultEntry.fontIndex, 0);

    defaultEntry.fileName = QStringLiteral("original.ttf");
    defaultEntry.fontIndex = 3;
    FileEntry copiedEntry(defaultEntry);
    FileEntry assignedEntry{};
    assignedEntry = defaultEntry;
    defaultEntry.fileName = QStringLiteral("changed.ttf");
    defaultEntry.fontIndex = 9;
    QCOMPARE(copiedEntry.fileName, QStringLiteral("original.ttf"));
    QCOMPARE(copiedEntry.fontIndex, 3);
    QCOMPARE(assignedEntry.fileName, QStringLiteral("original.ttf"));
    QCOMPARE(assignedEntry.fontIndex, 3);

    Family family;
    QVERIFY(family.fontFamilyName.isEmpty());
    QVERIFY(family.typographicFamilyName.isEmpty());
    QCOMPARE(int(family.type), int(KoSvgText::UnknownFontType));

    family.fontFamilyName = QStringLiteral("Original Family");
    family.typographicFamilyName = QStringLiteral("Original Typographic Family");
    family.type = KoSvgText::OpenTypeFontType;
    const Family copiedFamily(family);
    Family assignedFamily;
    assignedFamily = family;
    family.fontFamilyName = QStringLiteral("Changed Family");
    family.typographicFamilyName = QStringLiteral("Changed Typographic Family");
    family.type = KoSvgText::Type1FontType;
    QCOMPARE(copiedFamily.fontFamilyName, QStringLiteral("Original Family"));
    QCOMPARE(copiedFamily.typographicFamilyName, QStringLiteral("Original Typographic Family"));
    QCOMPARE(int(copiedFamily.type), int(KoSvgText::OpenTypeFontType));
    QCOMPARE(assignedFamily.fontFamilyName, QStringLiteral("Original Family"));
    QCOMPARE(assignedFamily.typographicFamilyName, QStringLiteral("Original Typographic Family"));
    QCOMPARE(int(assignedFamily.type), int(KoSvgText::OpenTypeFontType));
}

void KoFFWWSConverterSchemaContractTest::fontFamilyLocalizationValuesRemainStable()
{
    static_assert(std::is_same_v<decltype(Family::localizedFontFamilyNames), QHash<QLocale, QString>>);
    static_assert(std::is_same_v<decltype(Family::localizedTypographicFamily), QHash<QLocale, QString>>);
    static_assert(std::is_same_v<decltype(Family::localizedTypographicStyles), QHash<QLocale, QString>>);
    static_assert(std::is_same_v<decltype(Family::lastModified), QDateTime>);
    static_assert(std::is_same_v<decltype(Family::sampleStrings), QHash<QString, QString>>);
    static_assert(std::is_same_v<decltype(Family::supportedLanguages), QList<QLocale>>);

    Family family;
    QVERIFY(family.localizedFontFamilyNames.isEmpty());
    QVERIFY(family.localizedTypographicFamily.isEmpty());
    QVERIFY(family.localizedTypographicStyles.isEmpty());
    QVERIFY(!family.lastModified.isValid());
    QVERIFY(family.sampleStrings.isEmpty());
    QVERIFY(family.supportedLanguages.isEmpty());

    const QLocale english(QLocale::English);
    family.localizedFontFamilyNames.insert(english, QStringLiteral("Family"));
    family.localizedTypographicFamily.insert(english, QStringLiteral("Typographic Family"));
    family.localizedTypographicStyles.insert(english, QStringLiteral("Regular"));
    family.lastModified = QDateTime(QDate(2026, 1, 2), QTime(3, 4), QTimeZone::UTC);
    family.sampleStrings.insert(QStringLiteral("Latn"), QStringLiteral("Sample"));
    family.supportedLanguages.append(english);

    const Family copied(family);
    Family assigned;
    assigned = family;
    family.localizedFontFamilyNames[english] = QStringLiteral("Changed Family");
    family.localizedTypographicFamily[english] = QStringLiteral("Changed Typographic Family");
    family.localizedTypographicStyles[english] = QStringLiteral("Changed Style");
    family.lastModified = QDateTime();
    family.sampleStrings[QStringLiteral("Latn")] = QStringLiteral("Changed Sample");
    family.supportedLanguages.clear();

    QCOMPARE(copied.localizedFontFamilyNames.value(english), QStringLiteral("Family"));
    QCOMPARE(copied.localizedTypographicFamily.value(english), QStringLiteral("Typographic Family"));
    QCOMPARE(copied.localizedTypographicStyles.value(english), QStringLiteral("Regular"));
    QVERIFY(copied.lastModified.isValid());
    QCOMPARE(copied.sampleStrings.value(QStringLiteral("Latn")), QStringLiteral("Sample"));
    QCOMPARE(copied.supportedLanguages, QList<QLocale>{english});
    QCOMPARE(assigned.localizedFontFamilyNames.value(english), QStringLiteral("Family"));
    QCOMPARE(assigned.localizedTypographicFamily.value(english), QStringLiteral("Typographic Family"));
    QCOMPARE(assigned.localizedTypographicStyles.value(english), QStringLiteral("Regular"));
    QVERIFY(assigned.lastModified.isValid());
    QCOMPARE(assigned.sampleStrings.value(QStringLiteral("Latn")), QStringLiteral("Sample"));
    QCOMPARE(assigned.supportedLanguages, QList<QLocale>{english});
}

void KoFFWWSConverterSchemaContractTest::fontFamilyAxesStylesAndColorFlagsRemainStable()
{
    static_assert(std::is_same_v<decltype(Family::axes), QHash<QString, KoSvgText::FontFamilyAxis>>);
    static_assert(std::is_same_v<decltype(Family::styles), QList<KoSvgText::FontFamilyStyleInfo>>);
    static_assert(std::is_same_v<decltype(Family::isVariable), bool>);
    static_assert(std::is_same_v<decltype(Family::colorClrV0), bool>);
    static_assert(std::is_same_v<decltype(Family::colorClrV1), bool>);
    static_assert(std::is_same_v<decltype(Family::colorSVG), bool>);
    static_assert(std::is_same_v<decltype(Family::colorBitMap), bool>);

    Family family;
    QVERIFY(family.axes.isEmpty());
    QVERIFY(family.styles.isEmpty());
    QVERIFY(!family.isVariable);
    QVERIFY(!family.colorClrV0);
    QVERIFY(!family.colorClrV1);
    QVERIFY(!family.colorSVG);
    QVERIFY(!family.colorBitMap);

    KoSvgText::FontFamilyAxis axis(QStringLiteral("wght"), 625.0);
    KoSvgText::FontFamilyStyleInfo style;
    style.instanceCoords.insert(QStringLiteral("wght"), 625.0f);
    style.isItalic = true;
    family.axes.insert(QStringLiteral("wght"), axis);
    family.styles.append(style);
    family.isVariable = true;
    family.colorClrV0 = true;
    family.colorClrV1 = true;
    family.colorSVG = true;
    family.colorBitMap = true;

    const Family copied(family);
    Family assigned;
    assigned = family;
    family.axes[QStringLiteral("wght")].value = 200.0;
    family.styles[0].instanceCoords[QStringLiteral("wght")] = 200.0f;
    family.styles[0].isItalic = false;
    family.isVariable = false;
    family.colorClrV0 = false;
    family.colorClrV1 = false;
    family.colorSVG = false;
    family.colorBitMap = false;

    QCOMPARE(copied.axes.value(QStringLiteral("wght")).value, 625.0);
    QCOMPARE(copied.styles.constFirst().instanceCoords.value(QStringLiteral("wght")), 625.0f);
    QVERIFY(copied.styles.constFirst().isItalic);
    QVERIFY(copied.isVariable);
    QVERIFY(copied.colorClrV0);
    QVERIFY(copied.colorClrV1);
    QVERIFY(copied.colorSVG);
    QVERIFY(copied.colorBitMap);
    QCOMPARE(assigned.axes.value(QStringLiteral("wght")).value, 625.0);
    QCOMPARE(assigned.styles.constFirst().instanceCoords.value(QStringLiteral("wght")), 625.0f);
    QVERIFY(assigned.styles.constFirst().isItalic);
    QVERIFY(assigned.isVariable);
    QVERIFY(assigned.colorClrV0);
    QVERIFY(assigned.colorClrV1);
    QVERIFY(assigned.colorSVG);
    QVERIFY(assigned.colorBitMap);
}

void KoFFWWSConverterSchemaContractTest::fontConverterTypeAndIngestionSignaturesRemainStable()
{
    using AddPattern = bool (Converter::*)(const FcPattern *, FT_LibrarySP);
    using AddFile = bool (Converter::*)(const QString &, int, FT_LibrarySP);
    using AddLanguages = void (Converter::*)(const QString &, int, const QList<QLocale> &, FcCharSet *);
    using AddGenericFamily = void (Converter::*)(const QString &);

    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_default_constructible_v<Converter>);
    static_assert(std::is_destructible_v<Converter>);
    static_assert(std::is_same_v<decltype(&Converter::addFontFromPattern), AddPattern>);
    static_assert(std::is_same_v<decltype(&Converter::addFontFromFile), AddFile>);
    static_assert(std::is_same_v<decltype(&Converter::addSupportedLanguagesByFile), AddLanguages>);
    static_assert(std::is_same_v<decltype(&Converter::addGenericFamily), AddGenericFamily>);

    QVERIFY(true);
}

void KoFFWWSConverterSchemaContractTest::fontConverterCollectionAndLookupSignaturesRemainStable()
{
    using SortFamilies = void (Converter::*)();
    using CollectFamilies = QList<Family> (Converter::*)() const;
    using Representation = std::optional<Family> (Converter::*)(const QString &) const;
    using WwsName = std::optional<QString> (Converter::*)(QString) const;
    using Candidates = QVector<FileEntry> (Converter::*)(KoCSSFontInfo, quint32, quint32) const;
    using DebugInfo = void (Converter::*)() const;

    static_assert(std::is_same_v<decltype(&Converter::sortIntoWWSFamilies), SortFamilies>);
    static_assert(std::is_same_v<decltype(&Converter::collectFamilies), CollectFamilies>);
    static_assert(std::is_same_v<decltype(&Converter::representationByFamilyName), Representation>);
    static_assert(std::is_same_v<decltype(&Converter::wwsNameByFamilyName), WwsName>);
    static_assert(std::is_same_v<decltype(&Converter::candidatesForCssValues), Candidates>);
    static_assert(std::is_same_v<decltype(&Converter::debugInfo), DebugInfo>);
    static_assert(std::is_same_v<decltype(std::declval<const Converter &>().candidatesForCssValues(
                                     std::declval<KoCSSFontInfo>())),
                                 QVector<FileEntry>>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KoFFWWSConverterSchemaContractTest)

#include "KoFFWWSConverterSchemaContractTest.moc"
