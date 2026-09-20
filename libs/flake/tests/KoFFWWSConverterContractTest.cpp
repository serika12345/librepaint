/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoFFWWSConverter.h"

#include <QTest>

class KoFFWWSConverterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void genericCssFamiliesAreListedAndResolved();
    void unknownCssFamilyIsNotResolved();
};

void KoFFWWSConverterContractTest::genericCssFamiliesAreListedAndResolved()
{
    // Consumer: text-tool users selecting CSS generic font families.
    // Operation: register generic families and finalize the font-family grouping.
    // Observable result: the family picker can list and resolve each CSS family name.
    // Failure impact: text falls back to an unintended font or the family is unavailable.
    KoFFWWSConverter converter;
    converter.addGenericFamily(QStringLiteral("serif"));
    converter.addGenericFamily(QStringLiteral("sans-serif"));
    converter.sortIntoWWSFamilies();

    const QList<KoFontFamilyWWSRepresentation> families = converter.collectFamilies();
    QCOMPARE(families.size(), 2);

    const auto serif = converter.representationByFamilyName(QStringLiteral("serif"));
    QVERIFY(serif.has_value());
    QCOMPARE(serif->fontFamilyName, QStringLiteral("serif"));
    QVERIFY(!serif->styles.isEmpty());

    const auto sansSerif = converter.wwsNameByFamilyName(QStringLiteral("sans-serif"));
    QVERIFY(sansSerif.has_value());
    QCOMPARE(*sansSerif, QStringLiteral("sans-serif"));
}

void KoFFWWSConverterContractTest::unknownCssFamilyIsNotResolved()
{
    // Consumer: text-tool users entering a font family that is not available.
    // Operation: resolve an unknown CSS family name after the font list is built.
    // Observable result: no family representation or WWS name is returned.
    // Failure impact: an unavailable family silently selects an unrelated installed font.
    KoFFWWSConverter converter;
    converter.addGenericFamily(QStringLiteral("serif"));
    converter.sortIntoWWSFamilies();

    QVERIFY(!converter.representationByFamilyName(QStringLiteral("missing-family")).has_value());
    QVERIFY(!converter.wwsNameByFamilyName(QStringLiteral("missing-family")).has_value());
}

QTEST_APPLESS_MAIN(KoFFWWSConverterContractTest)

#include "KoFFWWSConverterContractTest.moc"
