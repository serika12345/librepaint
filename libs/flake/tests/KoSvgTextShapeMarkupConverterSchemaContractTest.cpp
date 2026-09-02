/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoSvgTextShapeMarkupConverter.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_CONVERTER_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoSvgTextShapeMarkupConverter::method)), signature>)
} // namespace

class KoSvgTextShapeMarkupConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgTextMarkupConverterIdentityAndWrappingSchemaRemainStable();
    void svgTextMarkupDocumentPropertySchemaRemainStable();
    void svgTextMarkupSvgAndHtmlConversionSignaturesRemainStable();
    void svgTextMarkupDocumentConversionAndDiagnosticSignaturesRemainStable();
    void svgTextMarkupStyleConversionSignaturesRemainStable();
};

void KoSvgTextShapeMarkupConverterSchemaContractTest::svgTextMarkupConverterIdentityAndWrappingSchemaRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;
    using WrappingMode = Converter::WrappingMode;

    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_constructible_v<Converter, KoSvgTextShape *>);
    static_assert(std::is_destructible_v<Converter>);
    static_assert(std::is_enum_v<WrappingMode>);

    QCOMPARE(static_cast<int>(WrappingMode::QtLegacy), 0);
    QCOMPARE(static_cast<int>(WrappingMode::WhiteSpacePre), 1);
    QCOMPARE(static_cast<int>(WrappingMode::WhiteSpacePreWrap), 2);
}

void KoSvgTextShapeMarkupConverterSchemaContractTest::svgTextMarkupDocumentPropertySchemaRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;

    static_assert(std::is_same_v<std::remove_cv_t<decltype(Converter::WrappingModeProperty)>, QTextFormat::Property>);
    static_assert(std::is_same_v<std::remove_cv_t<decltype(Converter::InlineSizeProperty)>, QTextFormat::Property>);
    QCOMPARE(static_cast<int>(Converter::WrappingModeProperty), static_cast<int>(QTextFormat::UserProperty) + 56784);
    QCOMPARE(static_cast<int>(Converter::InlineSizeProperty), static_cast<int>(Converter::WrappingModeProperty) + 1);

    static_assert(
        std::is_same_v<decltype(&Converter::getWrappingMode), Converter::WrappingMode (*)(const QTextFrameFormat &)>);
    static_assert(
        std::is_same_v<decltype(&Converter::setWrappingMode), void (*)(QTextFrameFormat *, Converter::WrappingMode)>);
    static_assert(
        std::is_same_v<decltype(&Converter::getInlineSize), std::optional<double> (*)(const QTextFrameFormat &)>);
    static_assert(std::is_same_v<decltype(&Converter::setInlineSize), void (*)(QTextFrameFormat *, double)>);
}

void KoSvgTextShapeMarkupConverterSchemaContractTest::svgTextMarkupSvgAndHtmlConversionSignaturesRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;

    ASSERT_CONVERTER_SIGNATURE(convertToSvg, bool (Converter::*)(QString *, QString *));
    ASSERT_CONVERTER_SIGNATURE(convertFromSvg,
                               bool (Converter::*)(const QString &, const QString &, const QRectF &, qreal));
    ASSERT_CONVERTER_SIGNATURE(convertToHtml, bool (Converter::*)(QString *));
    ASSERT_CONVERTER_SIGNATURE(convertFromHtml, bool (Converter::*)(const QString &, QString *, QString *));
}

void KoSvgTextShapeMarkupConverterSchemaContractTest::
    svgTextMarkupDocumentConversionAndDiagnosticSignaturesRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;

    ASSERT_CONVERTER_SIGNATURE(convertDocumentToSvg, bool (Converter::*)(const QTextDocument *, QString *));
    ASSERT_CONVERTER_SIGNATURE(convertSvgToDocument, bool (Converter::*)(const QString &, QTextDocument *));
    ASSERT_CONVERTER_SIGNATURE(errors, QStringList (Converter::*)() const);
    ASSERT_CONVERTER_SIGNATURE(warnings, QStringList (Converter::*)() const);
}

void KoSvgTextShapeMarkupConverterSchemaContractTest::svgTextMarkupStyleConversionSignaturesRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;

    ASSERT_CONVERTER_SIGNATURE(style, QString (Converter::*)(QTextCharFormat, QTextBlockFormat, QTextCharFormat, bool));
    static_assert(std::is_same_v<
                  decltype(&Converter::stylesFromString),
                  QVector<QTextFormat> (*)(QStringList, QTextCharFormat, QTextBlockFormat, Converter::ExtraStyles &)>);
    ASSERT_CONVERTER_SIGNATURE(formatDifference, QTextFormat (Converter::*)(QTextFormat, QTextFormat));

    static_assert(std::is_same_v<decltype(std::declval<Converter &>().style(std::declval<QTextCharFormat>(),
                                                                            std::declval<QTextBlockFormat>())),
                                 QString>);
    static_assert(std::is_same_v<decltype(std::declval<Converter &>().style(std::declval<QTextCharFormat>(),
                                                                            std::declval<QTextBlockFormat>(),
                                                                            std::declval<QTextCharFormat>())),
                                 QString>);
}

QTEST_APPLESS_MAIN(KoSvgTextShapeMarkupConverterSchemaContractTest)

#include "KoSvgTextShapeMarkupConverterSchemaContractTest.moc"
