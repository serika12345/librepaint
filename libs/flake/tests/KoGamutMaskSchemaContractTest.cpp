/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/KoGamutMask.h"

#include <QTest>

#include <type_traits>

namespace
{
using GamutMask = KoGamutMask;
using GamutMaskShape = KoGamutMaskShape;
using ResourceTypePair = QPair<QString, QString>;
using ShapeList = QList<KoShape *>;

#define ASSERT_GAMUT_MASK_SHAPE_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&GamutMaskShape::method)), signature>)
#define ASSERT_GAMUT_MASK_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&GamutMask::method)), signature>)
} // namespace

class KoGamutMaskSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gamutMaskShapeTypeGeometryAndRenderingSignaturesRemainStable();
    void gamutMaskTypeLifetimeAndValueSemanticsRemainStable();
    void gamutMaskPersistenceAndMetadataSignaturesRemainStable();
    void gamutMaskGeometryAndRenderingSignaturesRemainStable();
    void gamutMaskShapeCollectionSignaturesRemainStable();
};

void KoGamutMaskSchemaContractTest::gamutMaskShapeTypeGeometryAndRenderingSignaturesRemainStable()
{
    static_assert(std::is_class_v<GamutMaskShape>);
    static_assert(std::is_default_constructible_v<GamutMaskShape>);
    static_assert(std::is_constructible_v<GamutMaskShape, KoShape *>);
    static_assert(std::is_destructible_v<GamutMaskShape>);
    ASSERT_GAMUT_MASK_SHAPE_SIGNATURE(coordIsClear, bool (GamutMaskShape::*)(const QPointF &) const);
    ASSERT_GAMUT_MASK_SHAPE_SIGNATURE(outline, QPainterPath (GamutMaskShape::*)());
    ASSERT_GAMUT_MASK_SHAPE_SIGNATURE(paint, void (GamutMaskShape::*)(QPainter &));
    ASSERT_GAMUT_MASK_SHAPE_SIGNATURE(paintStroke, void (GamutMaskShape::*)(QPainter &));
    ASSERT_GAMUT_MASK_SHAPE_SIGNATURE(koShape, KoShape * (GamutMaskShape::*)());
}

void KoGamutMaskSchemaContractTest::gamutMaskTypeLifetimeAndValueSemanticsRemainStable()
{
    static_assert(std::is_same_v<KoGamutMaskSP, QSharedPointer<GamutMask>>);
    static_assert(std::is_class_v<GamutMask>);
    static_assert(std::is_base_of_v<QObject, GamutMask>);
    static_assert(std::is_base_of_v<KoResource, GamutMask>);
    static_assert(std::is_default_constructible_v<GamutMask>);
    static_assert(std::is_constructible_v<GamutMask, GamutMask *>);
    static_assert(std::is_copy_constructible_v<GamutMask>);
    static_assert(std::is_constructible_v<GamutMask, const QString &>);
    static_assert(!std::is_copy_assignable_v<GamutMask>);
    static_assert(std::has_virtual_destructor_v<GamutMask>);
}

void KoGamutMaskSchemaContractTest::gamutMaskPersistenceAndMetadataSignaturesRemainStable()
{
    ASSERT_GAMUT_MASK_SIGNATURE(clone, KoResourceSP (GamutMask::*)() const);
    ASSERT_GAMUT_MASK_SIGNATURE(loadFromDevice, bool (GamutMask::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_GAMUT_MASK_SIGNATURE(saveToDevice, bool (GamutMask::*)(QIODevice *) const);
    ASSERT_GAMUT_MASK_SIGNATURE(resourceType, ResourceTypePair (GamutMask::*)() const);
    ASSERT_GAMUT_MASK_SIGNATURE(defaultFileExtension, QString (GamutMask::*)() const);
    ASSERT_GAMUT_MASK_SIGNATURE(title, QString (GamutMask::*)() const);
    ASSERT_GAMUT_MASK_SIGNATURE(setTitle, void (GamutMask::*)(QString));
    ASSERT_GAMUT_MASK_SIGNATURE(description, QString (GamutMask::*)() const);
    ASSERT_GAMUT_MASK_SIGNATURE(setDescription, void (GamutMask::*)(QString));
}

void KoGamutMaskSchemaContractTest::gamutMaskGeometryAndRenderingSignaturesRemainStable()
{
    ASSERT_GAMUT_MASK_SIGNATURE(coordIsClear, bool (GamutMask::*)(const QPointF &, bool));
    ASSERT_GAMUT_MASK_SIGNATURE(paint, void (GamutMask::*)(QPainter &, bool));
    ASSERT_GAMUT_MASK_SIGNATURE(paintStroke, void (GamutMask::*)(QPainter &, bool));
    ASSERT_GAMUT_MASK_SIGNATURE(maskToViewTransform, QTransform (GamutMask::*)(qreal));
    ASSERT_GAMUT_MASK_SIGNATURE(viewToMaskTransform, QTransform (GamutMask::*)(qreal));
    ASSERT_GAMUT_MASK_SIGNATURE(rotation, int (GamutMask::*)());
    ASSERT_GAMUT_MASK_SIGNATURE(setRotation, void (GamutMask::*)(int));
    ASSERT_GAMUT_MASK_SIGNATURE(maskSize, QSizeF (GamutMask::*)());
}

void KoGamutMaskSchemaContractTest::gamutMaskShapeCollectionSignaturesRemainStable()
{
    ASSERT_GAMUT_MASK_SIGNATURE(setMaskShapes, void (GamutMask::*)(ShapeList));
    ASSERT_GAMUT_MASK_SIGNATURE(setPreviewMaskShapes, void (GamutMask::*)(ShapeList));
    ASSERT_GAMUT_MASK_SIGNATURE(koShapes, ShapeList (GamutMask::*)() const);
    ASSERT_GAMUT_MASK_SIGNATURE(clearPreview, void (GamutMask::*)());
}

QTEST_APPLESS_MAIN(KoGamutMaskSchemaContractTest)

#include "KoGamutMaskSchemaContractTest.moc"
