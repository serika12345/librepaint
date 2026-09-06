/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_abr_brush.h>
#include <kis_abr_brush_collection.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_ABR_SIGNATURE(method, signature)                                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAbrBrush::method)), signature>)
#define ASSERT_COLLECTION_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAbrBrushCollection::method)), signature>)
} // namespace

class KisAbrBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void brushTypeAndConstructionSchemaRemainStable();
    void brushResourceIoSchemaRemainsStable();
    void brushImageAndSerializationSchemaRemainStable();
    void collectionTypeAndIoSchemaRemainStable();
    void collectionContentAndMetadataSchemaRemainStable();
};

void KisAbrBrushSchemaContractTest::brushTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAbrBrush>);
    static_assert(std::is_same_v<KisAbrBrushSP, QSharedPointer<KisAbrBrush>>);
    static_assert(std::is_same_v<KisQImagemaskSP, KisSharedPtr<KisQImagemask>>);
    static_assert(std::is_constructible_v<KisAbrBrush, const QString &, KisAbrBrushCollection *>);
    static_assert(std::is_copy_constructible_v<KisAbrBrush>);
    static_assert(std::is_constructible_v<KisAbrBrush, const KisAbrBrush &, KisAbrBrushCollection *>);
    static_assert(!std::is_copy_assignable_v<KisAbrBrush>);
}

void KisAbrBrushSchemaContractTest::brushResourceIoSchemaRemainsStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisAbrBrush::*)() const;

    ASSERT_ABR_SIGNATURE(clone, KoResourceSP (KisAbrBrush::*)() const);
    ASSERT_ABR_SIGNATURE(isSerializable, bool (KisAbrBrush::*)() const);
    ASSERT_ABR_SIGNATURE(loadFromDevice, bool (KisAbrBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_ABR_SIGNATURE(saveToDevice, bool (KisAbrBrush::*)(QIODevice *) const);
    ASSERT_ABR_SIGNATURE(resourceType, ResourceTypeSignature);
}

void KisAbrBrushSchemaContractTest::brushImageAndSerializationSchemaRemainStable()
{
    ASSERT_ABR_SIGNATURE(defaultFileExtension, QString (KisAbrBrush::*)() const);
    ASSERT_ABR_SIGNATURE(brushTipImage, QImage (KisAbrBrush::*)() const);
    ASSERT_ABR_SIGNATURE(setBrushTipImage, void (KisAbrBrush::*)(const QImage &));
    ASSERT_ABR_SIGNATURE(toXML, void (KisAbrBrush::*)(QDomDocument &, QDomElement &) const);
}

void KisAbrBrushSchemaContractTest::collectionTypeAndIoSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAbrBrushCollection>);
    static_assert(std::is_same_v<KisAbrBrushCollectionSP, QSharedPointer<KisAbrBrushCollection>>);
    static_assert(std::is_constructible_v<KisAbrBrushCollection, const QString &>);
    static_assert(std::is_destructible_v<KisAbrBrushCollection>);
    ASSERT_COLLECTION_SIGNATURE(load, bool (KisAbrBrushCollection::*)());
    ASSERT_COLLECTION_SIGNATURE(loadFromDevice, bool (KisAbrBrushCollection::*)(QIODevice *));
    ASSERT_COLLECTION_SIGNATURE(save, bool (KisAbrBrushCollection::*)());
    ASSERT_COLLECTION_SIGNATURE(saveToDevice, bool (KisAbrBrushCollection::*)(QIODevice *) const);
    ASSERT_COLLECTION_SIGNATURE(isLoaded, bool (KisAbrBrushCollection::*)() const);
}

void KisAbrBrushSchemaContractTest::collectionContentAndMetadataSchemaRemainStable()
{
    using BrushesMapSignature = QSharedPointer<QMap<QString, KisAbrBrushSP>> (KisAbrBrushCollection::*)() const;

    ASSERT_COLLECTION_SIGNATURE(image, QImage (KisAbrBrushCollection::*)() const);
    ASSERT_COLLECTION_SIGNATURE(defaultFileExtension, QString (KisAbrBrushCollection::*)() const);
    ASSERT_COLLECTION_SIGNATURE(brushes, QList<KisAbrBrushSP> (KisAbrBrushCollection::*)() const);
    ASSERT_COLLECTION_SIGNATURE(brushesMap, BrushesMapSignature);
    ASSERT_COLLECTION_SIGNATURE(brushByName, KisAbrBrushSP (KisAbrBrushCollection::*)(QString) const);
    ASSERT_COLLECTION_SIGNATURE(lastModified, QDateTime (KisAbrBrushCollection::*)() const);
    ASSERT_COLLECTION_SIGNATURE(filename, QString (KisAbrBrushCollection::*)() const);
}

QTEST_GUILESS_MAIN(KisAbrBrushSchemaContractTest)

#include "KisAbrBrushSchemaContractTest.moc"
