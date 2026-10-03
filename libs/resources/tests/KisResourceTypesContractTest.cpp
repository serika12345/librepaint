/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceTypes.h"

#include <KLocalizedString>
#include <QTest>

class KisResourceTypesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceTypesResolveToDisplayNames_data();
    void resourceTypesResolveToDisplayNames();
};

void KisResourceTypesContractTest::resourceTypesResolveToDisplayNames_data()
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

void KisResourceTypesContractTest::resourceTypesResolveToDisplayNames()
{
    // Consumer: resource-type selection dialogs, bundle summaries, and missing-resource warnings.
    // Operation: Resolve a resource type key to its display name.
    // Observable result: Every supported type resolves to its non-empty matching display name.
    // Failure impact: A resource type is missing or mislabeled in the chooser, bundle summary, or warning.
    QFETCH(QString, resourceType);
    QFETCH(QString, expectedName);

    QVERIFY(!expectedName.isEmpty());
    QCOMPARE(ResourceName::resourceTypeToName(resourceType), expectedName);
}

QTEST_GUILESS_MAIN(KisResourceTypesContractTest)

#include "KisResourceTypesContractTest.moc"
