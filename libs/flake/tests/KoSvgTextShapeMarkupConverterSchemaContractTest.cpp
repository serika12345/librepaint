/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoSvgTextShapeMarkupConverter.h>

#include <QTest>

#include <utility>

namespace
{
} // namespace

class KoSvgTextShapeMarkupConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgTextMarkupConverterIdentityAndWrappingSchemaRemainStable();
    void svgTextMarkupDocumentPropertySchemaRemainStable();
};

void KoSvgTextShapeMarkupConverterSchemaContractTest::svgTextMarkupConverterIdentityAndWrappingSchemaRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;
    using WrappingMode = Converter::WrappingMode;


    QCOMPARE(static_cast<int>(WrappingMode::QtLegacy), 0);
    QCOMPARE(static_cast<int>(WrappingMode::WhiteSpacePre), 1);
    QCOMPARE(static_cast<int>(WrappingMode::WhiteSpacePreWrap), 2);
}

void KoSvgTextShapeMarkupConverterSchemaContractTest::svgTextMarkupDocumentPropertySchemaRemainStable()
{
    using Converter = KoSvgTextShapeMarkupConverter;

    QCOMPARE(static_cast<int>(Converter::WrappingModeProperty), static_cast<int>(QTextFormat::UserProperty) + 56784);
    QCOMPARE(static_cast<int>(Converter::InlineSizeProperty), static_cast<int>(Converter::WrappingModeProperty) + 1);

}

QTEST_APPLESS_MAIN(KoSvgTextShapeMarkupConverterSchemaContractTest)

#include "KoSvgTextShapeMarkupConverterSchemaContractTest.moc"
