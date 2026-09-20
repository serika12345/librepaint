/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDocumentResourceManager.h>
#include <SvgLoadingContext.h>
#include <text/KoSvgTextProperties.h>

#include <QFont>
#include <QTest>

class KoSvgTextFontStretchContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cssFontStretchSurvivesSvgImportAndExport_data();
    void cssFontStretchSurvivesSvgImportAndExport();
};

void KoSvgTextFontStretchContractTest::cssFontStretchSurvivesSvgImportAndExport_data()
{
    QTest::addColumn<QString>("cssValue");
    QTest::addColumn<int>("fontStretch");

    QTest::newRow("ultra-condensed") << QStringLiteral("ultra-condensed") << int(QFont::UltraCondensed);
    QTest::newRow("extra-condensed") << QStringLiteral("extra-condensed") << int(QFont::ExtraCondensed);
    QTest::newRow("condensed") << QStringLiteral("condensed") << int(QFont::Condensed);
    QTest::newRow("semi-condensed") << QStringLiteral("semi-condensed") << int(QFont::SemiCondensed);
    QTest::newRow("normal") << QStringLiteral("normal") << int(QFont::Unstretched);
    QTest::newRow("semi-expanded") << QStringLiteral("semi-expanded") << int(QFont::SemiExpanded);
    QTest::newRow("expanded") << QStringLiteral("expanded") << int(QFont::Expanded);
    QTest::newRow("extra-expanded") << QStringLiteral("extra-expanded") << int(QFont::ExtraExpanded);
    QTest::newRow("ultra-expanded") << QStringLiteral("ultra-expanded") << int(QFont::UltraExpanded);
}

void KoSvgTextFontStretchContractTest::cssFontStretchSurvivesSvgImportAndExport()
{
    // Consumer: Artists who import and save SVG text with a CSS font-stretch declaration.
    // Operation: Read one CSS font-stretch keyword and serialize the text properties again.
    // Observable result: The resolved Qt font stretch and written CSS keyword retain the selected width.
    // Failure impact: Saving an SVG changes the width of text when it is reopened or used by another application.
    QFETCH(QString, cssValue);
    QFETCH(int, fontStretch);

    KoDocumentResourceManager resources;
    SvgLoadingContext context(&resources);
    context.pushGraphicsContext();

    KoSvgTextProperties properties;
    properties.parseSvgTextAttribute(context, QStringLiteral("font-stretch"), cssValue);

    QCOMPARE(properties.property(KoSvgTextProperties::FontStretchId).toInt(), fontStretch);
    QCOMPARE(properties.convertToSvgTextAttributes().value(QStringLiteral("font-stretch")), cssValue);

    context.popGraphicsContext();
}

QTEST_GUILESS_MAIN(KoSvgTextFontStretchContractTest)

#include "KoSvgTextFontStretchContractTest.moc"
