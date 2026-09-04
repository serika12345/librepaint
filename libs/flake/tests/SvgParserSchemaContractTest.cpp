/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoDrag.h"
#include "KoPathShapeLoader.h"
#include "KoSvgPaste.h"
#include "svg/SvgParser.h"
#include "svg/SvgShape.h"

#include <QTest>
#include <QXmlStreamReader>

#include <type_traits>
#include <utility>

namespace
{
using Parser = SvgParser;
using FileFetcher = std::function<QByteArray(const QString &)>;
using MarkerList = QList<QExplicitlySharedDataPointer<KoMarker>>;
using ShapeList = QList<KoShape *>;
using SymbolList = QVector<KoSvgSymbol *>;

#define ASSERT_SVG_PARSER_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Parser::method)), signature>)
#define ASSERT_SVG_PARSER_STATIC_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Parser::method)), signature>)
#define ASSERT_SVG_EXCHANGE_SIGNATURE(type, method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
#define ASSERT_SVG_EXCHANGE_STATIC_SIGNATURE(type, method, signature)                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
} // namespace

class SvgParserSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgParserTypeLifetimeAndFileFetcherSchemaRemainStable();
    void svgParserDocumentCreationSignaturesRemainStable();
    void svgParserConfigurationSignaturesRemainStable();
    void svgParserShapeAndDefinitionSignaturesRemainStable();
    void svgParserMetadataAndWarningSignaturesRemainStable();
    void svgDragClipboardSchemaRemainStable();
    void svgPasteImportSchemaRemainStable();
    void svgPathShapeLoaderSchemaRemainStable();
    void svgShapeTypeAndLifetimeSchemaRemainStable();
    void svgShapePersistenceSignaturesRemainStable();
};

void SvgParserSchemaContractTest::svgParserTypeLifetimeAndFileFetcherSchemaRemainStable()
{
    static_assert(std::is_class_v<Parser>);
    static_assert(std::is_constructible_v<Parser, KoDocumentResourceManager *>);
    static_assert(std::has_virtual_destructor_v<Parser>);
    static_assert(std::is_same_v<Parser::FileFetcherFunc, FileFetcher>);
    ASSERT_SVG_PARSER_SIGNATURE(setFileFetcher, void (Parser::*)(Parser::FileFetcherFunc));
}

void SvgParserSchemaContractTest::svgParserDocumentCreationSignaturesRemainStable()
{
    ASSERT_SVG_PARSER_STATIC_SIGNATURE(createDocumentFromSvg, QDomDocument (*)(QIODevice *, QString *, int *, int *));
    ASSERT_SVG_PARSER_STATIC_SIGNATURE(createDocumentFromSvg,
                                       QDomDocument (*)(QXmlStreamReader, QString *, int *, int *));
    ASSERT_SVG_PARSER_STATIC_SIGNATURE(createDocumentFromSvg,
                                       QDomDocument (*)(const QByteArray &, QString *, int *, int *));
    ASSERT_SVG_PARSER_STATIC_SIGNATURE(createDocumentFromSvg,
                                       QDomDocument (*)(const QString &, QString *, int *, int *));

    static_assert(std::is_same_v<decltype(Parser::createDocumentFromSvg(std::declval<QIODevice *>())), QDomDocument>);
    static_assert(std::is_same_v<decltype(Parser::createDocumentFromSvg(QXmlStreamReader{})), QDomDocument>);
    static_assert(
        std::is_same_v<decltype(Parser::createDocumentFromSvg(std::declval<const QByteArray &>())), QDomDocument>);
    static_assert(
        std::is_same_v<decltype(Parser::createDocumentFromSvg(std::declval<const QString &>())), QDomDocument>);
}

void SvgParserSchemaContractTest::svgParserConfigurationSignaturesRemainStable()
{
    ASSERT_SVG_PARSER_SIGNATURE(setDefaultKraTextVersion, void (Parser::*)(int));
    ASSERT_SVG_PARSER_SIGNATURE(setFillStrokeInheritByDefault, void (Parser::*)(bool));
    ASSERT_SVG_PARSER_SIGNATURE(setResolution, void (Parser::*)(QRectF, qreal));
    ASSERT_SVG_PARSER_SIGNATURE(setResolveTextPropertiesForTopLevel, void (Parser::*)(bool));
    ASSERT_SVG_PARSER_SIGNATURE(setXmlBaseDir, void (Parser::*)(const QString &));
}

void SvgParserSchemaContractTest::svgParserShapeAndDefinitionSignaturesRemainStable()
{
    ASSERT_SVG_PARSER_SIGNATURE(knownMarkers, MarkerList (Parser::*)() const);
    ASSERT_SVG_PARSER_SIGNATURE(parseDefsElement, void (Parser::*)(const QDomElement &));
    ASSERT_SVG_PARSER_SIGNATURE(parseSvg, ShapeList (Parser::*)(const QDomElement &, QSizeF *));
    ASSERT_SVG_PARSER_SIGNATURE(parseTextElement, KoShape * (Parser::*)(const QDomElement &, KoSvgTextShape *));
    ASSERT_SVG_PARSER_SIGNATURE(shapes, ShapeList (Parser::*)() const);
    ASSERT_SVG_PARSER_SIGNATURE(takeSymbols, SymbolList (Parser::*)());

    static_assert(
        std::is_same_v<decltype(std::declval<Parser &>().parseSvg(std::declval<const QDomElement &>())), ShapeList>);
    static_assert(
        std::is_same_v<decltype(std::declval<Parser &>().parseTextElement(std::declval<const QDomElement &>())),
                       KoShape *>);
}

void SvgParserSchemaContractTest::svgParserMetadataAndWarningSignaturesRemainStable()
{
    ASSERT_SVG_PARSER_SIGNATURE(documentDescription, QString (Parser::*)() const);
    ASSERT_SVG_PARSER_SIGNATURE(documentTitle, QString (Parser::*)() const);
    ASSERT_SVG_PARSER_SIGNATURE(warnings, QStringList (Parser::*)() const);
}

void SvgParserSchemaContractTest::svgDragClipboardSchemaRemainStable()
{
    static_assert(std::is_class_v<KoDrag>);
    static_assert(std::is_default_constructible_v<KoDrag>);
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoDrag, addToClipboard, void (KoDrag::*)());
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoDrag, mimeData, QMimeData * (KoDrag::*)());
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoDrag, setData, void (KoDrag::*)(const QString &, const QByteArray &));
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoDrag, setSvg, bool (KoDrag::*)(QList<KoShape *>));
    static_assert(std::is_destructible_v<KoDrag>);
}

void SvgParserSchemaContractTest::svgPasteImportSchemaRemainStable()
{
    using Shapes = QList<KoShape *>;

    static_assert(std::is_class_v<KoSvgPaste>);
    static_assert(std::is_default_constructible_v<KoSvgPaste>);
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoSvgPaste, fetchShapes, Shapes (KoSvgPaste::*)(QRectF, qreal, QSizeF *));
    ASSERT_SVG_EXCHANGE_STATIC_SIGNATURE(KoSvgPaste,
                                         fetchShapesFromData,
                                         Shapes (*)(const QByteArray &, QRectF, qreal, QSizeF *));
    static_assert(std::is_same_v<decltype(std::declval<KoSvgPaste &>().fetchShapes(std::declval<QRectF>(),
                                                                                   std::declval<qreal>())),
                                 Shapes>);
    static_assert(std::is_same_v<decltype(KoSvgPaste::fetchShapesFromData(std::declval<const QByteArray &>(),
                                                                          std::declval<QRectF>(),
                                                                          std::declval<qreal>())),
                                 Shapes>);
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoSvgPaste, hasShapes, bool (KoSvgPaste::*)());
    static_assert(std::has_virtual_destructor_v<KoSvgPaste>);
}

void SvgParserSchemaContractTest::svgPathShapeLoaderSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPathShapeLoader>);
    static_assert(std::is_constructible_v<KoPathShapeLoader, KoPathShape *>);
    ASSERT_SVG_EXCHANGE_SIGNATURE(KoPathShapeLoader, parseSvg, void (KoPathShapeLoader::*)(const QString &, bool));
    static_assert(
        std::is_same_v<decltype(std::declval<KoPathShapeLoader &>().parseSvg(std::declval<const QString &>())), void>);
    static_assert(std::is_destructible_v<KoPathShapeLoader>);
}

void SvgParserSchemaContractTest::svgShapeTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<SvgShape>);
    static_assert(std::has_virtual_destructor_v<SvgShape>);
}

void SvgParserSchemaContractTest::svgShapePersistenceSignaturesRemainStable()
{
    ASSERT_SVG_EXCHANGE_SIGNATURE(SvgShape, loadSvg, bool (SvgShape::*)(const QDomElement &, SvgLoadingContext &));
    ASSERT_SVG_EXCHANGE_SIGNATURE(SvgShape, saveMetadata, void (SvgShape::*)(SvgSavingContext &));
    ASSERT_SVG_EXCHANGE_SIGNATURE(SvgShape, saveSvg, bool (SvgShape::*)(SvgSavingContext &));
}

QTEST_APPLESS_MAIN(SvgParserSchemaContractTest)

#include "SvgParserSchemaContractTest.moc"
