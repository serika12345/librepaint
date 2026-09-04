/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Document.h>

#include <QTest>

#include <type_traits>
#include <utility>

class DocumentNodeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentTypeLifetimeAndComparisonSchemaRemainStable();
    void documentNodeGraphSignaturesRemainStable();
    void documentMaskCreationSignaturesRemainStable();
    void documentNodeAndLayerCreationSignaturesRemainStable();
    void documentFlatteningSignatureRemainsStable();
};

void DocumentNodeSchemaContractTest::documentTypeLifetimeAndComparisonSchemaRemainStable()
{
    using CloneSignature = Document *(Document::*)() const;
    using ComparisonSignature = bool (Document::*)(const Document &) const;

    static_assert(std::is_base_of_v<QObject, Document>);
    static_assert(std::is_constructible_v<Document, KisDocument *, bool, QObject *>);
    static_assert(std::is_constructible_v<Document, KisDocument *, bool>);
    static_assert(std::has_virtual_destructor_v<Document>);
    static_assert(std::is_same_v<decltype(&Document::clone), CloneSignature>);
    static_assert(std::is_same_v<decltype(&Document::operator==), ComparisonSignature>);
    static_assert(std::is_same_v<decltype(&Document::operator!=), ComparisonSignature>);
}

void DocumentNodeSchemaContractTest::documentNodeGraphSignaturesRemainStable()
{
    using NodeAccessorSignature = Node *(Document::*)() const;
    using NamedNodeAccessorSignature = Node *(Document::*)(const QString &) const;
    using UniqueNodeAccessorSignature = Node *(Document::*)(const QUuid &) const;
    using SelectionAccessorSignature = Selection *(Document::*)() const;
    using SetNodeSignature = void (Document::*)(Node *);
    using SetSelectionSignature = void (Document::*)(Selection *);
    using TopLevelNodesSignature = QList<Node *> (Document::*)() const;

    static_assert(std::is_same_v<decltype(&Document::activeNode), NodeAccessorSignature>);
    static_assert(std::is_same_v<decltype(&Document::nodeByName), NamedNodeAccessorSignature>);
    static_assert(std::is_same_v<decltype(&Document::nodeByUniqueID), UniqueNodeAccessorSignature>);
    static_assert(std::is_same_v<decltype(&Document::rootNode), NodeAccessorSignature>);
    static_assert(std::is_same_v<decltype(&Document::selection), SelectionAccessorSignature>);
    static_assert(std::is_same_v<decltype(&Document::setActiveNode), SetNodeSignature>);
    static_assert(std::is_same_v<decltype(&Document::setSelection), SetSelectionSignature>);
    static_assert(std::is_same_v<decltype(&Document::topLevelNodes), TopLevelNodesSignature>);
}

void DocumentNodeSchemaContractTest::documentMaskCreationSignaturesRemainStable()
{
    using ColorizeMaskSignature = ColorizeMask *(Document::*)(const QString &);
    using FilterMaskSelectionSignature = FilterMask *(Document::*)(const QString &, Filter &, Selection &);
    using FilterMaskNodeSignature = FilterMask *(Document::*)(const QString &, Filter &, const Node *);
    using SelectionMaskSignature = SelectionMask *(Document::*)(const QString &);
    using TransformMaskSignature = TransformMask *(Document::*)(const QString &);
    using TransparencyMaskSignature = TransparencyMask *(Document::*)(const QString &);

    static_assert(std::is_same_v<decltype(&Document::createColorizeMask), ColorizeMaskSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FilterMaskSelectionSignature>(&Document::createFilterMask)),
                                 FilterMaskSelectionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FilterMaskNodeSignature>(&Document::createFilterMask)),
                                 FilterMaskNodeSignature>);
    static_assert(std::is_same_v<decltype(&Document::createSelectionMask), SelectionMaskSignature>);
    static_assert(std::is_same_v<decltype(&Document::createTransformMask), TransformMaskSignature>);
    static_assert(std::is_same_v<decltype(&Document::createTransparencyMask), TransparencyMaskSignature>);
}

void DocumentNodeSchemaContractTest::documentNodeAndLayerCreationSignaturesRemainStable()
{
    using CloneLayerSignature = CloneLayer *(Document::*)(const QString &, const Node *);
    using FileLayerSignature = FileLayer *(Document::*)(const QString &, const QString, const QString, const QString);
    using FillLayerSignature = FillLayer *(Document::*)(const QString &, const QString, InfoObject &, Selection &);
    using FilterLayerSignature = FilterLayer *(Document::*)(const QString &, Filter &, Selection &);
    using GroupLayerSignature = GroupLayer *(Document::*)(const QString &);
    using NodeSignature = Node *(Document::*)(const QString &, const QString &);
    using VectorLayerSignature = VectorLayer *(Document::*)(const QString &);

    static_assert(std::is_same_v<decltype(&Document::createCloneLayer), CloneLayerSignature>);
    static_assert(std::is_same_v<decltype(&Document::createFileLayer), FileLayerSignature>);
    static_assert(std::is_same_v<decltype(&Document::createFillLayer), FillLayerSignature>);
    static_assert(std::is_same_v<decltype(&Document::createFilterLayer), FilterLayerSignature>);
    static_assert(std::is_same_v<decltype(&Document::createGroupLayer), GroupLayerSignature>);
    static_assert(std::is_same_v<decltype(&Document::createNode), NodeSignature>);
    static_assert(std::is_same_v<decltype(&Document::createVectorLayer), VectorLayerSignature>);
    static_assert(std::is_same_v<decltype(std::declval<Document &>().createFileLayer(std::declval<const QString &>(),
                                                                                     std::declval<QString>(),
                                                                                     std::declval<QString>())),
                                 FileLayer *>);
}

void DocumentNodeSchemaContractTest::documentFlatteningSignatureRemainsStable()
{
    using FlattenSignature = void (Document::*)();

    static_assert(std::is_same_v<decltype(&Document::flatten), FlattenSignature>);
}

QTEST_APPLESS_MAIN(DocumentNodeSchemaContractTest)

#include "DocumentNodeSchemaContractTest.moc"
