/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeLoadingContext.h>

#include <QTest>
#include <QVariant>

#include <type_traits>
#include <utility>

namespace
{
using ShapeSubItem = QPair<KoShape *, QVariant>;

#define ASSERT_LOADING_CONTEXT_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeLoadingContext::method)), signature>)
} // namespace

class KoShapeLoadingContextSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void additionalAttributeValueSchemaRemainsStable();
    void shapeLoadingIdentityStoreAndResourceSignaturesRemainStable();
    void shapeLoadingLayerZIndexAndSectionSignaturesRemainStable();
    void shapeLoadingShapeReferenceAndUpdaterSignaturesRemainStable();
    void shapeLoadingSharedAndAdditionalDataSignaturesRemainStable();
};

void KoShapeLoadingContextSchemaContractTest::additionalAttributeValueSchemaRemainsStable()
{
    using Attribute = KoShapeLoadingContext::AdditionalAttributeData;

    static_assert(std::is_same_v<decltype(Attribute::ns), const QString>);
    static_assert(std::is_same_v<decltype(Attribute::tag), const QString>);
    static_assert(std::is_same_v<decltype(Attribute::name), const QString>);
    static_assert(std::is_constructible_v<Attribute, const QString &, const QString &, const QString &>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Attribute &>() == std::declval<const Attribute &>()), bool>);

    QString sourceNamespace = QStringLiteral("urn:librepaint:first");
    QString sourceTag = QStringLiteral("first-tag");
    QString sourceName = QStringLiteral("shared-name");
    const Attribute original(sourceNamespace, sourceTag, sourceName);
    const Attribute copied(original);

    sourceNamespace = QStringLiteral("urn:librepaint:changed");
    sourceTag = QStringLiteral("changed-tag");
    sourceName = QStringLiteral("changed-name");

    QCOMPARE(original.ns, QStringLiteral("urn:librepaint:first"));
    QCOMPARE(original.tag, QStringLiteral("first-tag"));
    QCOMPARE(original.name, QStringLiteral("shared-name"));
    QCOMPARE(copied.ns, original.ns);
    QCOMPARE(copied.tag, original.tag);
    QCOMPARE(copied.name, original.name);

    const Attribute sameNameDifferentContext(QStringLiteral("urn:librepaint:second"),
                                             QStringLiteral("second-tag"),
                                             QStringLiteral("shared-name"));
    const Attribute differentName(original.ns, original.tag, QStringLiteral("different-name"));
    QVERIFY(original == sameNameDifferentContext);
    QVERIFY(!(original == differentName));
}

void KoShapeLoadingContextSchemaContractTest::shapeLoadingIdentityStoreAndResourceSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoShapeLoadingContext>);
    static_assert(std::is_constructible_v<KoShapeLoadingContext, KoStore *, KoDocumentResourceManager *>);
    static_assert(std::is_destructible_v<KoShapeLoadingContext>);

    ASSERT_LOADING_CONTEXT_SIGNATURE(documentResourceManager,
                                     KoDocumentResourceManager * (KoShapeLoadingContext::*)() const);
    ASSERT_LOADING_CONTEXT_SIGNATURE(mimeTypeForPath, QString (KoShapeLoadingContext::*)(const QString &, bool));
    ASSERT_LOADING_CONTEXT_SIGNATURE(store, KoStore * (KoShapeLoadingContext::*)() const);

    static_assert(std::is_same_v<decltype(std::declval<KoShapeLoadingContext &>().mimeTypeForPath(
                                     std::declval<const QString &>())),
                                 QString>);
}

void KoShapeLoadingContextSchemaContractTest::shapeLoadingLayerZIndexAndSectionSignaturesRemainStable()
{
    ASSERT_LOADING_CONTEXT_SIGNATURE(addLayer, void (KoShapeLoadingContext::*)(KoShapeLayer *, const QString &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(clearLayers, void (KoShapeLoadingContext::*)());
    ASSERT_LOADING_CONTEXT_SIGNATURE(layer, KoShapeLayer * (KoShapeLoadingContext::*)(const QString &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(sectionModel, KoSectionModel * (KoShapeLoadingContext::*)());
    ASSERT_LOADING_CONTEXT_SIGNATURE(setSectionModel, void (KoShapeLoadingContext::*)(KoSectionModel *));
    ASSERT_LOADING_CONTEXT_SIGNATURE(setZIndex, void (KoShapeLoadingContext::*)(int));
    ASSERT_LOADING_CONTEXT_SIGNATURE(zIndex, int (KoShapeLoadingContext::*)());
}

void KoShapeLoadingContextSchemaContractTest::shapeLoadingShapeReferenceAndUpdaterSignaturesRemainStable()
{
    ASSERT_LOADING_CONTEXT_SIGNATURE(addShapeId, void (KoShapeLoadingContext::*)(KoShape *, const QString &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(addShapeSubItemId,
                                     void (KoShapeLoadingContext::*)(KoShape *, const QVariant &, const QString &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(shapeById, KoShape * (KoShapeLoadingContext::*)(const QString &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(shapeLoaded, void (KoShapeLoadingContext::*)(KoShape *));
    ASSERT_LOADING_CONTEXT_SIGNATURE(shapeSubItemById, ShapeSubItem (KoShapeLoadingContext::*)(const QString &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(updateShape,
                                     void (KoShapeLoadingContext::*)(const QString &, KoLoadingShapeUpdater *));
}

void KoShapeLoadingContextSchemaContractTest::shapeLoadingSharedAndAdditionalDataSignaturesRemainStable()
{
    using Attribute = KoShapeLoadingContext::AdditionalAttributeData;

    ASSERT_LOADING_CONTEXT_SIGNATURE(addAdditionalAttributeData, void (*)(const Attribute &));
    ASSERT_LOADING_CONTEXT_SIGNATURE(additionalAttributeData, QSet<Attribute> (*)());
    ASSERT_LOADING_CONTEXT_SIGNATURE(addSharedData,
                                     void (KoShapeLoadingContext::*)(const QString &, KoSharedLoadingData *));
    ASSERT_LOADING_CONTEXT_SIGNATURE(sharedData,
                                     KoSharedLoadingData * (KoShapeLoadingContext::*)(const QString &) const);
}

QTEST_APPLESS_MAIN(KoShapeLoadingContextSchemaContractTest)

#include "KoShapeLoadingContextSchemaContractTest.moc"
