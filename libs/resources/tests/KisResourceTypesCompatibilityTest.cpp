/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceTypes.h"

#include <QTest>

class KisResourceTypesCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceTypeKeysRemainCompatibleWithSavedResources_data();
    void resourceTypeKeysRemainCompatibleWithSavedResources();
    void resourceTypesResolveToDisplayNames_data();
    void resourceTypesResolveToDisplayNames();
};

void KisResourceTypesCompatibilityTest::resourceTypeKeysRemainCompatibleWithSavedResources_data()
{
    QTest::addColumn<QString>("actual");
    QTest::addColumn<QString>("expected");

    QTest::newRow("PaintOpPresets") << ResourceType::PaintOpPresets << QStringLiteral("paintoppresets");
    QTest::newRow("Brushes") << ResourceType::Brushes << QStringLiteral("brushes");
    QTest::newRow("Gradients") << ResourceType::Gradients << QStringLiteral("gradients");
    QTest::newRow("Palettes") << ResourceType::Palettes << QStringLiteral("palettes");
    QTest::newRow("Patterns") << ResourceType::Patterns << QStringLiteral("patterns");
    QTest::newRow("Workspaces") << ResourceType::Workspaces << QStringLiteral("workspaces");
    QTest::newRow("Symbols") << ResourceType::Symbols << QStringLiteral("symbols");
    QTest::newRow("WindowLayouts") << ResourceType::WindowLayouts << QStringLiteral("windowlayouts");
    QTest::newRow("Sessions") << ResourceType::Sessions << QStringLiteral("sessions");
    QTest::newRow("GamutMasks") << ResourceType::GamutMasks << QStringLiteral("gamutmasks");
    QTest::newRow("SeExprScripts") << ResourceType::SeExprScripts << QStringLiteral("seexpr_scripts");
    QTest::newRow("TaskSets") << ResourceType::TaskSets << QStringLiteral("tasksets");
    QTest::newRow("LayerStyles") << ResourceType::LayerStyles << QStringLiteral("layerstyles");
    QTest::newRow("FontFamilies") << ResourceType::FontFamilies << QStringLiteral("fontfamilies");
    QTest::newRow("CssStyles") << ResourceType::CssStyles << QStringLiteral("css_styles");
}

// Compatibility requirement: Existing resource bundles, resource tags, and resourcecache.sqlite entries depend on stable resource type keys.
void KisResourceTypesCompatibilityTest::resourceTypeKeysRemainCompatibleWithSavedResources()
{
    // Consumer: Users opening existing resource bundles, tags, and resource-cache databases.
    // Operation: LibrePaint resolves resource records using their saved type key.
    // Observable result: Every supported resource type keeps its established saved key.
    // Failure impact: Saved resources can be omitted, grouped under the wrong type, or fail to load.
    QFETCH(QString, actual);
    QFETCH(QString, expected);

    QCOMPARE(actual, expected);
}

void KisResourceTypesCompatibilityTest::resourceTypesResolveToDisplayNames_data()
{
    QTest::addColumn<QString>("resourceType");
    QTest::addColumn<QString>("expectedName");

    QTest::newRow("PaintOpPresets") << ResourceType::PaintOpPresets << ResourceName::PaintOpPresets.toString();
    QTest::newRow("Brushes") << ResourceType::Brushes << ResourceName::Brushes.toString();
    QTest::newRow("Gradients") << ResourceType::Gradients << ResourceName::Gradients.toString();
    QTest::newRow("Palettes") << ResourceType::Palettes << ResourceName::Palettes.toString();
    QTest::newRow("Patterns") << ResourceType::Patterns << ResourceName::Patterns.toString();
    QTest::newRow("Workspaces") << ResourceType::Workspaces << ResourceName::Workspaces.toString();
    QTest::newRow("Symbols") << ResourceType::Symbols << ResourceName::Symbols.toString();
    QTest::newRow("WindowLayouts") << ResourceType::WindowLayouts << ResourceName::WindowLayouts.toString();
    QTest::newRow("Sessions") << ResourceType::Sessions << ResourceName::Sessions.toString();
    QTest::newRow("GamutMasks") << ResourceType::GamutMasks << ResourceName::GamutMasks.toString();
    QTest::newRow("SeExprScripts") << ResourceType::SeExprScripts << ResourceName::SeExprScripts.toString();
    QTest::newRow("TaskSets") << ResourceType::TaskSets << ResourceName::TaskSets.toString();
    QTest::newRow("LayerStyles") << ResourceType::LayerStyles << ResourceName::LayerStyles.toString();
    QTest::newRow("FontFamilies") << ResourceType::FontFamilies << ResourceName::FontFamilies.toString();
    QTest::newRow("CssStyles") << ResourceType::CssStyles << ResourceName::CssStyles.toString();
}

void KisResourceTypesCompatibilityTest::resourceTypesResolveToDisplayNames()
{
    // Consumer: Users choosing a resource type in a resource-management view.
    // Operation: The view resolves a stored resource type to its display name.
    // Observable result: Every supported type resolves to its non-empty matching display name.
    // Failure impact: A resource type is missing or mislabeled in the chooser.
    QFETCH(QString, resourceType);
    QFETCH(QString, expectedName);

    QVERIFY(!expectedName.isEmpty());
    QCOMPARE(ResourceName::resourceTypeToName(resourceType), expectedName);
}

QTEST_GUILESS_MAIN(KisResourceTypesCompatibilityTest)

#include "KisResourceTypesCompatibilityTest.moc"
