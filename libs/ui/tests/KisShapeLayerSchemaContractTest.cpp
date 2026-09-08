/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <flake/kis_shape_controller.h>
#include <flake/kis_shape_layer.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_SHAPE_LAYER_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisShapeLayer::method)), signature>)
#define ASSERT_SHAPE_CONTROLLER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisShapeController::method)), signature>)
} // namespace

class KisShapeLayerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeLayerTypeLifetimeAndConstructionSchemaRemainStable();
    void shapeLayerNodeAndStateSignaturesRemainStable();
    void shapeLayerDeviceAndGeometrySignaturesRemainStable();
    void shapeLayerShapeAndPersistenceSignaturesRemainStable();
    void shapeLayerSectionUpdateAndNotificationSignaturesRemainStable();
    void shapeControllerTypeAndLifetimeSchemaRemainStable();
    void shapeControllerDummyQuerySignaturesRemainStable();
    void shapeControllerShapeCanvasAndImageSignaturesRemainStable();
    void shapeControllerCreationAndDocumentGeometrySignaturesRemainStable();
    void shapeControllerNotificationSignaturesRemainStable();
};

void KisShapeLayerSchemaContractTest::shapeLayerTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Layer = KisShapeLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_same_v<decltype(KIS_SHAPE_LAYER_ID), const QString>);
    static_assert(std::is_constructible_v<Layer, KoShapeControllerBase *, KisImageWSP, const QString &, quint8>);
    static_assert(std::is_copy_constructible_v<Layer>);
    static_assert(std::is_constructible_v<Layer, const Layer &, KoShapeControllerBase *>);
    static_assert(std::is_constructible_v<Layer,
                                          const Layer &,
                                          KoShapeControllerBase *,
                                          std::function<KisShapeLayerCanvasBase *()>>);
    static_assert(std::is_constructible_v<Layer, const Layer &, const QList<KoShape *> &>);
    static_assert(std::has_virtual_destructor_v<Layer>);
}

void KisShapeLayerSchemaContractTest::shapeLayerNodeAndStateSignaturesRemainStable()
{
    using Layer = KisShapeLayer;

    ASSERT_SHAPE_LAYER_SIGNATURE(clone, KisNodeSP (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(allowAsChild, bool (Layer::*)(KisNodeSP) const);
    ASSERT_SHAPE_LAYER_SIGNATURE(setImage, void (Layer::*)(KisImageWSP));
    ASSERT_SHAPE_LAYER_SIGNATURE(tryCreateInternallyMergedLayerFromMutipleLayers,
                                 KisLayerSP (Layer::*)(QList<KisLayerSP>));
    ASSERT_SHAPE_LAYER_SIGNATURE(createMergedLayerTemplate, KisLayerSP (Layer::*)(KisLayerSP));
    ASSERT_SHAPE_LAYER_SIGNATURE(fillMergedLayerTemplate, void (Layer::*)(KisLayerSP, KisLayerSP, bool));
    ASSERT_SHAPE_LAYER_SIGNATURE(accept, bool (Layer::*)(KisNodeVisitor &));
    ASSERT_SHAPE_LAYER_SIGNATURE(accept, void (Layer::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_SHAPE_LAYER_SIGNATURE(visible, bool (Layer::*)(bool) const);
    ASSERT_SHAPE_LAYER_SIGNATURE(setVisible, void (Layer::*)(bool, bool));
    ASSERT_SHAPE_LAYER_SIGNATURE(setUserLocked, void (Layer::*)(bool));
    ASSERT_SHAPE_LAYER_SIGNATURE(isShapeEditable, bool (Layer::*)(bool) const);
    ASSERT_SHAPE_LAYER_SIGNATURE(isSelectable, bool (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(setParent, void (Layer::*)(KoShapeContainer *));

    static_assert(std::is_same_v<decltype(std::declval<const Layer &>().visible()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<Layer &>().setVisible(true)), void>);
}

void KisShapeLayerSchemaContractTest::shapeLayerDeviceAndGeometrySignaturesRemainStable()
{
    using Layer = KisShapeLayer;
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;

    ASSERT_SHAPE_LAYER_SIGNATURE(icon, QIcon (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(resetCache, void (Layer::*)(const KoColorSpace *));
    ASSERT_SHAPE_LAYER_SIGNATURE(original, KisPaintDeviceSP (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(paintDevice, KisPaintDeviceSP (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(theoreticalBoundingRect, QRect (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(x, qint32 (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(y, qint32 (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(setX, void (Layer::*)(qint32));
    ASSERT_SHAPE_LAYER_SIGNATURE(setY, void (Layer::*)(qint32));
    ASSERT_SHAPE_LAYER_SIGNATURE(crop, KUndo2Command * (Layer::*)(const QRect &));
    ASSERT_SHAPE_LAYER_SIGNATURE(transform, KUndo2Command * (Layer::*)(const QTransform &));
    ASSERT_SHAPE_LAYER_SIGNATURE(setProfile, KUndo2Command * (Layer::*)(const KoColorProfile *));
    ASSERT_SHAPE_LAYER_SIGNATURE(convertTo, KUndo2Command * (Layer::*)(const KoColorSpace *, Intent, ConversionFlags));

    static_assert(std::is_same_v<decltype(std::declval<Layer &>().convertTo(nullptr)), KUndo2Command *>);
}

void KisShapeLayerSchemaContractTest::shapeLayerShapeAndPersistenceSignaturesRemainStable()
{
    using Layer = KisShapeLayer;
    using CreateShapes = QList<KoShape *> (*)(QIODevice *,
                                              const QString &,
                                              const QRectF &,
                                              qreal,
                                              KoDocumentResourceManager *,
                                              bool,
                                              QSizeF *,
                                              QStringList *,
                                              QStringList *);

    ASSERT_SHAPE_LAYER_SIGNATURE(shapeManager, KoShapeManager * (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(saveShapesToStore, bool (*)(KoStore *, QList<KoShape *>, const QSizeF &));
    ASSERT_SHAPE_LAYER_SIGNATURE(createShapesFromSvg, CreateShapes);
    ASSERT_SHAPE_LAYER_SIGNATURE(saveLayer, bool (Layer::*)(KoStore *) const);
    ASSERT_SHAPE_LAYER_SIGNATURE(loadLayer, bool (Layer::*)(KoStore *, QStringList *));
    ASSERT_SHAPE_LAYER_SIGNATURE(selectedShapesProxy, KoSelectedShapesProxy * (Layer::*)());
    ASSERT_SHAPE_LAYER_SIGNATURE(antialiased, bool (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(setAntialiased, void (Layer::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<Layer &>().loadLayer(nullptr)), bool>);
    static_assert(std::is_same_v<decltype(Layer::createShapesFromSvg(nullptr,
                                                                     std::declval<const QString &>(),
                                                                     std::declval<const QRectF &>(),
                                                                     qreal{},
                                                                     nullptr,
                                                                     false,
                                                                     nullptr)),
                                 QList<KoShape *>>);
}

void KisShapeLayerSchemaContractTest::shapeLayerSectionUpdateAndNotificationSignaturesRemainStable()
{
    using Layer = KisShapeLayer;

    ASSERT_SHAPE_LAYER_SIGNATURE(forceUpdateTimedNode, void (Layer::*)());
    ASSERT_SHAPE_LAYER_SIGNATURE(hasPendingTimedUpdates, bool (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(forceUpdateHiddenAreaOnOriginal, void (Layer::*)());
    ASSERT_SHAPE_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (Layer::*)() const);
    ASSERT_SHAPE_LAYER_SIGNATURE(setSectionModelProperties, void (Layer::*)(const KisBaseNode::PropertyList &));
    ASSERT_SHAPE_LAYER_SIGNATURE(currentLayerChanged, void (Layer::*)(const KoShapeLayer *));
    ASSERT_SHAPE_LAYER_SIGNATURE(selectionChanged, void (Layer::*)());
    ASSERT_SHAPE_LAYER_SIGNATURE(sigMoveShapes, void (Layer::*)(const QPointF &));
}

void KisShapeLayerSchemaContractTest::shapeControllerTypeAndLifetimeSchemaRemainStable()
{
    using Controller = KisShapeController;

    static_assert(std::is_class_v<Controller>);
    static_assert(std::is_constructible_v<Controller, KisNameServer *, KUndo2Stack *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Controller>);
}

void KisShapeLayerSchemaContractTest::shapeControllerDummyQuerySignaturesRemainStable()
{
    using Controller = KisShapeController;

    ASSERT_SHAPE_CONTROLLER_SIGNATURE(hasDummyForNode, bool (Controller::*)(KisNodeSP) const);
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(dummyForNode, KisNodeDummy * (Controller::*)(KisNodeSP) const);
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(rootDummy, KisNodeDummy * (Controller::*)() const);
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(dummiesCount, int (Controller::*)() const);
}

void KisShapeLayerSchemaContractTest::shapeControllerShapeCanvasAndImageSignaturesRemainStable()
{
    using Controller = KisShapeController;

    ASSERT_SHAPE_CONTROLLER_SIGNATURE(shapeForNode, KoShapeLayer * (Controller::*)(KisNodeSP) const);
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(setInitialShapeForCanvas, void (Controller::*)(KisCanvas2 *));
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(setImage, void (Controller::*)(KisImageWSP, KisNodeSP));
}

void KisShapeLayerSchemaContractTest::shapeControllerCreationAndDocumentGeometrySignaturesRemainStable()
{
    using Controller = KisShapeController;

    ASSERT_SHAPE_CONTROLLER_SIGNATURE(createParentForShapes,
                                      KoShapeContainer * (Controller::*)(QList<KoShape *>, bool, KUndo2Command *));
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(documentRectInPixels, QRectF (Controller::*)() const);
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(pixelsPerInch, qreal (Controller::*)() const);
}

void KisShapeLayerSchemaContractTest::shapeControllerNotificationSignaturesRemainStable()
{
    using Controller = KisShapeController;

    ASSERT_SHAPE_CONTROLLER_SIGNATURE(selectionChanged, void (Controller::*)());
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(selectionContentChanged, void (Controller::*)());
    ASSERT_SHAPE_CONTROLLER_SIGNATURE(currentLayerChanged, void (Controller::*)(const KoShapeLayer *));
}

#undef ASSERT_SHAPE_LAYER_SIGNATURE
#undef ASSERT_SHAPE_CONTROLLER_SIGNATURE

QTEST_APPLESS_MAIN(KisShapeLayerSchemaContractTest)

#include "KisShapeLayerSchemaContractTest.moc"
