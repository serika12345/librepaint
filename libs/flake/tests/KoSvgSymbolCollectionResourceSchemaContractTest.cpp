/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/KoSvgSymbolCollectionResource.h"

#include <QTest>

#include <type_traits>

namespace
{
using SvgSymbol = KoSvgSymbol;
using SymbolCollection = KoSvgSymbolCollectionResource;
using ResourceTypePair = QPair<QString, QString>;
using SymbolList = QVector<SvgSymbol *>;

#define ASSERT_SVG_SYMBOL_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgSymbol::method)), signature>)
#define ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SymbolCollection::method)), signature>)
} // namespace

class KoSvgSymbolCollectionResourceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgSymbolValueTypeAndLifetimeSchemaRemainStable();
    void svgSymbolCollectionTypeAndLifetimeSchemaRemainStable();
    void svgSymbolCollectionPersistenceSignaturesRemainStable();
    void svgSymbolCollectionMetadataSignaturesRemainStable();
    void svgSymbolCollectionContentsSignatureRemainsStable();
};

void KoSvgSymbolCollectionResourceSchemaContractTest::svgSymbolValueTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<SvgSymbol>);
    static_assert(std::is_default_constructible_v<SvgSymbol>);
    static_assert(std::is_copy_constructible_v<SvgSymbol>);
    static_assert(std::is_constructible_v<SvgSymbol, const QString &>);
    static_assert(std::is_destructible_v<SvgSymbol>);
    static_assert(std::is_same_v<decltype(&SvgSymbol::id), QString SvgSymbol::*>);
    static_assert(std::is_same_v<decltype(&SvgSymbol::title), QString SvgSymbol::*>);
    static_assert(std::is_same_v<decltype(&SvgSymbol::shape), KoShape * SvgSymbol::*>);
    ASSERT_SVG_SYMBOL_SIGNATURE(icon, QImage (SvgSymbol::*)(int));
    ASSERT_SVG_SYMBOL_SIGNATURE(operator==, bool (SvgSymbol::*)(const SvgSymbol &) const);
}

void KoSvgSymbolCollectionResourceSchemaContractTest::svgSymbolCollectionTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<SymbolCollection>);
    static_assert(std::is_base_of_v<KoResource, SymbolCollection>);
    static_assert(std::has_virtual_destructor_v<SymbolCollection>);
    static_assert(std::is_default_constructible_v<SymbolCollection>);
    static_assert(std::is_copy_constructible_v<SymbolCollection>);
    static_assert(std::is_constructible_v<SymbolCollection, const QString &>);
    static_assert(!std::is_copy_assignable_v<SymbolCollection>);
}

void KoSvgSymbolCollectionResourceSchemaContractTest::svgSymbolCollectionPersistenceSignaturesRemainStable()
{
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(clone, KoResourceSP (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(loadFromDevice,
                                           bool (SymbolCollection::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(saveToDevice, bool (SymbolCollection::*)(QIODevice *) const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(defaultFileExtension, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(resourceType, ResourceTypePair (SymbolCollection::*)() const);
}

void KoSvgSymbolCollectionResourceSchemaContractTest::svgSymbolCollectionMetadataSignaturesRemainStable()
{
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(title, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(description, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(creator, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(rights, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(language, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(subjects, QStringList (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(license, QString (SymbolCollection::*)() const);
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(permits, QStringList (SymbolCollection::*)() const);
}

void KoSvgSymbolCollectionResourceSchemaContractTest::svgSymbolCollectionContentsSignatureRemainsStable()
{
    ASSERT_SVG_SYMBOL_COLLECTION_SIGNATURE(symbols, SymbolList (SymbolCollection::*)() const);
}

QTEST_APPLESS_MAIN(KoSvgSymbolCollectionResourceSchemaContractTest)

#include "KoSvgSymbolCollectionResourceSchemaContractTest.moc"
