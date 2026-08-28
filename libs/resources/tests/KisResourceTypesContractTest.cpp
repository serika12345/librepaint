/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceTypes.h"
#include "ResourceDebug.h"

#include <QTest>

class KisResourceTypesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceTypesPreserveStorageKeys_data();
    void resourceTypesPreserveStorageKeys();
    void resourceSubTypesPreserveLoaderKeys_data();
    void resourceSubTypesPreserveLoaderKeys();
    void resourceNamesMapFromTypes_data();
    void resourceNamesMapFromTypes();
    void resourceLogPreservesCategoryAndDefaultSeverity();
};

void KisResourceTypesContractTest::resourceTypesPreserveStorageKeys_data()
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

void KisResourceTypesContractTest::resourceTypesPreserveStorageKeys()
{
    QFETCH(QString, actual);
    QFETCH(QString, expected);

    QCOMPARE(actual, expected);
}

void KisResourceTypesContractTest::resourceSubTypesPreserveLoaderKeys_data()
{
    QTest::addColumn<QString>("actual");
    QTest::addColumn<QString>("expected");

    QTest::newRow("AbrBrushes") << ResourceSubType::AbrBrushes << QStringLiteral("abr_brushes");
    QTest::newRow("GbrBrushes") << ResourceSubType::GbrBrushes << QStringLiteral("gbr_brushes");
    QTest::newRow("GihBrushes") << ResourceSubType::GihBrushes << QStringLiteral("gih_brushes");
    QTest::newRow("SvgBrushes") << ResourceSubType::SvgBrushes << QStringLiteral("svg_brushes");
    QTest::newRow("PngBrushes") << ResourceSubType::PngBrushes << QStringLiteral("png_brushes");
    QTest::newRow("SegmentedGradients") << ResourceSubType::SegmentedGradients << QStringLiteral("segmented_gradients");
    QTest::newRow("StopGradients") << ResourceSubType::StopGradients << QStringLiteral("stop_gradients");
    QTest::newRow("KritaPaintOpPresets") << ResourceSubType::KritaPaintOpPresets << QStringLiteral("krita_paintop_presets");
    QTest::newRow("MyPaintPaintOpPresets") << ResourceSubType::MyPaintPaintOpPresets << QStringLiteral("mypaint_paintop_presets");
}

void KisResourceTypesContractTest::resourceSubTypesPreserveLoaderKeys()
{
    QFETCH(QString, actual);
    QFETCH(QString, expected);

    QCOMPARE(actual, expected);
}

void KisResourceTypesContractTest::resourceNamesMapFromTypes_data()
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

void KisResourceTypesContractTest::resourceNamesMapFromTypes()
{
    QFETCH(QString, resourceType);
    QFETCH(QString, expectedName);

    QVERIFY(!expectedName.isEmpty());
    QCOMPARE(ResourceName::resourceTypeToName(resourceType), expectedName);
}

void KisResourceTypesContractTest::resourceLogPreservesCategoryAndDefaultSeverity()
{
    QLoggingCategory::setFilterRules(QString());

    const QLoggingCategory &first = RESOURCE_LOG();
    const QLoggingCategory &second = RESOURCE_LOG();
    QCOMPARE(&first, &second);
    QCOMPARE(QString::fromLatin1(first.categoryName()), QStringLiteral("krita.lib.resource"));
    QVERIFY(!first.isDebugEnabled());
    QVERIFY(first.isInfoEnabled());
    QVERIFY(first.isWarningEnabled());
    QVERIFY(first.isCriticalEnabled());
}

QTEST_GUILESS_MAIN(KisResourceTypesContractTest)

#include "KisResourceTypesContractTest.moc"
