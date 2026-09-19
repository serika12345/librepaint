/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDocumentResourceManager.h>
#include <SvgLoadingContext.h>
#include <SvgStyleParser.h>
#include <text/KoSvgTextProperties.h>

#include <QDomDocument>
#include <QFont>
#include <QTest>

class KoSvgTextFontImportContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgFontStylesResolveForEditing();
};

void KoSvgTextFontImportContractTest::svgFontStylesResolveForEditing()
{
    // Consumer: Artists importing SVG text with font and text-decoration styles.
    // Operation: Parse font-family, width, weight, style, variant, and decoration declarations.
    // Observable result: The resolved text properties select the requested font styling and decoration.
    // Failure impact: Imported SVG text loses its intended typeface width, emphasis, or decoration.
    const QString data =
        "<text x=\"7\" y=\"7\""
        "    font-family=\"Verdana , 'Times New Roman', serif\" font-size=\"15\" font-style=\"oblique\" fill=\"blue\""
        "    font-stretch=\"extra-condensed\""
        "    font-size-adjust=\"0.56\""
        "    font=\"bold italic large Palatino, serif\""
        "    font-variant=\"small-caps\" font-weight=\"600\" >"
        "    Hello, out there"
        "</text>";

    QDomDocument doc;
    QVERIFY(doc.setContent(data.toLatin1()));

    KoDocumentResourceManager resourceManager;
    SvgLoadingContext context(&resourceManager);
    context.pushGraphicsContext();

    const SvgStyles styles = context.styleParser().collectStyles(doc.documentElement());
    context.styleParser().parseFont(styles);

    const auto resolvedFont = [&context]() {
        return context.resolvedProperties();
    };

    QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontFamiliesId).toStringList(),
             QStringList({QStringLiteral("Verdana"), QStringLiteral("Times New Roman"), QStringLiteral("serif")}));
    QCOMPARE(resolvedFont().fontSize().value, 15.0);
    QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontStyleId).value<KoSvgText::CssFontStyleData>(),
             KoSvgText::CssFontStyleData(QFont::StyleOblique));
    QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontVariantCapsId).toInt(), int(KoSvgText::CapsSmall));
    QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontWeightId).toInt(), 600);
    QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontStretchId).toInt(), int(QFont::ExtraCondensed));

    {
        SvgStyles fontModifier;
        fontModifier[QStringLiteral("font-weight")] = QStringLiteral("bolder");
        context.pushGraphicsContext();
        context.styleParser().parseFont(fontModifier);
        QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontWeightId).toInt(), 700);
        context.popGraphicsContext();
    }

    {
        SvgStyles fontModifier;
        fontModifier[QStringLiteral("font-weight")] = QStringLiteral("lighter");
        context.pushGraphicsContext();
        context.styleParser().parseFont(fontModifier);
        QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontWeightId).toInt(), 500);
        context.popGraphicsContext();
    }

    {
        SvgStyles fontModifier;
        fontModifier[QStringLiteral("font-stretch")] = QStringLiteral("narrower");
        context.pushGraphicsContext();
        context.styleParser().parseFont(fontModifier);
        QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontStretchId).toInt(), int(QFont::UltraCondensed));
        context.popGraphicsContext();
    }

    {
        SvgStyles fontModifier;
        fontModifier[QStringLiteral("font-stretch")] = QStringLiteral("wider");
        context.pushGraphicsContext();
        context.styleParser().parseFont(fontModifier);
        QCOMPARE(resolvedFont().property(KoSvgTextProperties::FontStretchId).toInt(), int(QFont::Condensed));
        context.popGraphicsContext();
    }

    const auto expectDecoration = [&context, &resolvedFont](const QString &declaration,
                                                               KoSvgText::TextDecoration decoration,
                                                               bool expected) {
        SvgStyles fontModifier;
        fontModifier[QStringLiteral("text-decoration")] = declaration;
        context.styleParser().parseFont(fontModifier);
        const KoSvgText::TextDecorations decorations =
            resolvedFont().property(KoSvgTextProperties::TextDecorationLineId).value<KoSvgText::TextDecorations>();
        QCOMPARE(decorations.testFlag(decoration), expected);
    };

    expectDecoration(QStringLiteral("underline"), KoSvgText::DecorationUnderline, true);
    expectDecoration(QStringLiteral("overline"), KoSvgText::DecorationOverline, true);
    expectDecoration(QStringLiteral("line-through"), KoSvgText::DecorationLineThrough, true);
    expectDecoration(QStringLiteral(" line-through overline"), KoSvgText::DecorationUnderline, false);
    expectDecoration(QStringLiteral(" line-through overline"), KoSvgText::DecorationLineThrough, true);
    expectDecoration(QStringLiteral(" line-through overline"), KoSvgText::DecorationOverline, true);

    context.popGraphicsContext();
}

QTEST_GUILESS_MAIN(KoSvgTextFontImportContractTest)

#include "KoSvgTextFontImportContractTest.moc"
