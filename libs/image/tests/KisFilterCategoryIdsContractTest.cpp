/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filter/kis_filter_category_ids.h"

#include <QSet>
#include <QTest>

#include <array>

namespace
{

struct ExpectedCategory {
    const KoID *category;
    const char *identifier;
};

constexpr std::array<ExpectedCategory, 9> expectedCategories{{
    {&FiltersCategoryAdjustId, "adjust_filters"},
    {&FiltersCategoryArtisticId, "artistic_filters"},
    {&FiltersCategoryBlurId, "blur_filters"},
    {&FiltersCategoryColorId, "color_filters"},
    {&FiltersCategoryEdgeDetectionId, "edge_filters"},
    {&FiltersCategoryEmbossId, "emboss_filters"},
    {&FiltersCategoryEnhanceId, "enhance_filters"},
    {&FiltersCategoryMapId, "map_filters"},
    {&FiltersCategoryOtherId, "other_filters"},
}};

} // namespace

class KisFilterCategoryIdsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void categoriesPreserveUniqueStableIdentifiersAndDisplayNames();
};

void KisFilterCategoryIdsContractTest::categoriesPreserveUniqueStableIdentifiersAndDisplayNames()
{
    QSet<QString> identifiers;

    for (const ExpectedCategory &expected : expectedCategories) {
        const QString identifier = QString::fromLatin1(expected.identifier);
        QCOMPARE(expected.category->id(), identifier);
        QVERIFY2(!expected.category->name().isEmpty(),
                 qPrintable(QStringLiteral("category %1 has an empty display name").arg(identifier)));
        QVERIFY2(!identifiers.contains(identifier),
                 qPrintable(QStringLiteral("duplicate category identifier: %1").arg(identifier)));
        identifiers.insert(identifier);
    }

    QCOMPARE(identifiers.size(), static_cast<qsizetype>(expectedCategories.size()));
}

QTEST_GUILESS_MAIN(KisFilterCategoryIdsContractTest)

#include "KisFilterCategoryIdsContractTest.moc"
