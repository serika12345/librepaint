/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/kis_image_view_converter.h>
#include <flake/kis_shape_selection.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_SHAPE_SELECTION_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisShapeSelection::method)), signature>)
#define ASSERT_SHAPE_SELECTION_FACTORY_SIGNATURE(method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisShapeSelectionFactory::method)), signature>)
#define ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImageViewConverter::method)), signature>)
} // namespace

class KisShapeSelectionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeSelectionTypesLifetimeAndConstructionSchemaRemainStable();
    void shapeSelectionContentSignaturesRemainStable();
    void shapeSelectionOutlineAndGeometrySignaturesRemainStable();
    void shapeSelectionPersistenceAndIntegrationSignaturesRemainStable();
    void shapeSelectionFactoryAndNotificationSignaturesRemainStable();
    void imageViewConverterTypeConstructionAndLifetimeSchemaRemainStable();
    void imageViewConverterCloneAndImageSignaturesRemainStable();
    void imageViewConverterZoomSignaturesRemainStable();
    void imageViewConverterDocumentToViewSignaturesRemainStable();
    void imageViewConverterViewToDocumentSignaturesRemainStable();
};

void KisShapeSelectionSchemaContractTest::shapeSelectionTypesLifetimeAndConstructionSchemaRemainStable()
{
    using Selection = KisShapeSelection;
    using Factory = KisShapeSelectionFactory;

    static_assert(std::is_class_v<KisShapeSelectionMarker>);
    static_assert(std::is_class_v<Selection>);
    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_constructible_v<Selection, KoShapeControllerBase *, KisSelectionWSP>);
    static_assert(std::is_constructible_v<Selection, const Selection &, KisSelection *>);
    static_assert(std::has_virtual_destructor_v<Selection>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::has_virtual_destructor_v<Factory>);
}

void KisShapeSelectionSchemaContractTest::shapeSelectionContentSignaturesRemainStable()
{
    using Selection = KisShapeSelection;

    ASSERT_SHAPE_SELECTION_SIGNATURE(clone, KisSelectionComponent * (Selection::*)(KisSelection *));
    ASSERT_SHAPE_SELECTION_SIGNATURE(isEmpty, bool (Selection::*)() const);
    ASSERT_SHAPE_SELECTION_SIGNATURE(resetToEmpty, KUndo2Command * (Selection::*)());
    ASSERT_SHAPE_SELECTION_SIGNATURE(renderToProjection, void (Selection::*)(KisPaintDeviceSP));
    ASSERT_SHAPE_SELECTION_SIGNATURE(renderToProjection, void (Selection::*)(KisPaintDeviceSP, const QRect &));
}

void KisShapeSelectionSchemaContractTest::shapeSelectionOutlineAndGeometrySignaturesRemainStable()
{
    using Selection = KisShapeSelection;

    ASSERT_SHAPE_SELECTION_SIGNATURE(outlineCache, QPainterPath (Selection::*)() const);
    ASSERT_SHAPE_SELECTION_SIGNATURE(outlineCacheValid, bool (Selection::*)() const);
    ASSERT_SHAPE_SELECTION_SIGNATURE(recalculateOutlineCache, void (Selection::*)());
    ASSERT_SHAPE_SELECTION_SIGNATURE(moveX, void (Selection::*)(qint32));
    ASSERT_SHAPE_SELECTION_SIGNATURE(moveY, void (Selection::*)(qint32));
    ASSERT_SHAPE_SELECTION_SIGNATURE(transform, KUndo2Command * (Selection::*)(const QTransform &));
}

void KisShapeSelectionSchemaContractTest::shapeSelectionPersistenceAndIntegrationSignaturesRemainStable()
{
    using Selection = KisShapeSelection;

    ASSERT_SHAPE_SELECTION_SIGNATURE(saveSelection, bool (Selection::*)(KoStore *, const QRect &) const);
    ASSERT_SHAPE_SELECTION_SIGNATURE(loadSelection, bool (Selection::*)(KoStore *, const QRect &));
    ASSERT_SHAPE_SELECTION_SIGNATURE(shapeManager, KoShapeManager * (Selection::*)() const);
    ASSERT_SHAPE_SELECTION_SIGNATURE(setResolutionProxy, void (Selection::*)(KisImageResolutionProxySP));
}

void KisShapeSelectionSchemaContractTest::shapeSelectionFactoryAndNotificationSignaturesRemainStable()
{
    using Selection = KisShapeSelection;
    using Factory = KisShapeSelectionFactory;

    ASSERT_SHAPE_SELECTION_FACTORY_SIGNATURE(createDefaultShape,
                                             KoShape * (Factory::*)(KoDocumentResourceManager *) const);
    ASSERT_SHAPE_SELECTION_FACTORY_SIGNATURE(supports,
                                             bool (Factory::*)(const QDomElement &, KoShapeLoadingContext &) const);
    ASSERT_SHAPE_SELECTION_SIGNATURE(sigMoveShapes, void (Selection::*)(const QPointF &));

    static_assert(std::is_same_v<decltype(std::declval<const Factory &>().createDefaultShape()), KoShape *>);
}

void KisShapeSelectionSchemaContractTest::imageViewConverterTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Converter = KisImageViewConverter;

    static_assert(std::is_same_v<KisImageViewConverterSP, QSharedPointer<Converter>>);
    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_default_constructible_v<Converter>);
    static_assert(std::is_constructible_v<Converter, KisImageWSP>);
    static_assert(std::is_constructible_v<Converter, KisImageResolutionProxySP>);
    static_assert(std::is_copy_constructible_v<Converter>);
    static_assert(std::has_virtual_destructor_v<Converter>);
}

void KisShapeSelectionSchemaContractTest::imageViewConverterCloneAndImageSignaturesRemainStable()
{
    using Converter = KisImageViewConverter;

    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(clone, KisClonableViewConverter * (Converter::*)() const);
    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(setImage, void (Converter::*)(KisImageWSP));
}

void KisShapeSelectionSchemaContractTest::imageViewConverterZoomSignaturesRemainStable()
{
    using Converter = KisImageViewConverter;

    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(zoom, void (Converter::*)(qreal *, qreal *) const);
    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(zoom, qreal (Converter::*)() const);
}

void KisShapeSelectionSchemaContractTest::imageViewConverterDocumentToViewSignaturesRemainStable()
{
    using Converter = KisImageViewConverter;

    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(documentToViewX, qreal (Converter::*)(qreal) const);
    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(documentToViewY, qreal (Converter::*)(qreal) const);
}

void KisShapeSelectionSchemaContractTest::imageViewConverterViewToDocumentSignaturesRemainStable()
{
    using Converter = KisImageViewConverter;

    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(viewToDocumentX, qreal (Converter::*)(qreal) const);
    ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE(viewToDocumentY, qreal (Converter::*)(qreal) const);
}

#undef ASSERT_SHAPE_SELECTION_SIGNATURE
#undef ASSERT_SHAPE_SELECTION_FACTORY_SIGNATURE
#undef ASSERT_IMAGE_VIEW_CONVERTER_SIGNATURE

QTEST_APPLESS_MAIN(KisShapeSelectionSchemaContractTest)

#include "KisShapeSelectionSchemaContractTest.moc"
