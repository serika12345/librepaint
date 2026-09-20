/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <text/KoSvgTextShapeMarkupConverter.h>

#include <QTextDocument>
#include <QTextFrame>
#include <QTest>

class KoSvgTextWrappingContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgWrappingSurvivesEditingRoundTrip_data();
    void svgWrappingSurvivesEditingRoundTrip();
};

void KoSvgTextWrappingContractTest::svgWrappingSurvivesEditingRoundTrip_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<KoSvgTextShapeMarkupConverter::WrappingMode>("wrappingMode");
    QTest::addColumn<bool>("hasInlineSize");
    QTest::addColumn<QString>("savedStyle");

    QTest::newRow("pre")
        << QStringLiteral("white-space: pre")
        << KoSvgTextShapeMarkupConverter::WrappingMode::WhiteSpacePre
        << false
        << QStringLiteral("white-space: pre");
    QTest::newRow("pre-wrap-with-inline-size")
        << QStringLiteral("white-space: pre-wrap;inline-size:42")
        << KoSvgTextShapeMarkupConverter::WrappingMode::WhiteSpacePreWrap
        << true
        << QStringLiteral("white-space: pre-wrap;inline-size:42");
    QTest::newRow("pre-wrap-without-inline-size")
        << QStringLiteral("white-space: pre-wrap")
        << KoSvgTextShapeMarkupConverter::WrappingMode::WhiteSpacePre
        << false
        << QStringLiteral("white-space: pre");
}

void KoSvgTextWrappingContractTest::svgWrappingSurvivesEditingRoundTrip()
{
    // Consumer: Artists editing imported SVG text in the text tool and PSD text conversion.
    // Operation: Load SVG text with a white-space declaration into a rich-text document and save it again.
    // Observable result: The effective wrapping mode and inline-size are preserved in the saved SVG.
    // Failure impact: Edited text changes line breaks or loses its intended wrapping width after saving.
    QFETCH(QString, style);
    QFETCH(KoSvgTextShapeMarkupConverter::WrappingMode, wrappingMode);
    QFETCH(bool, hasInlineSize);
    QFETCH(QString, savedStyle);

    KoSvgTextShapeMarkupConverter converter(nullptr);
    QTextDocument document;
    const QString source = QStringLiteral("<text style=\"%1\">Hello</text>").arg(style);

    QVERIFY(converter.convertSvgToDocument(source, &document));
    QCOMPARE(KoSvgTextShapeMarkupConverter::getWrappingMode(document.rootFrame()->frameFormat()), wrappingMode);

    const std::optional<double> inlineSize =
        KoSvgTextShapeMarkupConverter::getInlineSize(document.rootFrame()->frameFormat());
    QCOMPARE(inlineSize.has_value(), hasInlineSize);
    if (hasInlineSize) {
        QCOMPARE(*inlineSize, 42.0);
    }

    QString savedSvg;
    QVERIFY(converter.convertDocumentToSvg(&document, &savedSvg));
    QVERIFY(savedSvg.contains(savedStyle));
}

QTEST_MAIN(KoSvgTextWrappingContractTest)

#include "KoSvgTextWrappingContractTest.moc"
